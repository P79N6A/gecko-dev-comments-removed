





































#pragma once
#include "resourceppc.h"

extern nsSetupStrings Strings;
extern const WCHAR c_sStringsFile[];

extern WCHAR g_sSourcePath[MAX_PATH];
extern WCHAR g_sExeFileName[MAX_PATH];
extern WCHAR g_sExeFullFileName[MAX_PATH];
extern DWORD g_dwExeSize;

#define ErrorMsg(msg) MessageBoxW(GetForegroundWindow(), msg, L"Setup", MB_OK|MB_ICONERROR)
bool ConvertToChar(const WCHAR *wstr, char *str);
