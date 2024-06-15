// SymY.cpp 
//

#include <iostream>
#include<Windows.h>
#include "SymDL.h"
#pragma comment(lib,"urlmon.lib")
#pragma comment(lib,"shlwapi.lib")
#include <stdio.h>
#include <Shlwapi.h>
#include <ctime>
int main()
{
	TCHAR SymYini[MAX_PATH + 1] = { 0 };
	GetModuleFileName(NULL, SymYini, MAX_PATH);
	(wcsrchr(SymYini, TEXT('\\')))[1] = 0; // 删除文件名，只获得路径字串
	wcscat_s(SymYini, L"SymY.ini");
	WCHAR symname[90];
	g_importtable = new WCHAR[20000];
	memset(g_importtable, 0, sizeof(WCHAR)*20000);
	GetPrivateProfileStringW(L"Config", L"ImportDLLS", L"", g_importtable, 1000, SymYini);
	GetPrivateProfileStringW(L"Config", L"Symname", L"", symname,90, SymYini);
	size_t timestampmin = GetPrivateProfileIntW(L"Config", L"TimeStampmin", 0, SymYini);
	size_t timestampmax = GetPrivateProfileIntW(L"Config", L"TimeStampmax", 0, SymYini);
	size_t imagesizemin = GetPrivateProfileIntW(L"Config", L"ImageSizemin", 0, SymYini);
	size_t imagesizemax = GetPrivateProfileIntW(L"Config", L"ImageSizemax", 0, SymYini);
	bool isdebug = GetPrivateProfileIntW(L"Config", L"isdebug", 0, SymYini)==1;
	bool timesearch = GetPrivateProfileIntW(L"Config", L"Timesearch", 0, SymYini) == 1;
	bool imagesizesearch = GetPrivateProfileIntW(L"Config", L"ImageSizesearch", 0, SymYini) == 1;
	bool autosrmlib = GetPrivateProfileIntW(L"Config", L"autosrmlib", 0, SymYini) == 1;
	g_imagesizescale= GetPrivateProfileIntW(L"Config", L"ImageSizeScale", 0, SymYini);
	if (g_imagesizescale == 0) g_imagesizescale = 0x1000;
	WCHAR arch[9];
	GetPrivateProfileStringW(L"Config", L"arch", L"", arch, 9, SymYini);
	CurlSelectMulti multServer;
	multServer.GlobalInit();
    multServer.init();
	if (!PathFileExists(SymYini)) {
		size_t time= std::time(NULL);
		imagesizemin = 0x1a000;
		imagesizemax = 0x1a000;
		timestampmin = time - 86400 * 4;
		timestampmax = time;
		timesearch = true;
		wsprintfW(symname,L"srmlib.dll");
		isdebug = true;
		wsprintfW(arch, L"mips");

	}
	wprintf_s(L"%ls\n", symname);
	if(timesearch)SearchAndDownloadAndMustFindTs(&multServer, symname, timestampmin, timestampmax, imagesizemin, imagesizemax, L".", arch, isdebug);
	else if (imagesizesearch)SearchAndDownloadAndMustFindIs(&multServer, symname, timestampmin, timestampmax, imagesizemin, imagesizemax, L".", arch, isdebug);
	else SearchAndDownloadAndMustFind(&multServer, symname, timestampmin, timestampmax, imagesizemin, imagesizemax, L".", arch, isdebug);
	list<std::string>::iterator iter;
	printf("Done\n");
	for (iter = multServer.m_foundFiles.begin(); iter != multServer.m_foundFiles.end(); iter++)
	{
		std::string url = *iter;

		printf(url.c_str());
		std::string outputpath = url.substr(url.rfind("/")+1);
		std::string tmp = url.substr(0, url.rfind("/"));
			outputpath = std::string(".\\").append(tmp.substr(tmp.rfind("/") + 1)).append("_").append(outputpath);

		printf(outputpath.c_str());
		URLDownloadToFileA(NULL, url.c_str(), outputpath.c_str(), 0, NULL);
	}
	delete[]g_importtable;
	exit(0);
	return (int)0;
}