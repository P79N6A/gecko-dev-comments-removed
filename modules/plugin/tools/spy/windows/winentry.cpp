





































#include <windows.h>

char szAppName[] = "NPSpy";
HINSTANCE hInst = NULL;

BOOL WINAPI DllMain(HINSTANCE hDLL, DWORD dwReason, LPVOID lpReserved)
{
#ifdef DEBUG
  char szReason[80];

  switch (dwReason)
  {
    case DLL_PROCESS_ATTACH:
      strcpy(szReason, "DLL_PROCESS_ATTACH");
      break;
    case DLL_THREAD_ATTACH:
      strcpy(szReason, "DLL_THREAD_ATTACH");
      break;
    case DLL_PROCESS_DETACH:
      strcpy(szReason, "DLL_PROCESS_DETACH");
      break;
    case DLL_THREAD_DETACH:
      strcpy(szReason, "DLL_THREAD_DETACH");
      break;
  }
#endif

  hInst = hDLL;
  return TRUE;
}
