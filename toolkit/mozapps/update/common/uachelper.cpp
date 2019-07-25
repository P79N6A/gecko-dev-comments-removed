




































#include <windows.h>
#include "uachelper.h"
#include "updatelogging.h"

typedef BOOL (WINAPI *LPWTSQueryUserToken)(ULONG, PHANDLE);




LPCTSTR UACHelper::PrivsToDisable[] = { 
  SE_ASSIGNPRIMARYTOKEN_NAME,
  SE_AUDIT_NAME,
  SE_BACKUP_NAME,
  
  
  SE_CHANGE_NOTIFY_NAME,
  SE_CREATE_GLOBAL_NAME,
  SE_CREATE_PAGEFILE_NAME,
  SE_CREATE_PERMANENT_NAME,
  SE_CREATE_SYMBOLIC_LINK_NAME,
  SE_CREATE_TOKEN_NAME,
  SE_DEBUG_NAME,
  SE_ENABLE_DELEGATION_NAME,
  SE_IMPERSONATE_NAME,
  SE_INC_BASE_PRIORITY_NAME,
  SE_INCREASE_QUOTA_NAME,
  SE_INC_WORKING_SET_NAME,
  SE_LOAD_DRIVER_NAME,
  SE_LOCK_MEMORY_NAME,
  SE_MACHINE_ACCOUNT_NAME,
  SE_MANAGE_VOLUME_NAME,
  SE_PROF_SINGLE_PROCESS_NAME,
  SE_RELABEL_NAME,
  SE_REMOTE_SHUTDOWN_NAME,
  SE_RESTORE_NAME,
  SE_SECURITY_NAME,
  SE_SHUTDOWN_NAME,
  SE_SYNC_AGENT_NAME,
  SE_SYSTEM_ENVIRONMENT_NAME,
  SE_SYSTEM_PROFILE_NAME,
  SE_SYSTEMTIME_NAME,
  SE_TAKE_OWNERSHIP_NAME,
  SE_TCB_NAME,
  SE_TIME_ZONE_NAME,
  SE_TRUSTED_CREDMAN_ACCESS_NAME,
  SE_UNDOCK_NAME,
  SE_UNSOLICITED_INPUT_NAME
};






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










BOOL 
UACHelper::SetPrivilege(HANDLE token, LPCTSTR priv, BOOL enable)
{
  LUID luidOfPriv;
  if (!LookupPrivilegeValue(NULL, priv, &luidOfPriv)) {
    return FALSE; 
  }

  TOKEN_PRIVILEGES tokenPriv;
  tokenPriv.PrivilegeCount = 1;
  tokenPriv.Privileges[0].Luid = luidOfPriv;
  tokenPriv.Privileges[0].Attributes = enable ? SE_PRIVILEGE_ENABLED : 0;

  SetLastError(ERROR_SUCCESS);
  if (!AdjustTokenPrivileges(token, false, &tokenPriv,
                             sizeof(tokenPriv), NULL, NULL)) {
    return FALSE; 
  } 

  return GetLastError() == ERROR_SUCCESS;
}











BOOL
UACHelper::DisableUnneededPrivileges(HANDLE token, 
                                     LPCTSTR *unneededPrivs, 
                                     size_t count)
{
  HANDLE obtainedToken = NULL;
  if (!token) {
    
    HANDLE process = GetCurrentProcess();
    if (!OpenProcessToken(process, TOKEN_ALL_ACCESS_P, &obtainedToken)) {
      LOG(("Could not obtain token for current process, no "
           "privileges changed. (%d)\n", GetLastError()));
      return FALSE;
    }
    token = obtainedToken;
  }

  BOOL result = TRUE;
  for (size_t i = 0; i < count; i++) {
    if (SetPrivilege(token, unneededPrivs[i], FALSE)) {
      LOG(("Disabled unneeded token privilege: %s.\n", 
           unneededPrivs[i]));
    } else {
      LOG(("Could not disable token privilege value: %s. (%d)\n", 
           unneededPrivs[i], GetLastError()));
      result = FALSE;
    }
  }

  if (obtainedToken) {
    CloseHandle(obtainedToken);
  }
  return result;
}











BOOL
UACHelper::DisablePrivileges(HANDLE token)
{
  static const size_t PrivsToDisableSize = 
    sizeof(UACHelper::PrivsToDisable) / sizeof(UACHelper::PrivsToDisable[0]);

  return DisableUnneededPrivileges(token, UACHelper::PrivsToDisable, 
                                   PrivsToDisableSize);
}
