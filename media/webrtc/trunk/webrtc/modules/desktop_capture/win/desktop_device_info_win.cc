



#include "webrtc/modules/desktop_capture/win/desktop_device_info_win.h"
#include "webrtc/modules/desktop_capture/win/win_shared.h"
#include <stdio.h>



BOOL WINAPI QueryFullProcessImageName(HANDLE hProcess, DWORD dwFlags, LPTSTR lpExeName, PDWORD lpdwSize);



DWORD WINAPI GetProcessImageFileName(HANDLE hProcess, LPTSTR lpImageFileName, DWORD nSize);

namespace webrtc {

DesktopDeviceInfo * DesktopDeviceInfoImpl::Create() {
  DesktopDeviceInfoWin * pDesktopDeviceInfo = new DesktopDeviceInfoWin();
  if(pDesktopDeviceInfo && pDesktopDeviceInfo->Init() != 0){
    delete pDesktopDeviceInfo;
    pDesktopDeviceInfo = NULL;
  }
  return pDesktopDeviceInfo;
}

DesktopDeviceInfoWin::DesktopDeviceInfoWin() {
}

DesktopDeviceInfoWin::~DesktopDeviceInfoWin() {
}

#if !defined(MULTI_MONITOR_SCREENSHARE)
void DesktopDeviceInfoWin::MultiMonitorScreenshare()
{
  DesktopDisplayDevice *pDesktopDeviceInfo = new DesktopDisplayDevice;
  if (pDesktopDeviceInfo) {
    pDesktopDeviceInfo->setScreenId(webrtc::kFullDesktopScreenId);
    pDesktopDeviceInfo->setDeviceName("Primary Monitor");

    char idStr[64];
    _snprintf_s(idStr, sizeof(idStr), sizeof(idStr) - 1, "%ld", pDesktopDeviceInfo->getScreenId());
    pDesktopDeviceInfo->setUniqueIdName(idStr);
    desktop_display_list_[pDesktopDeviceInfo->getScreenId()] = pDesktopDeviceInfo;
  }
}
#endif

void DesktopDeviceInfoWin::InitializeScreenList() {
#if !defined(MULTI_MONITOR_SCREENSHARE)
  MultiMonitorScreenshare();
#endif
}
void DesktopDeviceInfoWin::InitializeApplicationList() {
  
  HWND hWnd;
  for (hWnd = GetWindow(GetDesktopWindow(), GW_CHILD); hWnd; hWnd = GetWindow(hWnd, GW_HWNDNEXT)) {
    if (!IsWindowVisible(hWnd)) {
      continue;
    }

    DWORD dwProcessId = 0;
    GetWindowThreadProcessId(hWnd, &dwProcessId);

    
    if (dwProcessId == 0 || dwProcessId == GetCurrentProcessId()) {
      continue;
    }

    
    DesktopApplicationList::iterator itr = desktop_application_list_.find(dwProcessId);
    if (itr != desktop_application_list_.end()) {
      itr->second->setWindowCount(itr->second->getWindowCount() + 1);
      continue;
    }

    
    DesktopApplication *pDesktopApplication = new DesktopApplication;
    if (!pDesktopApplication) {
      continue;
    }

    
    pDesktopApplication->setProcessId(dwProcessId);
    
    pDesktopApplication->setWindowCount(1);

    
    WCHAR szFilePathName[MAX_PATH]={0};
    decltype(QueryFullProcessImageName) *lpfnQueryFullProcessImageNameProc =
      reinterpret_cast<decltype(QueryFullProcessImageName) *>(GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "QueryFullProcessImageNameW"));
    if (lpfnQueryFullProcessImageNameProc) {
      
      DWORD dwMaxSize = _MAX_PATH;
      HANDLE hWndPro = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, dwProcessId);
      if(hWndPro) {
        lpfnQueryFullProcessImageNameProc(hWndPro, 0, szFilePathName, &dwMaxSize);
        CloseHandle(hWndPro);
      }
    } else {
      HMODULE hModPSAPI = LoadLibrary(TEXT("PSAPI.dll"));
      if (hModPSAPI) {
        decltype(GetProcessImageFileName) *pfnGetProcessImageFileName =
          reinterpret_cast<decltype(GetProcessImageFileName) *>(GetProcAddress(hModPSAPI, "GetProcessImageFileNameW"));

        if (pfnGetProcessImageFileName) {
          HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, 0, dwProcessId);
          if (hProcess) {
            DWORD dwMaxSize = _MAX_PATH;
            pfnGetProcessImageFileName(hProcess, szFilePathName, dwMaxSize);
            CloseHandle(hProcess);
          }
        }
        FreeLibrary(hModPSAPI);
      }
    }
    pDesktopApplication->setProcessPathName(Utf16ToUtf8(szFilePathName).c_str());

    
    WCHAR szWndTitle[_MAX_PATH]={0};
    GetWindowText(hWnd, szWndTitle, MAX_PATH);
    if (lstrlen(szWndTitle) <= 0) {
      pDesktopApplication->setProcessAppName(Utf16ToUtf8(szFilePathName).c_str());
    } else {
      pDesktopApplication->setProcessAppName(Utf16ToUtf8(szWndTitle).c_str());
    }

    
    char idStr[64];
    _snprintf_s(idStr, sizeof(idStr), sizeof(idStr) - 1, "%ld", pDesktopApplication->getProcessId());
    pDesktopApplication->setUniqueIdName(idStr);

    desktop_application_list_[pDesktopApplication->getProcessId()] = pDesktopApplication;
  }

  
  DesktopApplicationList::iterator itr;
  for (itr = desktop_application_list_.begin(); itr != desktop_application_list_.end(); itr++) {
    DesktopApplication *pApp = itr->second;

    
    char nameStr[BUFSIZ];
    _snprintf_s(nameStr, sizeof(nameStr), sizeof(nameStr) - 1, "%d\x1e%s",
                pApp->getWindowCount(), pApp->getProcessAppName());
    pApp->setProcessAppName(nameStr);
  }
}

} 
