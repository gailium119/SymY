#include"symdl.h"
#include <wchar.h>
#pragma comment(lib,"urlmon.lib")
#pragma comment(lib,"wininet.lib")
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"crypt32.lib")
#pragma comment(lib,"wldap32.lib")
#include <wininet.h>
#include <stdio.h>
#include<thread>

#include <shlwapi.h>
#include "CurlSelectMulti.h"
#include "PE.h"
#pragma comment(lib,"Shlwapi.lib") 
size_t g_imagesizescale = 0x1000;
wchar_t* g_importtable;
int g_threads = 0;
int g_maxthreads =30;
bool g_found = false;
FILE* logfile;
void DownSymWithThreads(CurlSelectMulti* m_multServer,LPCWSTR symname, size_t timestampmin, size_t timestampmax, size_t imagesizemin, size_t imagesizemax, LPCWSTR outputdir, int threads) {

	//m_multServer->GlobalInit();
	//m_multServer->init();


	int taskid = 0;
	int time = 0;
	//while (true)
	{
		for (size_t i = timestampmin; i <= timestampmax; i++)
		{
			for (size_t j = imagesizemin;j <= imagesizemax;j+= g_imagesizescale) {
				shared_ptr<Task> task = std::make_shared<Task>();
				task->iType = HTTPS_GET;
				char buffer[260] = "";
				sprintf_s(buffer, "https://msdl.microsoft.com/download/symbols/%ls/%08x%x/%ls", symname,(unsigned int)i, (unsigned int)j, symname);
				task->strUrl = buffer;

				taskid++;
				task->taskid = taskid;
				task->headers=NULL; //Fix 0xcdcdcdcd
				m_multServer->addTask(task);
			}
		}
		//printf("using time %ds,task num %d\n", (time++) * 1, time * 100);
	}
	Sleep(4000);
	while (m_multServer->m_mapRunningTask.size() > 0) {
		//printf_s("running task size:%d\n", m_multServer.m_mapRuningTask.size());
		Sleep(1000);
	}
}
void SearchAndDownload(CurlSelectMulti* m_multServer, LPCWSTR symname, size_t timestampmin, size_t timestampmax, size_t imagesizemin, size_t imagesizemax,LPCWSTR outputdir) {
	DownSymWithThreads(m_multServer,symname,timestampmin,timestampmax,imagesizemin,imagesizemax,outputdir,g_maxthreads);
}
void SearchAndDownloadAndMustFind(CurlSelectMulti* m_multServer, LPCWSTR symname, size_t timestampmin, size_t timestampmax, size_t imagesizemin, size_t imagesizemax, LPCWSTR outputdir,LPCWSTR arch,bool isdebug) {
	SearchAndDownload(m_multServer, symname, timestampmin, timestampmax, imagesizemin, imagesizemax, outputdir);
	if (m_multServer->m_foundFiles.size() != 0) {
		list<std::string>::iterator iter;
		bool bbreak = false;
		for (iter = m_multServer->m_foundFiles.begin(); iter != m_multServer->m_foundFiles.end(); iter++)
		{
			std::string url = *iter;

			printf(url.c_str());
			std::string outputpath = url.substr(url.rfind("/") + 1);
			std::string tmp = url.substr(0, url.rfind("/"));
			outputpath = std::string(".\\").append(tmp.substr(tmp.rfind("/") + 1)).append("_").append(outputpath);

			printf(outputpath.c_str());
			printf("\n");
			URLDownloadToFileA(NULL, url.c_str(), outputpath.c_str(), 0, NULL);
			if (VerifyPE(outputpath.c_str(), arch, isdebug, symname,g_importtable))bbreak = true;
		}
		m_multServer->m_foundFiles.clear();
		if (bbreak)return;;
	}

	//if (m_multServer->m_foundFiles.size() == 0) {
		SearchAndDownload(m_multServer, symname, timestampmin-(timestampmax-timestampmin)/2, timestampmin, imagesizemin, imagesizemax, outputdir);
		SearchAndDownload(m_multServer, symname, timestampmax, timestampmax+ (timestampmax - timestampmin) / 2, imagesizemin, imagesizemax, outputdir);
		size_t t = (timestampmax - timestampmin) / 2;
		timestampmin = timestampmin - t;
		timestampmax = timestampmax + t;

	//}
	while (imagesizemin >0) {
		imagesizemin -= g_imagesizescale;
		imagesizemax += g_imagesizescale;
		if (m_multServer->m_foundFiles.size() != 0) {
			list<std::string>::iterator iter;
			bool bbreak = false;
			for (iter = m_multServer->m_foundFiles.begin(); iter != m_multServer->m_foundFiles.end(); iter++)
			{
				std::string url = *iter;

				printf(url.c_str());
				std::string outputpath = url.substr(url.rfind("/") + 1);
				std::string tmp = url.substr(0, url.rfind("/"));
			    outputpath = std::string(".\\").append(tmp.substr(tmp.rfind("/") + 1)).append("_").append(outputpath);

				printf(outputpath.c_str());
				printf("\n");
				URLDownloadToFileA(NULL, url.c_str(), outputpath.c_str(), 0, NULL);
				if (VerifyPE(outputpath.c_str(), arch, isdebug,symname, g_importtable))bbreak = true;
			}
			m_multServer->m_foundFiles.clear();
			if (bbreak)break;
		}
		SearchAndDownload(m_multServer, symname, timestampmin, timestampmax, imagesizemax, imagesizemax, outputdir);
		if (m_multServer->m_foundFiles.size() != 0) {
			list<std::string>::iterator iter;
			bool bbreak = false;
			for (iter = m_multServer->m_foundFiles.begin(); iter != m_multServer->m_foundFiles.end(); iter++)
			{
				std::string url = *iter;

				printf(url.c_str());
				std::string outputpath = url.substr(url.rfind("/") + 1);
				std::string tmp = url.substr(0, url.rfind("/"));
			    outputpath = std::string(".\\").append(tmp.substr(tmp.rfind("/") + 1)).append("_").append(outputpath);

				printf(outputpath.c_str());
				printf("\n");
				URLDownloadToFileA(NULL, url.c_str(), outputpath.c_str(), 0, NULL);
				if (VerifyPE(outputpath.c_str(), arch, isdebug,symname, g_importtable))bbreak = true;
			}
			m_multServer->m_foundFiles.clear();
			if (bbreak)break;
		}
		SearchAndDownload(m_multServer, symname, timestampmin, timestampmax, imagesizemin, imagesizemin, outputdir);
		

	}
}
void SearchAndDownloadAndMustFindTs(CurlSelectMulti* m_multServer, LPCWSTR symname, size_t timestampmin, size_t timestampmax, size_t imagesizemin, size_t imagesizemax, LPCWSTR outputdir, LPCWSTR arch, bool isdebug) {
	//SearchAndDownload(m_multServer, symname, timestampmin, timestampmax, imagesizemin, imagesizemax, outputdir);
	TCHAR SymYini[MAX_PATH + 1] = { 0 };
	GetModuleFileName(NULL, SymYini, MAX_PATH);
	(wcsrchr(SymYini, TEXT('\\')))[1] = 0; // 删除文件名，只获得路径字串
	wcscat_s(SymYini, L"SymY.ini");
	while (timestampmin<timestampmax) {

		if (m_multServer->m_foundFiles.size() != 0) {
			list<std::string>::iterator iter;
			bool bbreak = false;
			for (iter = m_multServer->m_foundFiles.begin(); iter != m_multServer->m_foundFiles.end(); iter++)
			{
				std::string url = *iter;

				printf(url.c_str());
				std::string outputpath = url.substr(url.rfind("/") + 1);
				std::string tmp = url.substr(0, url.rfind("/"));
			    outputpath = std::string(".\\").append(tmp.substr(tmp.rfind("/") + 1)).append("_").append(outputpath);

				printf(outputpath.c_str());
				printf("\n");
				URLDownloadToFileA(NULL, url.c_str(), outputpath.c_str(), 0, NULL);
				if (VerifyPE(outputpath.c_str(), arch, isdebug,symname, g_importtable))bbreak = true;
			}
			m_multServer->m_foundFiles.clear();
			if (bbreak)break;
		}
		SearchAndDownload(m_multServer, symname, timestampmin, timestampmin+0xfff, imagesizemin, imagesizemax, outputdir);
		timestampmin += 0x1000;
		WritePrivateProfileInt(L"Config", L"TimeStampmin", (INT)timestampmin, SymYini);

	}
}
void SearchAndDownloadAndMustFindIs(CurlSelectMulti* m_multServer, LPCWSTR symname, size_t timestampmin, size_t timestampmax, size_t imagesizemin, size_t imagesizemax, LPCWSTR outputdir, LPCWSTR arch, bool isdebug) {
	//SearchAndDownload(m_multServer, symname, timestampmin, timestampmax, imagesizemin, imagesizemax, outputdir);
	TCHAR SymYini[MAX_PATH + 1] = { 0 };
	GetModuleFileName(NULL, SymYini, MAX_PATH);
	(wcsrchr(SymYini, TEXT('\\')))[1] = 0; // 删除文件名，只获得路径字串
	wcscat_s(SymYini, L"SymY.ini");
	if (!imagesizemax) imagesizemax = 0x7fffffff;
	int scale = 4096 / (int)(timestampmax-timestampmin);
	
	while (imagesizemin < imagesizemax) {

		if (m_multServer->m_foundFiles.size() != 0) {
			list<std::string>::iterator iter;
			bool bbreak = false;
			for (iter = m_multServer->m_foundFiles.begin(); iter != m_multServer->m_foundFiles.end(); iter++)
			{
				std::string url = *iter;

				printf(url.c_str());
				std::string outputpath = url.substr(url.rfind("/") + 1);
				std::string tmp = url.substr(0, url.rfind("/"));
			outputpath = std::string(".\\").append(tmp.substr(tmp.rfind("/") + 1)).append("_").append(outputpath);

				printf(outputpath.c_str());
				printf("\n");
				URLDownloadToFileA(NULL, url.c_str(), outputpath.c_str(), 0, NULL);
				if (VerifyPE(outputpath.c_str(), arch, isdebug, symname, g_importtable))bbreak = true;
			}
			m_multServer->m_foundFiles.clear();
			if (bbreak)break;
		}
		SearchAndDownload(m_multServer, symname, timestampmin, timestampmax, imagesizemin, imagesizemin + scale * 0x1000, outputdir);
		imagesizemin += scale * 0x1000;
		WritePrivateProfileInt(L"Config", L"ImageSizeMin", (INT)imagesizemin, SymYini);

	}
}
