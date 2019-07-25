




































#include <windows.h>
#include "../../../../toolkit/components/maintenanceservice/pathhash.h"

#pragma comment(lib, "advapi32.lib") 

typedef struct _stack_t {
  struct _stack_t *next;
  TCHAR text[MAX_PATH];
} stack_t;

int popstring(stack_t **stacktop, LPTSTR str, int len);
void pushstring(stack_t **stacktop, LPCTSTR str, int len);








static BOOL
IsServiceInstalled(LPCWSTR serviceName, BOOL &exists)
{
  exists = FALSE;

  
  SC_HANDLE serviceManager = OpenSCManager(NULL, NULL, 
                                           SC_MANAGER_ENUMERATE_SERVICE);
  if (!serviceManager) {
    return FALSE;
  }

  SC_HANDLE serviceHandle = OpenServiceW(serviceManager, 
                                         serviceName, 
                                         SERVICE_QUERY_CONFIG);
  if (!serviceHandle && GetLastError() != ERROR_SERVICE_DOES_NOT_EXIST) {
    CloseServiceHandle(serviceManager);
    return FALSE;
  }
 
  if (serviceHandle) {
    CloseServiceHandle(serviceHandle);
    exists = TRUE;
  } 

  CloseServiceHandle(serviceManager);
  return TRUE;
}










extern "C" void __declspec(dllexport)
IsInstalled(HWND hwndParent, int string_size, 
            TCHAR *variables, stack_t **stacktop, void *extra)
{
  TCHAR tmp[MAX_PATH] = { L'\0' };
  WCHAR serviceName[MAX_PATH] = { '\0' };
  popstring(stacktop, tmp, MAX_PATH);

#if !defined(UNICODE)
    MultiByteToWideChar(CP_ACP, 0, tmp, -1, serviceName, MAX_PATH);
#else
    wcscpy(serviceName, tmp);
#endif

  BOOL serviceInstalled;
  if (!IsServiceInstalled(serviceName, serviceInstalled)) {
    pushstring(stacktop, TEXT("-1"), 3);
  } else {
    pushstring(stacktop, serviceInstalled ? TEXT("1") : TEXT("0"), 2);
  }
}







static BOOL
StopService(LPCWSTR serviceName)
{
  
  SC_HANDLE serviceManager = OpenSCManager(NULL, NULL, 
                                           SC_MANAGER_ENUMERATE_SERVICE);
  if (!serviceManager) {
    return FALSE;
  }

  SC_HANDLE serviceHandle = OpenServiceW(serviceManager, 
                                         serviceName, 
                                         SERVICE_STOP);
  if (!serviceHandle) {
    CloseServiceHandle(serviceManager);
    return FALSE;
  }

  
  
  DWORD totalWaitTime = 0;
  SERVICE_STATUS status;
  static const int maxWaitTime = 1000 * 60; 
  BOOL stopped = FALSE;
  if (ControlService(serviceHandle, SERVICE_CONTROL_STOP, &status)) {
    do {
      Sleep(status.dwWaitHint);
      
      totalWaitTime += (status.dwWaitHint + 10);
      if (status.dwCurrentState == SERVICE_STOPPED) {
        stopped = true;
        break;
      } else if (totalWaitTime > maxWaitTime) {
        break;
      }
    } while (QueryServiceStatus(serviceHandle, &status));
  }

  CloseServiceHandle(serviceHandle);
  CloseServiceHandle(serviceManager);
  return stopped;
}








extern "C" void __declspec(dllexport)
Stop(HWND hwndParent, int string_size, 
     TCHAR *variables, stack_t **stacktop, void *extra)
{
  TCHAR tmp[MAX_PATH] = { L'\0' };
  WCHAR serviceName[MAX_PATH] = { '\0' };

  popstring(stacktop, tmp, MAX_PATH);

#if !defined(UNICODE)
    MultiByteToWideChar(CP_ACP, 0, tmp, -1, serviceName, MAX_PATH);
#else
    wcscpy(serviceName, tmp);
#endif

  if (StopService(serviceName)) {
    pushstring(stacktop, TEXT("1"), 2);
  } else {
    pushstring(stacktop, TEXT("0"), 2);
  }
}








extern "C" void __declspec(dllexport)
PathToUniqueRegistryPath(HWND hwndParent, int string_size, 
                         TCHAR *variables, stack_t **stacktop, 
                         void *extra)
{
  TCHAR tmp[MAX_PATH] = { L'\0' };
  WCHAR installBasePath[MAX_PATH] = { '\0' };
  popstring(stacktop, tmp, MAX_PATH);

#if !defined(UNICODE)
    MultiByteToWideChar(CP_ACP, 0, tmp, -1, installBasePath, MAX_PATH);
#else
    wcscpy(installBasePath, tmp);
#endif

  WCHAR registryPath[MAX_PATH + 1] = { '\0' };
  if (CalculateRegistryPathFromFilePath(installBasePath, registryPath)) {
    pushstring(stacktop, registryPath, wcslen(registryPath) + 1);
  } else {
    pushstring(stacktop, TEXT(""), 1);
  }
}

BOOL WINAPI 
DllMain(HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
{
  return TRUE;
}









int popstring(stack_t **stacktop, TCHAR *str, int len)
{
  
  stack_t *th;
  if (!stacktop || !*stacktop) {
    return 1;
  }

  th = (*stacktop);
  lstrcpyn(str,th->text, len);
  *stacktop = th->next;
  GlobalFree((HGLOBAL)th);
  return 0;
}









void pushstring(stack_t **stacktop, const TCHAR *str, int len)
{
  stack_t *th;
  if (!stacktop) { 
    return;
  }

  th = (stack_t*)GlobalAlloc(GPTR, sizeof(stack_t) + len);
  lstrcpyn(th->text, str, len);
  th->next = *stacktop;
  *stacktop = th;
}
