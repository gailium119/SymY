// GetPE.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <stdio.h>
#include<windows.h>
#include<strsafe.h>
#include<DbgHelp.h>
#include<crtdbg.h>
#include <string>
#include <Shlwapi.h>
#pragma comment(lib, "version.lib")
#pragma comment(lib, "dbghelp.lib")
#include<algorithm>
#include<string>
#include<vector>
//返回文件版本号
//@params:filename:文件名
struct Version {
    int VerMajor;
    int VerMinor;
    int VerFull;
    int VerUbr;
    bool isdebug;
	Version(){
    VerMajor = 0;
    VerMinor = 0;
    VerFull = 0;
    VerUbr = 0;
	isdebug = false;
	}
};
Version GetFileVersionW(LPCWSTR filename)
{
    Version ver;
    VS_FIXEDFILEINFO* pVsInfo;
    unsigned int iFileInfoSize = sizeof(VS_FIXEDFILEINFO);
    int iVerInfoSize = GetFileVersionInfoSizeW(filename, NULL);
    if (iVerInfoSize != 0)
    {
        char* pBuf = NULL;

        while (!pBuf)
        {
            pBuf = new char[iVerInfoSize];
        }
        if (GetFileVersionInfoW(filename, 0, iVerInfoSize, pBuf))
        {
            if (VerQueryValueA(pBuf, "\\", (void**)&pVsInfo, &iFileInfoSize))
            {
                ver.VerMajor = HIWORD(pVsInfo->dwFileVersionMS);
                ver.VerMinor = LOWORD(pVsInfo->dwFileVersionMS);
                ver.VerFull = HIWORD(pVsInfo->dwFileVersionLS);
                ver.VerUbr = LOWORD(pVsInfo->dwFileVersionLS);
                ver.isdebug = (pVsInfo->dwFileFlags == 1);
            }
        }
        delete pBuf;
    }
    return ver;
}
Version GetFileVersionA(LPCSTR filename)
{
    Version ver;
    VS_FIXEDFILEINFO* pVsInfo;
    unsigned int iFileInfoSize = sizeof(VS_FIXEDFILEINFO);
    int iVerInfoSize = GetFileVersionInfoSizeA(filename, NULL);
    if (iVerInfoSize != 0)
    {
        char* pBuf = NULL;

        while (!pBuf)
        {
            pBuf = new char[iVerInfoSize];
        }
        if (GetFileVersionInfoA(filename, 0, iVerInfoSize, pBuf))
        {
            if (VerQueryValueA(pBuf, "\\", (void**)&pVsInfo, &iFileInfoSize))
            {
                ver.VerMajor = HIWORD(pVsInfo->dwFileVersionMS);
                ver.VerMinor = LOWORD(pVsInfo->dwFileVersionMS);
                ver.VerFull = HIWORD(pVsInfo->dwFileVersionLS);
                ver.VerUbr = LOWORD(pVsInfo->dwFileVersionLS);
                ver.isdebug = (pVsInfo->dwFileFlags == 1);
            }
        }
        delete pBuf;
    }
    return ver;
}

BOOL WritePrivateProfileInt(
    LPCTSTR lpAppName,
    LPCTSTR lpKeyName,
    INT Value,
    LPCTSTR lpFileName) {

    TCHAR ValBuf[32] = { 0 };

    StringCbPrintfW(ValBuf, sizeof(ValBuf), TEXT("%d"), Value);

    return WritePrivateProfileString(lpAppName, lpKeyName, ValBuf, lpFileName);
}
std::string GetImportDLLs(LPCSTR path) {

    int i;
    std::string ret = "";
    std::vector<std::string> imports;
    imports.clear();
    HANDLE hFile = CreateFileA(
        path, //PE文件名
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        printf("Create File Failed.\n");
        return ret;
    }

    HANDLE hFileMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);

    if (hFileMapping == NULL || hFileMapping == INVALID_HANDLE_VALUE)
    {
        printf("Could not create file mapping object (%d).\n", GetLastError());
        return ret;
    }

    LPBYTE lpBaseAddress = (LPBYTE)MapViewOfFile(hFileMapping,   // handle to map object
        FILE_MAP_READ, 0, 0, 0);

    if (lpBaseAddress == NULL)
    {
        printf("Could not map view of file (%d).\n", GetLastError());
        return ret;
    }

    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)lpBaseAddress;
    PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)(lpBaseAddress + pDosHeader->e_lfanew);

    //导入表的rva：0x2a000;
    DWORD Rva_import_table = pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;

    if (Rva_import_table == 0)
    {
        UnmapViewOfFile(lpBaseAddress);
        CloseHandle(hFileMapping);
        CloseHandle(hFile);
        return ret;
    }

    //这个虽然是内存地址，但是减去文件开头的地址，就是文件地址了
    //这个地址可以直接从里面读取你想要的东西了
    PIMAGE_IMPORT_DESCRIPTOR pImportTable = (PIMAGE_IMPORT_DESCRIPTOR)ImageRvaToVa(
        pNtHeaders,
        lpBaseAddress,
        Rva_import_table,
        NULL
    );

    //减去内存映射的首地址，就是文件地址了。。（很简单吧）

    //现在来到了导入表的面前：IMAGE_IMPORT_DESCRIPTOR 数组（以0元素为终止）
    //定义表示数组结尾的null元素！
    IMAGE_IMPORT_DESCRIPTOR null_iid;
    IMAGE_THUNK_DATA null_thunk;
    memset(&null_iid, 0, sizeof(null_iid));
    memset(&null_thunk, 0, sizeof(null_thunk));

    //每个元素代表了一个引入的DLL。
    for (i = 0; memcmp(pImportTable + i, &null_iid, sizeof(null_iid)) != 0; i++)
    {
        //LPCSTR: 就是 const char*
        LPCSTR szDllName = (LPCSTR)ImageRvaToVa(
            pNtHeaders, lpBaseAddress,
            pImportTable[i].Name, //DLL名称的RVA
            NULL);

        //拿到了DLL的名字
        imports.push_back(szDllName);
        std::transform(imports[imports.size() - 1].begin(), imports[imports.size() - 1].end(), imports[imports.size() - 1].begin(), tolower);
    }



    //关闭文件，句柄。。
    UnmapViewOfFile(lpBaseAddress);
    CloseHandle(hFileMapping);
    CloseHandle(hFile);
    std::sort(imports.begin(), imports.end());
    for (int i = 0;i < imports.size();i++) {
        ret += imports[i];
        if (i != imports.size() - 1)ret += ",";
    }
    return ret;
}
std::string wstring2string(std::wstring wstr)
{
    std::string result;
    //获取缓冲区大小，并申请空间，缓冲区大小事按字节计算的  
    int len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
    char* buffer = new char[len + 1];
    //宽字节编码转换成多字节编码  
    WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), (int)wstr.size(), buffer, len, NULL, NULL);
    buffer[len] = '\0';
    //删除缓冲区并返回值  
    result.append(buffer);
    delete[] buffer;
    return result;
}
bool VerifyPE( LPCSTR filename,LPCWSTR arch,bool isdebug,LPCWSTR symname,LPCWSTR importtable)
{
       
    FILE* pFile;
    long lSize;
    char* buffer;
    size_t result;
    if (!PathFileExistsA(filename))return false;
    Version ver = GetFileVersionA(filename);
    if (isdebug != ver.isdebug)return false;

    fopen_s(&pFile, filename, "rb");
    if (pFile == NULL)
    {
        fputs("File error", stderr);
        return false;
    }

    fseek(pFile, 0, SEEK_END);
    lSize = ftell(pFile);
    rewind(pFile);

    buffer = (char*)malloc(sizeof(char) * lSize);
    if (buffer == NULL)
    {
        fputs("Memory error", stderr);
        return false;
    }
    result = fread(buffer, 1, lSize, pFile);
    if (result != lSize)
    {
        fputs("Reading error", stderr);
        return false;
    }

    if (buffer[0] != 'M' || buffer[1] != 'Z') return false;
    fclose(pFile);
    DWORD dwFileSize = 0;
    PCHAR pFileBuffer = buffer;
    PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)pFileBuffer;
    PIMAGE_NT_HEADERS pNth = (PIMAGE_NT_HEADERS)(pFileBuffer + pDos->e_lfanew);
    PIMAGE_FILE_HEADER pFil = (PIMAGE_FILE_HEADER) & (pNth->FileHeader);

    if (pFil->Machine == IMAGE_FILE_MACHINE_AMD64) {
        free(buffer);
        if (_wcsicmp(arch, L"amd64") != 0)return false;
    }
    else if (pFil->Machine == IMAGE_FILE_MACHINE_I386) {
        LPCWSTR realarch=L"x86";
        PIMAGE_SECTION_HEADER sectionHeader;
        DWORD ptr;
        UINT nSectionCount;
        UINT i;
        uintptr_t offset;
        UINT cbDebug = 0;
        PIMAGE_DEBUG_DIRECTORY imageDebugDirectory;
        PIMAGE_OPTIONAL_HEADER32 pOptionalHeader = (PIMAGE_OPTIONAL_HEADER32) & (pNth->OptionalHeader);

        cbDebug = pOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size;
        ptr = pOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
        sectionHeader = IMAGE_FIRST_SECTION(pNth);
        nSectionCount = pNth->FileHeader.NumberOfSections;
        for (i = 0; i <= nSectionCount; ++i, ++sectionHeader)
        {
            if ((sectionHeader->VirtualAddress) > ptr)
            {
                sectionHeader--;
                break;
            }
        }
        if (i > nSectionCount)
        {
            sectionHeader = IMAGE_FIRST_SECTION(pNth);
            UINT nSectionCount = pNth->FileHeader.NumberOfSections;
            for (i = 0; i < nSectionCount - 1; ++i, ++sectionHeader);
        }
        offset = ptr + (uintptr_t)sectionHeader->PointerToRawData + (uintptr_t)pFileBuffer - (uintptr_t)sectionHeader->VirtualAddress;
        while (cbDebug >= sizeof(IMAGE_DEBUG_DIRECTORY))
        {
            imageDebugDirectory = (PIMAGE_DEBUG_DIRECTORY)(offset);
            offset += sizeof(IMAGE_DEBUG_DIRECTORY);
            if (imageDebugDirectory->Type == IMAGE_DEBUG_TYPE_CODEVIEW)
            {
                LPSTR str = (LPSTR)((uintptr_t)pFileBuffer + imageDebugDirectory->PointerToRawData) + sizeof(DWORD) * 2 + sizeof(GUID);
                cbDebug -= (UINT)sizeof(IMAGE_DEBUG_DIRECTORY);
                std::wstring filename2 = symname;
                wchar_t verbuf[12];
                wsprintfW(verbuf, L"%d.%d", ver.VerMajor, ver.VerMinor);
                WCHAR buffer[200];
                mbstowcs_s(0, buffer, str, strlen(str));
                std::wstring symname = buffer;
                bool iswow64 = false;
                bool isarm64x86 = false;
                auto str1 = (symname.substr(0, symname.rfind(L'.')) + filename2.substr(filename2.rfind(L'.')));
                if (_wcsicmp(str1.c_str(), (std::wstring(L"w") + filename2).c_str()) == 0) iswow64 = true;
                if (_wcsicmp(str1.c_str(), (std::wstring(L"y") + filename2).c_str()) == 0) isarm64x86 = true;
                if (iswow64) realarch=L"wow64";
                else if (isarm64x86) realarch = L"arm64x86";
                else realarch = L"x86";
                wprintf(L"symname=%s str1=%s filename2=%s realarch=%s\n", symname.c_str(),str1.c_str(),filename2.c_str(),realarch);

                break;
            }
        }

        free(buffer);
        if (_wcsicmp(arch, realarch) != 0)return false;
    }
    else if (pFil->Machine == IMAGE_FILE_MACHINE_ARM ||
        pFil->Machine == IMAGE_FILE_MACHINE_THUMB ||
        pFil->Machine == IMAGE_FILE_MACHINE_ARMNT) {
        free(buffer);
        if (_wcsicmp(arch, L"arm") != 0)return false;
    }
#if _MSC_VER > 1700
    else if (pFil->Machine == IMAGE_FILE_MACHINE_ARM64) {
        free(buffer);
        if (_wcsicmp(arch, L"arm64") != 0)return false;
    }
#endif
    if (importtable != NULL && wcslen(importtable)!=0) {
        std::string realimporttables = GetImportDLLs(filename);
        std::string importtables = wstring2string(importtable);
        if (realimporttables != importtables)return false;
    }
    return true;
}
