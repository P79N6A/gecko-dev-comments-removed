




































#include <windows.h>
#include "uachelper.h"

typedef BOOL (WINAPI *LPWTSQueryUserToken)(ULONG, PHANDLE);






BOOL
UACHelper::IsVistaOrLater() 
{
  
  OSVERSIONINFO osInfo;
  osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  return GetVersionEx(&osInfo) && osInfo.dwMajorVersion >= 6;
}








HANDLE
UACHelper::OpenUserToken(DWORD sessionID)
{
  HMODULE module = LoadLibraryW(L"wtsapi32.dll");
  HANDLE token = NULL;
  LPWTSQueryUserToken wtsQueryUserToken = 
    (LPWTSQueryUserToken)GetProcAddress(module, "WTSQueryUserToken");
  if (wtsQueryUserToken) {
    wtsQueryUserToken(sessionID, &token);
  }
  FreeLibrary(module);
  return token;
}








HANDLE
UACHelper::OpenLinkedToken(HANDLE token) 
{
  
  
  
  
  TOKEN_LINKED_TOKEN tlt;
  HANDLE hNewLinkedToken = NULL;
  DWORD len;
  if (GetTokenInformation(token, (TOKEN_INFORMATION_CLASS)TokenLinkedToken, 
                          &tlt, sizeof(TOKEN_LINKED_TOKEN), &len)) {
    token = tlt.LinkedToken;
    hNewLinkedToken = token;
  }
  return hNewLinkedToken;
}
