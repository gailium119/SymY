#pragma once

#include <curl/curl.h>
#include <mutex>
#include <condition_variable>
#include <list>
#include <thread>
#include <map>
#include <memory>

#define HTTPS_GET 1
#define HTTP_POST 2
#define HTTPS_POST 3

using namespace std;

// Task structure to hold request details
typedef struct _Task_ {
    int iType;
    std::string strUrl;
    int taskid;
    curl_slist* headers;
    std::string strResponse;
    std::string strPostContent;
} Task;

class CurlSelectMulti
{
public:
    CurlSelectMulti();
    ~CurlSelectMulti();
    // Global initialization for CURL
    static void GlobalInit();
    // Global cleanup for CURL
    static void GlobalFinit();
    // Initialize the multi CURL handler and threads
    int init();
    // Cleanup and finalize the multi CURL handler and threads
    void finit();
    // Add a task to the task queue
    void addTask(shared_ptr<Task>& task);
    // Process tasks: continuously fetch tasks from the queue and add them to multi handle
    void dealTask();
    // Handle task results: check if tasks are completed
    void handTaskResult();
    // Read and process completed tasks
    void readTaskResult();
    // Get an easy handle from the pool, or create a new one if the pool is empty
    CURL* GetCurl();
    // Create a new easy handle
    CURL* CreateCurl();
    // Return an easy handle to the pool
    void PutCurl(CURL* curl);
    // Add tasks to the multi handle and execute them
    void addTaskToMultiRequest(list<shared_ptr<Task>>& listTask);
    // Set parameters for the easy handle based on the task details
    int setTaskParameter(CURL* easy_handle, shared_ptr<Task>& task);

    bool m_bDebug;
    CURL* m_pMultiHandle; // Multi CURL handle
    list<shared_ptr<Task>> m_listTask; // Task queue
    list<std::string> m_foundFiles; // List of found files
    mutex m_taskMutex; // Mutex for task queue
    mutex m_easyHandleMutex; // Mutex for easy handle pool
    list<CURL*> m_listEasyHandle; // Easy handle pool
    bool m_bRunning; // Thread control flag
    thread m_taskAddThread; // Thread for adding tasks
    thread m_taskHandThread; // Thread for handling task results
    condition_variable m_conVarTask; // Condition variable for task queue
    map<CURL*, shared_ptr<Task>> m_mapRunningTask; // Map of running tasks
    mutex m_runningTaskMutex; // Mutex for running tasks map
    mutex m_curlApiMutex; // Mutex for CURL API calls
    int m_curlNum; // Number of easy handles created
    int m_successNum; // Number of successful tasks
    int m_failedNum; // Number of failed tasks
    int m_addMultiFailed; // Number of failed additions to multi handle
};
