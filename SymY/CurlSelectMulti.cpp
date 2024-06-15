#include "CurlSelectMulti.h"
#include <iostream>
#include <chrono>

#define SYMY_MAX_THREADS 50

using namespace std;

// Debug callback function for CURL
static int OnDebug(CURL* handle, curl_infotype itype, char* pData, size_t size, void* userptr)
{
    if (itype == CURLINFO_TEXT)
    {
        // printf("[TEXT]%s\n", pData);
    }
    else if (itype == CURLINFO_HEADER_IN)
    {
        printf("[HEADER_IN]%s\n", pData);
    }
    else if (itype == CURLINFO_HEADER_OUT)
    {
        printf("[HEADER_OUT]%s\n", pData);
    }
    else if (itype == CURLINFO_DATA_IN)
    {
        printf("[DATA_IN]%s\n", pData);
    }
    else if (itype == CURLINFO_DATA_OUT)
    {
        printf("[DATA_OUT]%s\n", pData);
    }
    return 0;
}

// Write callback function for CURL
static size_t OnWriteData(void* buffer, size_t size, size_t nmemb, void* userptr)
{
    std::string* str = static_cast<std::string*>(userptr);
    if (str == nullptr || buffer == nullptr)
    {
        return -1;
    }

    char* pData = static_cast<char*>(buffer);
    str->append(pData, size * nmemb);
    return nmemb;
}

// Constructor
CurlSelectMulti::CurlSelectMulti()
{
	m_bDebug=false;
	m_pMultiHandle=nullptr;
	m_bRunning=true;
	m_curlNum=0;
	m_successNum=0;
	m_failedNum=0;
}

// Destructor
CurlSelectMulti::~CurlSelectMulti()
{
    finit();
}

// Global initialization for CURL
void CurlSelectMulti::GlobalInit()
{
    curl_global_init(CURL_GLOBAL_ALL);
}

// Global cleanup for CURL
void CurlSelectMulti::GlobalFinit()
{
    curl_global_cleanup();
}

// Initialize the multi CURL handler and threads
int CurlSelectMulti::init()
{
    m_pMultiHandle = curl_multi_init();
    if (m_pMultiHandle == nullptr)
    {
        return false;
    }
    m_bRunning = true;
    m_taskAddThread = std::thread(&CurlSelectMulti::dealTask, this);
    m_taskHandThread = std::thread(&CurlSelectMulti::handTaskResult, this);
    return true;
}

// Cleanup and finalize the multi CURL handler and threads
void CurlSelectMulti::finit()
{
    m_bRunning = false;
    m_conVarTask.notify_all();

    if (m_taskAddThread.joinable()) m_taskAddThread.join();
    if (m_taskHandThread.joinable()) m_taskHandThread.join();

    while (!m_listEasyHandle.empty())
    {
        auto it = m_listEasyHandle.front();
        curl_multi_remove_handle(m_pMultiHandle, it);
        curl_easy_cleanup(it);
        m_listEasyHandle.pop_front();
    }

    if (m_pMultiHandle != nullptr)
    {
        curl_multi_cleanup(m_pMultiHandle);
        m_pMultiHandle = nullptr;
    }
}

// Add a task to the task queue
void CurlSelectMulti::addTask(shared_ptr<Task>& task)
{
    if (m_listTask.size() > 1000000)
    {
        return;
    }
    unique_lock<mutex> lk(m_taskMutex);
    m_listTask.push_back(task);
    lk.unlock();
    m_conVarTask.notify_one();
}

// Get an easy handle from the pool, or create a new one if the pool is empty
CURL* CurlSelectMulti::GetCurl()
{
    CURL* curl = nullptr;
    m_easyHandleMutex.lock();
    if (!m_listEasyHandle.empty())
    {
        curl = m_listEasyHandle.front();
        m_listEasyHandle.pop_front();
    }
    m_easyHandleMutex.unlock();
    if (curl == nullptr)
    {
        curl = CreateCurl();
    }
    return curl;
}

// Create a new easy handle
CURL* CurlSelectMulti::CreateCurl()
{
    if (m_curlNum > SYMY_MAX_THREADS)
    {
        return nullptr;
    }
    m_curlNum++;
    CURL* curl = curl_easy_init();
    if (curl == nullptr)
    {
        return nullptr;
    }
    if (m_bDebug)
    {
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, OnDebug);
    }
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnWriteData);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 300L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 200L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 100);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl, CURLOPT_COOKIESESSION, 1);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 100);
    return curl;
}

// Return an easy handle to the pool
void CurlSelectMulti::PutCurl(CURL* curl)
{
    m_easyHandleMutex.lock();
    m_listEasyHandle.push_back(curl);
    m_easyHandleMutex.unlock();
}

// Process tasks: continuously fetch tasks from the queue and add them to multi handle
void CurlSelectMulti::dealTask()
{
    while (m_bRunning)
    {
        unique_lock<mutex> lk(m_taskMutex);
        m_conVarTask.wait_for(lk, chrono::milliseconds(20));
        if (!m_listTask.empty())
        {
			list<shared_ptr<Task>> listTask;
            listTask.swap(m_listTask);
            lk.unlock();
            addTaskToMultiRequest(listTask);
        }
        else
        {
            lk.unlock();
            this_thread::sleep_for(chrono::milliseconds(20));
        }
    }
}

// Handle task results: check if tasks are completed
void CurlSelectMulti::handTaskResult()
{
    CURLMcode mc = CURLM_OK;
    int still_running = 0;
    int ret = 0;
    while (m_bRunning)
    {
        m_curlApiMutex.lock();
        mc = curl_multi_perform(m_pMultiHandle, &still_running);
        m_curlApiMutex.unlock();
        mc = curl_multi_poll(m_pMultiHandle, nullptr, 0, 1000, &ret);
        if (mc == CURLM_OK)
        {
            readTaskResult();
        }
        else
        {
            printf("curl_multi_poll error %d", mc);
        }
    }
}

// Read and process completed tasks
void CurlSelectMulti::readTaskResult()
{
    CURLMsg* m = nullptr;
    do {
        int msgq = 0;
        m_curlApiMutex.lock();
        m = curl_multi_info_read(m_pMultiHandle, &msgq);
        m_curlApiMutex.unlock();
        if (m && (m->msg == CURLMSG_DONE))
        {
            CURL* e = m->easy_handle;
            auto it = m_mapRunningTask.find(e);
            if (it != m_mapRunningTask.end())
            {
                if (m->data.result != CURLE_OK)
                {
                    m_failedNum++;
                    printf("Failed %d, failedNum %d, taskid %d, %s\n", m->data.result, m_failedNum, m_mapRunningTask[e]->taskid, m_mapRunningTask[e]->strUrl.c_str());
                    m_foundFiles.push_back(m_mapRunningTask[e]->strUrl);
                }
                else
                {
                    m_successNum++;
                    long lStateCode = 0;
                    m->data.result = curl_easy_getinfo(m->easy_handle, CURLINFO_RESPONSE_CODE, &lStateCode);
                    if (lStateCode != 200)
                    {
                        fprintf(stderr, "request error %ld, failedNum %d, taskid %d, %s\n", lStateCode, m_failedNum, m_mapRunningTask[e]->taskid, m_mapRunningTask[e]->strUrl.c_str());
                    }
                    else
                    {
                        printf("Found taskid %d, %s\n", m_mapRunningTask[e]->taskid, m_mapRunningTask[e]->strUrl.c_str());
                        m_foundFiles.push_back(m_mapRunningTask[e]->strUrl);
                    }
                }
                m_curlApiMutex.lock();
                curl_multi_remove_handle(m_pMultiHandle, e);
                m_curlApiMutex.unlock();
                m_runningTaskMutex.lock();
                if (it->second->headers != nullptr)
                {
                    curl_slist_free_all(it->second->headers);
                }
                m_mapRunningTask.erase(it);
                m_runningTaskMutex.unlock();
                PutCurl(e);
            }
            else
            {
                m_curlApiMutex.lock();
                curl_multi_remove_handle(m_pMultiHandle, e);
                m_curlApiMutex.unlock();
                printf("find map key failed");
                PutCurl(e);
            }
        }
    } while (m);
}

// Add tasks to the multi handle and execute them
void CurlSelectMulti::addTaskToMultiRequest(list<shared_ptr<Task>>& listTask)
{
    while (!listTask.empty())
    {
        auto item = listTask.front();
        CURL* easy_handle = GetCurl();
        if (easy_handle == nullptr)
        {
            this_thread::sleep_for(chrono::milliseconds(1));
            continue;
        }
        if (setTaskParameter(easy_handle, item) != 0)
        {
            this_thread::sleep_for(chrono::milliseconds(2));
            PutCurl(easy_handle);
            continue;
        }
        m_curlApiMutex.lock();
        while (m_mapRunningTask.size() > SYMY_MAX_THREADS);
        CURLMcode code = curl_multi_add_handle(m_pMultiHandle, easy_handle);
        m_curlApiMutex.unlock();
        if (code != CURLM_OK)
        {
            m_addMultiFailed++;
            string strerror = curl_multi_strerror(code);
            printf("curl_multi_add_handle failed %d, %s\n", m_addMultiFailed, strerror.c_str());
            PutCurl(easy_handle);
            continue;
        }
        m_runningTaskMutex.lock();
        m_mapRunningTask[easy_handle]=item;
        m_runningTaskMutex.unlock();
        listTask.pop_front();
    }
}

// Set parameters for the easy handle based on the task details
int CurlSelectMulti::setTaskParameter(CURL* easy_handle, shared_ptr<Task>& task)
{
    CURLcode code = CURLE_OK;
    if (task->iType == HTTP_POST || task->iType == HTTPS_POST)
    {
        code = curl_easy_setopt(easy_handle, CURLOPT_POST, 1L);
        if (code != CURLE_OK)
        {
            printf("curl_easy_setopt error %d", code);
            return -1;
        }
        code = curl_easy_setopt(easy_handle, CURLOPT_POSTFIELDSIZE, task->strPostContent.size());
        if (code != CURLE_OK)
        {
            printf("curl_easy_setopt error %d", code);
            return -1;
        }
        code = curl_easy_setopt(easy_handle, CURLOPT_POSTFIELDS, task->strPostContent.data());
        if (code != CURLE_OK)
        {
            printf("curl_easy_setopt error %d", code);
            return -1;
        }
    }
    code = curl_easy_setopt(easy_handle, CURLOPT_URL, task->strUrl.c_str());
    if (code != CURLE_OK)
    {
        printf("curl_easy_setopt error %d", code);
        return -1;
    }
    code = curl_easy_setopt(easy_handle, CURLOPT_NOBODY, 1L);
    if (code != CURLE_OK)
    {
        printf("curl_easy_setopt error %d", code);
        return -1;
    }
    code = curl_easy_setopt(easy_handle, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/109.0.0.0 Safari/537.36 Edg/109.0.1518.52");
    if (code != CURLE_OK)
    {
        printf("curl_easy_setopt error %d", code);
        return -1;
    }
    if (task->headers != nullptr)
    {
        code = curl_easy_setopt(easy_handle, CURLOPT_HTTPHEADER, task->headers);
        if (code != CURLE_OK)
        {
            printf("curl_easy_setopt error %d", code);
            return -1;
        }
    }
    return 0;
}
