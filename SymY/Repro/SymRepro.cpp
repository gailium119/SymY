// ReproducibleTool.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <windows.h>
#include "PEInfo.h"
#include <curl/curl.h>
#include "SymRepro.h"
#include "../SymDL.h"
#include "../Download.h"
#include "../PE.h"
extern "C" CURLcode Curl_sha256it(unsigned char* outbuffer, const unsigned char* input,
    const size_t len);
#pragma warning(disable:4996)
INT VnGetSymbolGUID(
    HMODULE hModule,
    char* dllName,
    LPGUID guid
)
{
    HANDLE hFile;
    HANDLE hFileMapping;
    LPVOID lpFileBase;
    PBYTE baseImage;
    PIMAGE_DOS_HEADER dosHeader;
#ifdef _WIN64
    PIMAGE_NT_HEADERS64 ntHeader;
#else
    PIMAGE_NT_HEADERS32 ntHeader;
#endif
    PIMAGE_SECTION_HEADER sectionHeader;
    DWORD ptr;
    UINT nSectionCount;
    UINT i;
    uintptr_t offset;
    UINT cbDebug = 0;
    PIMAGE_DEBUG_DIRECTORY imageDebugDirectory;
    PdbInfo* pdb_info = NULL;

    hFile = CreateFileA(
        dllName,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        0
    );
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return 1;
    }

    hFileMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (hFileMapping == 0)
    {
        CloseHandle(hFile);
        return 2;
    }

    lpFileBase = MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);
    if (lpFileBase == 0)
    {
        CloseHandle(hFileMapping);
        CloseHandle(hFile);
        return 3;
    }

    baseImage = (PBYTE)lpFileBase;
    dosHeader = (PIMAGE_DOS_HEADER)lpFileBase;
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
    {
        UnmapViewOfFile(lpFileBase);
        CloseHandle(hFileMapping);
        CloseHandle(hFile);
        return 4;
    }

#ifdef _WIN64
    ntHeader = (PIMAGE_NT_HEADERS64)((u_char*)dosHeader + dosHeader->e_lfanew);
#else
    ntHeader = (PIMAGE_NT_HEADERS32)((u_char*)dosHeader + dosHeader->e_lfanew);
#endif
    if (ntHeader->Signature != IMAGE_NT_SIGNATURE)
    {
        UnmapViewOfFile(lpFileBase);
        CloseHandle(hFileMapping);
        CloseHandle(hFile);
        return 5;
    }
    if (ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress == 0)
    {
        UnmapViewOfFile(lpFileBase);
        CloseHandle(hFileMapping);
        CloseHandle(hFile);
        return 6;
    }
    cbDebug = ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size;
    ptr = ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
    sectionHeader = IMAGE_FIRST_SECTION(ntHeader);
    nSectionCount = ntHeader->FileHeader.NumberOfSections;
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
        sectionHeader = IMAGE_FIRST_SECTION(ntHeader);
        UINT nSectionCount = ntHeader->FileHeader.NumberOfSections;
        for (i = 0; i < nSectionCount - 1; ++i, ++sectionHeader);
    }
    offset = (uintptr_t)baseImage + ptr + (uintptr_t)sectionHeader->PointerToRawData - (uintptr_t)sectionHeader->VirtualAddress;
    while (cbDebug >= sizeof(IMAGE_DEBUG_DIRECTORY))
    {
        imageDebugDirectory = (PIMAGE_DEBUG_DIRECTORY)(offset);
        offset += sizeof(IMAGE_DEBUG_DIRECTORY);
        if (imageDebugDirectory->Type == IMAGE_DEBUG_TYPE_CODEVIEW)
        {
            pdb_info = (PdbInfo*)((uintptr_t)baseImage + imageDebugDirectory->PointerToRawData);
        }
        cbDebug -= (UINT)sizeof(IMAGE_DEBUG_DIRECTORY);
    }
    if (pdb_info == NULL)
    {
        UnmapViewOfFile(lpFileBase);
        CloseHandle(hFileMapping);
        CloseHandle(hFile);
        return 7;
    }

    *guid = pdb_info->Guid;
    UnmapViewOfFile(lpFileBase);
    CloseHandle(hFileMapping);
    CloseHandle(hFile);
    return 0;
}

INT VnGetReproHash(
    HMODULE hModule,
    const char* dllName,
    char* hash
)
{
    HANDLE hFile;
    HANDLE hFileMapping;
    LPVOID lpFileBase;
    PBYTE baseImage;
    PIMAGE_DOS_HEADER dosHeader;
#ifdef _WIN64
    PIMAGE_NT_HEADERS64 ntHeader;
#else
    PIMAGE_NT_HEADERS32 ntHeader;
#endif
    PIMAGE_SECTION_HEADER sectionHeader;
    DWORD ptr;
    UINT nSectionCount;
    UINT i;
    uintptr_t offset;
    UINT cbDebug = 0;
    PIMAGE_DEBUG_DIRECTORY imageDebugDirectory;
    ReproInfo* repro_info = NULL;

    hFile = CreateFileA(
        dllName,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        0
    );
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return 1;
    }

    hFileMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (hFileMapping == 0)
    {
        CloseHandle(hFile);
        return 2;
    }

    lpFileBase = MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);
    if (lpFileBase == 0)
    {
        CloseHandle(hFileMapping);
        CloseHandle(hFile);
        return 3;
    }

    baseImage = (PBYTE)lpFileBase;
    dosHeader = (PIMAGE_DOS_HEADER)lpFileBase;
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
    {
        UnmapViewOfFile(lpFileBase);
        CloseHandle(hFileMapping);
        CloseHandle(hFile);
        return 4;
    }

#ifdef _WIN64
    ntHeader = (PIMAGE_NT_HEADERS64)((u_char*)dosHeader + dosHeader->e_lfanew);
#else
    ntHeader = (PIMAGE_NT_HEADERS32)((u_char*)dosHeader + dosHeader->e_lfanew);
#endif
    if (ntHeader->Signature != IMAGE_NT_SIGNATURE)
    {
        UnmapViewOfFile(lpFileBase);
        CloseHandle(hFileMapping);
        CloseHandle(hFile);
        return 5;
    }
    if (ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress == 0)
    {
        UnmapViewOfFile(lpFileBase);
        CloseHandle(hFileMapping);
        CloseHandle(hFile);
        return 6;
    }
    cbDebug = ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size;
    ptr = ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
    sectionHeader = IMAGE_FIRST_SECTION(ntHeader);
    nSectionCount = ntHeader->FileHeader.NumberOfSections;
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
        sectionHeader = IMAGE_FIRST_SECTION(ntHeader);
        UINT nSectionCount = ntHeader->FileHeader.NumberOfSections;
        for (i = 0; i < nSectionCount - 1; ++i, ++sectionHeader);
    }
    offset = (uintptr_t)baseImage + ptr + (uintptr_t)sectionHeader->PointerToRawData - (uintptr_t)sectionHeader->VirtualAddress;
    while (cbDebug >= sizeof(IMAGE_DEBUG_DIRECTORY))
    {
        imageDebugDirectory = (PIMAGE_DEBUG_DIRECTORY)(offset);
        offset += sizeof(IMAGE_DEBUG_DIRECTORY);
        if (imageDebugDirectory->Type == IMAGE_DEBUG_TYPE_REPRO)
        {
            repro_info = (ReproInfo*)((uintptr_t)baseImage + imageDebugDirectory->PointerToRawData);
        }
        cbDebug -= (UINT)sizeof(IMAGE_DEBUG_DIRECTORY);
    }
    if (repro_info == NULL)
    {
        UnmapViewOfFile(lpFileBase);
        CloseHandle(hFileMapping);
        CloseHandle(hFile);
        return 7;
    }

    memcpy(hash, repro_info->SHA256hash, 32);
    UnmapViewOfFile(lpFileBase);
    CloseHandle(hFileMapping);
    CloseHandle(hFile);
    return 0;
}

size_t LoadFileToMem(const char* filename, void** buffer) {
    FILE* pFile;
    long lSize;
    size_t result;
    pFile = fopen(filename, "rb");
    if (pFile == NULL)
    {
        fputs("File error", stderr);
        return 0;
    }
    fseek(pFile, 0, SEEK_END);
    lSize = ftell(pFile);
    rewind(pFile);
    *buffer = (char*)malloc(sizeof(char) * lSize);
    if (*buffer == NULL)
    {
        fputs("Memory error", stderr);
        return 0;
    }
    result = fread(*buffer, 1, lSize, pFile);
    if (result != lSize)
    {
        fputs("Reading error", stderr);
        return 0;
    }
    fclose(pFile);
    return result;
}

DWORD RVAtoFOA(DWORD dwRVA, PBYTE pBuf)
{
    //找到导出位置，数据目录表的第一项（下标0）
    PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)pBuf;
    //NT头
    PIMAGE_NT_HEADERS pNt =
        (PIMAGE_NT_HEADERS)
        (pDos->e_lfanew + pBuf);
    //区段表首地址
    PIMAGE_SECTION_HEADER pSec = IMAGE_FIRST_SECTION(pNt);
    //区段表中的个数
    DWORD dwCount = pNt->FileHeader.NumberOfSections;
    for (int i = 0; i < dwCount; i++)
    {
        if (dwRVA >= pSec->VirtualAddress &&
            dwRVA < (pSec->VirtualAddress + pSec->SizeOfRawData))
        {
            return dwRVA -
                pSec->VirtualAddress + pSec->PointerToRawData;
        }
        //下一个区段
        pSec++;
    }
    return 0;
}
size_t WriteMemToFile(const char* filename, void** buffer, size_t size) {
    FILE* pFile;
    size_t result;
    pFile = fopen(filename, "wb");
    if (pFile == NULL)
    {
        fputs("File error", stderr);
        return 0;
    }
    if (*buffer == NULL)
    {
        fputs("Memory error", stderr);
        return 0;
    }
    result = fwrite(*buffer, 1, size, pFile);
    if (result != size)
    {
        fputs("Writing error", stderr);
        return 0;
    }
    fclose(pFile);
    return result;
}


BYTE* search_binary_sequence(BYTE* buffer, size_t buffer_size, BYTE* pattern, size_t pattern_len) {
    size_t i, j;
    for (i = 0; i <= buffer_size - pattern_len; ++i) {
        // 检查当前位置开始的子序列是否与pattern匹配
        for (j = 0; j < pattern_len; ++j) {
            if (buffer[i + j] != pattern[j])
                break;
        }
        if (j == pattern_len) {
            //printf("Pattern found at offset %zu\n", i);
            return buffer + i;
        }
    }
    return 0;
}
struct FileVersionMem {
    WORD MinorVersion;
    WORD MajorVersion;
    WORD UBR;
    WORD BuildBumber;
};
void DownSymRepro(CurlSelectMulti* m_multServer, LPCSTR symname, DWORD imagesize, void* PEBuffer, size_t PESize, LPCWSTR outputdir, Version originalVersion, DWORD minVersion, DWORD maxVersion) {
    FileVersionMem fVm = { originalVersion.VerMinor,originalVersion.VerMajor,originalVersion.VerUbr,originalVersion.VerFull };
    FileVersionMem* pFileVersion = (FileVersionMem*)search_binary_sequence((BYTE*)PEBuffer, PESize, (BYTE*) & fVm, sizeof(FileVersionMem));
    FileVersionMem* pProductVersion = pFileVersion + 1;
    WCHAR versionBuf[20];
    swprintf_s(versionBuf, L"%d.%d.%04d.%d", originalVersion.VerMajor, originalVersion.VerMinor, originalVersion.VerFull, originalVersion.VerUbr);
    BYTE* ver1= search_binary_sequence((BYTE*)PEBuffer, PESize, (BYTE*)versionBuf, (wcslen(versionBuf))*sizeof(WCHAR));
    BYTE* ver2 = search_binary_sequence((BYTE*)ver1+1, PESize-(ver1-PEBuffer+1), (BYTE*)versionBuf, (wcslen(versionBuf)) * sizeof(WCHAR));
    int taskid = 0;
    for (DWORD startVersion = minVersion; startVersion <= maxVersion; startVersion++) {
        pFileVersion->BuildBumber = startVersion;
        pProductVersion->BuildBumber = startVersion;
        swprintf_s(versionBuf, L"%d.%d.%04d.%d", originalVersion.VerMajor, originalVersion.VerMinor, startVersion, originalVersion.VerUbr);
        memcpy(ver1, versionBuf, (wcslen(versionBuf)) * sizeof(WCHAR));
        memcpy(ver2, versionBuf, (wcslen(versionBuf)) * sizeof(WCHAR));
        char calculatedhash[32] = {};
        Curl_sha256it((unsigned char*)calculatedhash, (BYTE*)PEBuffer, PESize);
        shared_ptr<Task> task = std::make_shared<Task>();
        task->iType = HTTPS_GET;
        char buffer[260] = "";
        sprintf_s(buffer, "https://msdl.microsoft.com/download/symbols/%s/%08x%x/%s", symname, *(DWORD*)&calculatedhash[28], (unsigned int)imagesize, symname);
        task->strUrl = buffer;

        taskid++;
        task->taskid = taskid;
        task->headers = NULL; // Fix 0xcdcdcdcd
        m_multServer->addTask(task);
    }
    Sleep(4000);
    while (m_multServer->m_mapRunningTask.size() > 0) {
        Sleep(1000);
    }
}
bool ValidateReproHashAndLoad(const char* filename, LPBYTE & buffer, DWORD & filesize)
{
    GUID guid = {};
    char hash[32] = {};
    VnGetReproHash(0, filename, hash);
    buffer = NULL;
    filesize = LoadFileToMem(filename, (void**)&buffer);
    GeneralizePE(buffer, filesize, FALSE);
    char calculatedhash[32] = {};
    Curl_sha256it((unsigned char*)calculatedhash, buffer, filesize);
    if (!memcmp(calculatedhash, hash, 32))
        return true;
    WriteMemToFile("D:\\notepad2.exe", (void**)&buffer, filesize);
    return false;
}
__inline const char* GetFileName(const char* filePath) {
    const char* fileName = strrchr(filePath, '\\');
    if (fileName) {
        return fileName + 1;  // 跳过 '/' 字符
    }
    return filePath;
}
bool DownSymRepro(CurlSelectMulti* m_multServer, const char* filename, LPCWSTR outputDir, DWORD minVersion, DWORD maxVersion) {
    LPBYTE buffer = NULL;
    DWORD filesize = 0;
    if (!ValidateReproHashAndLoad(filename, buffer, filesize)) {
        return false;
    }

    IMAGE_DOS_HEADER* pDosHeader = (IMAGE_DOS_HEADER*)buffer;
    IMAGE_NT_HEADERS* pNtHeaders = (IMAGE_NT_HEADERS*)((BYTE*)buffer + pDosHeader->e_lfanew);

    DWORD imagesize =  pNtHeaders->OptionalHeader.SizeOfImage;
    Version originalVersion = GetFileVersionA(filename);
    DownSymRepro(m_multServer, GetFileName(filename), imagesize, buffer, filesize, outputDir, originalVersion,minVersion, maxVersion);
    for (auto iter = m_multServer->m_foundFiles.begin(); iter != m_multServer->m_foundFiles.end(); iter++) {
        std::string url = *iter;

        printf("%s\n", url.c_str());

        std::string outputpath = url.substr(url.rfind("/") + 1);
        std::string tmp = url.substr(0, url.rfind("/"));
        outputpath = std::string(".\\").append(tmp.substr(tmp.rfind("/") + 1)).append("_").append(outputpath);

        DownloadFileWithCurl(url, outputpath);
    }
}