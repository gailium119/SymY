#include "symdl.h"
#include <wchar.h>
#include <stdio.h>
#include <thread>
#include <shlwapi.h>
#include "CurlSelectMulti.h"
#include "PE.h"
#include "Download.h"

#pragma comment(lib,"advapi32.lib")
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"crypt32.lib")
#pragma comment(lib,"wldap32.lib")
#pragma comment(lib,"Shlwapi.lib")

size_t g_imagesizescale = 0x1000;
wchar_t* g_importtable;
int g_maxthreads = 30;
void DownSymWithThreads(CurlSelectMulti* m_multServer, LPCWSTR symname, size_t timestampmin, size_t timestampmax, size_t imagesizemin, size_t imagesizemax, LPCWSTR outputdir) {
    int taskid = 0;
    for (size_t i = timestampmin; i <= timestampmax; i++) {
        for (size_t j = imagesizemin; j <= imagesizemax; j += g_imagesizescale) {
            shared_ptr<Task> task = std::make_shared<Task>();
            task->iType = HTTPS_GET;
            char buffer[260] = "";
            sprintf_s(buffer, "https://msdl.microsoft.com/download/symbols/%ls/%08x%x/%ls", symname, (unsigned int)i, (unsigned int)j, symname);
            task->strUrl = buffer;

            taskid++;
            task->taskid = taskid;
            task->headers = NULL; // Fix 0xcdcdcdcd
            m_multServer->addTask(task);
        }
    }
    Sleep(4000);
    while (m_multServer->m_mapRunningTask.size() > 0) {
        Sleep(1000);
    }
}

bool DownloadAndVerifyFiles(CurlSelectMulti* m_multServer, LPCWSTR symname, LPCWSTR arch, bool isdebug, LPCWSTR outputdir) {
    if (m_multServer->m_foundFiles.size() != 0) {
        bool bFoundMatchingFile = false;
        for (auto iter = m_multServer->m_foundFiles.begin(); iter != m_multServer->m_foundFiles.end(); iter++) {
            std::string url = *iter;

            printf("%s\n", url.c_str());

            std::string outputpath = url.substr(url.rfind("/") + 1);
            std::string tmp = url.substr(0, url.rfind("/"));
            outputpath = std::string(".\\").append(tmp.substr(tmp.rfind("/") + 1)).append("_").append(outputpath);

			DownloadFileWithCurl(url,outputpath);
            if (VerifyPE(outputpath.c_str(), arch, isdebug, symname, g_importtable)) {
                bFoundMatchingFile = true;
                break;
            }
        }
        m_multServer->m_foundFiles.clear();
        if (bFoundMatchingFile) return true;
    }
    return false;
}

bool SearchAndDownload(CurlSelectMulti* m_multServer, LPCWSTR symname, size_t timestampmin, size_t timestampmax, size_t imagesizemin, size_t imagesizemax, LPCWSTR outputdir, LPCWSTR arch, bool isdebug) {
    DownSymWithThreads(m_multServer, symname, timestampmin, timestampmax, imagesizemin, imagesizemax, outputdir);
    return DownloadAndVerifyFiles(m_multServer, symname, arch, isdebug, outputdir);
}

void SearchAndDownloadAndMustFind(CurlSelectMulti* m_multServer, LPCWSTR symname, size_t timestampmin, size_t timestampmax, size_t imagesizemin, size_t imagesizemax, LPCWSTR outputdir, LPCWSTR arch, bool isdebug) {
    bool bret = SearchAndDownload(m_multServer, symname, timestampmin, timestampmax, imagesizemin, imagesizemax, outputdir, arch, isdebug);
    if (bret) return;
    //if (m_multServer->m_foundFiles.size() == 0) {
        bret = SearchAndDownload(m_multServer, symname, timestampmin - (timestampmax - timestampmin) / 2, timestampmin, imagesizemin, imagesizemax, outputdir, arch, isdebug);
        if (bret) return;
        bret = SearchAndDownload(m_multServer, symname, timestampmax, timestampmax + (timestampmax - timestampmin) / 2, imagesizemin, imagesizemax, outputdir, arch, isdebug);
        if (bret) return;
    //}
    while (imagesizemin > 0) {
        imagesizemin -= g_imagesizescale;
        imagesizemax += g_imagesizescale;
        bret = SearchAndDownload(m_multServer, symname, timestampmin - (timestampmax - timestampmin) / 2, timestampmax + (timestampmax - timestampmin) / 2, imagesizemax, imagesizemax, outputdir, arch, isdebug);
        if (bret) return; 
        bret = SearchAndDownload(m_multServer, symname, timestampmin - (timestampmax - timestampmin) / 2, timestampmax + (timestampmax - timestampmin) / 2, imagesizemin, imagesizemin, outputdir, arch, isdebug);
        if (bret) return;
    }
}

void SearchAndDownloadAndMustFindTs(CurlSelectMulti* m_multServer, LPCWSTR symname, size_t timestampmin, size_t timestampmax, size_t imagesizemin, size_t imagesizemax, LPCWSTR outputdir, LPCWSTR arch, bool isdebug) {
    TCHAR SymYini[MAX_PATH + 1] = { 0 };
    GetModuleFileName(NULL, SymYini, MAX_PATH);
    (wcsrchr(SymYini, TEXT('\\')))[1] = 0; // Delete file name, get only path string
    wcscat_s(SymYini, L"SymY.ini");

    while (timestampmin < timestampmax) {
        DownloadAndVerifyFiles(m_multServer, symname, arch, isdebug, outputdir);
        SearchAndDownload(m_multServer, symname, timestampmin, timestampmin + 0xfff, imagesizemin, imagesizemax, outputdir, arch, isdebug);
        timestampmin += 0x1000;
        WritePrivateProfileInt(L"Config", L"TimeStampmin", (INT)timestampmin, SymYini);
    }
}

void SearchAndDownloadAndMustFindIs(CurlSelectMulti* m_multServer, LPCWSTR symname, size_t timestampmin, size_t timestampmax, size_t imagesizemin, size_t imagesizemax, LPCWSTR outputdir, LPCWSTR arch, bool isdebug) {
    TCHAR SymYini[MAX_PATH + 1] = { 0 };
    GetModuleFileName(NULL, SymYini, MAX_PATH);
    (wcsrchr(SymYini, TEXT('\\')))[1] = 0; // Delete file name, get only path string
    wcscat_s(SymYini, L"SymY.ini");

    if (!imagesizemax) imagesizemax = 0x7fffffff;
    int scale = (timestampmax - timestampmin==0)? 4096 : 4096 / (int)(timestampmax - timestampmin);

    while (imagesizemin < imagesizemax) {
        DownloadAndVerifyFiles(m_multServer, symname, arch, isdebug, outputdir);
        SearchAndDownload(m_multServer, symname, timestampmin, timestampmax, imagesizemin, imagesizemin + scale * 0x1000, outputdir, arch, isdebug);
        imagesizemin += scale * 0x1000;
        WritePrivateProfileInt(L"Config", L"ImageSizeMin", (INT)imagesizemin, SymYini);
    }
}
