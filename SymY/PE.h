#pragma once
#include<Windows.h>
bool VerifyPE( LPCSTR filename, LPCWSTR arch, bool isdebug,LPCWSTR symname, LPCWSTR importtable=NULL);
BOOL WritePrivateProfileInt(
    LPCTSTR lpAppName,
    LPCTSTR lpKeyName,
    INT Value,
    LPCTSTR lpFileName);
struct Version {
    int VerMajor;
    int VerMinor;
    int VerFull;
    int VerUbr;
    bool isdebug;
    Version() {
        VerMajor = 0;
        VerMinor = 0;
        VerFull = 0;
        VerUbr = 0;
        isdebug = false;
    }
};
Version GetFileVersionA(LPCSTR filename);