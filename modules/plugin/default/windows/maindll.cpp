




































#include <windows.h>

HINSTANCE hInst; 

BOOL WINAPI DllMain(HINSTANCE hDLL, DWORD dwReason, LPVOID lpReserved)
{
  hInst = hDLL;
  return TRUE;
}
