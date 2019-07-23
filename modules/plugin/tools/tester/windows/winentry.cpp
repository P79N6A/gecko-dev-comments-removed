









































#include <windows.h>

#include "xp.h"
#include "guiprefs.h"
#include "loadstatus.h"
#include "winutils.h"

HINSTANCE hInst = NULL;
HWND hWndLoadStatus = NULL;

BOOL WINAPI DllMain(HINSTANCE hDLL, DWORD dwReason, LPVOID lpReserved)
{
  hInst = hDLL;

  switch (dwReason) {
    case DLL_PROCESS_ATTACH:
    {
      char szFileName[_MAX_PATH];
      GetINIFileName(hInst, szFileName, sizeof(szFileName));
      char sz[256];
      XP_GetPrivateProfileString(SECTION_PREFERENCES, KEY_LOADSTATUS_WINDOW, ENTRY_NO, sz, sizeof(sz), szFileName);
      if (strcmpi(sz, ENTRY_YES) == 0)
        hWndLoadStatus = ShowLoadStatus("Tester dll is loaded");
      break;
    }
    case DLL_PROCESS_DETACH:
      if (hWndLoadStatus)
        DestroyLoadStatus(hWndLoadStatus);
      break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
      break;
    default:
      break;
  }
  return TRUE;
}
