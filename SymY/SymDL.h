#pragma once
#include<windows.h>
#include"CurlSelectMulti.h"
extern size_t g_imagesizescale;
extern WCHAR* g_importtable;
bool DownloadAndVerifyFiles(CurlSelectMulti* m_multServer, LPCWSTR symname, LPCWSTR arch, bool isdebug, LPCWSTR outputdir);
void DownSymWithThreads(CurlSelectMulti* m_multServer, LPCWSTR symname, size_t timestampmin, size_t timestampmax, size_t imagesizemin, size_t imagesizemax, LPCWSTR outputdir);
bool SearchAndDownload(CurlSelectMulti* m_multServer, LPCWSTR symname, size_t timestampmin, size_t timestampmax, size_t imagesizemin, size_t imagesizemax, LPCWSTR outputdir, LPCWSTR arch, bool isdebug);
void SearchAndDownloadAndMustFind(CurlSelectMulti* m_multServer, LPCWSTR symname, size_t timestampmin, size_t timestampmax, size_t imagesizemin, size_t imagesizemax, LPCWSTR outputdir, LPCWSTR arch, bool isdebug);
void SearchAndDownloadAndMustFindTs(CurlSelectMulti* m_multServer, LPCWSTR symname, size_t timestampmin, size_t timestampmax, size_t imagesizemin, size_t imagesizemax, LPCWSTR outputdir, LPCWSTR arch, bool isdebug);
void SearchAndDownloadAndMustFindIs(CurlSelectMulti* m_multServer, LPCWSTR symname, size_t timestampmin, size_t timestampmax, size_t imagesizemin, size_t imagesizemax, LPCWSTR outputdir, LPCWSTR arch, bool isdebug);