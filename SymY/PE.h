#pragma once
#include<Windows.h>
bool VerifyPE( LPCSTR filename, LPCWSTR arch, bool isdebug,LPCWSTR symname, LPCWSTR importtable=NULL);
BOOL WritePrivateProfileInt(
    LPCTSTR lpAppName,
    LPCTSTR lpKeyName,
    INT Value,
    LPCTSTR lpFileName);