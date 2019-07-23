




































#define INCL_WIN
#define INCL_GPI
#include <os2.h>
#include <string.h>

extern HMODULE hInst;
static char szClassName[] = "LoadStatusWindowClass";

HWND ShowLoadStatus(char * aMessage)
{
  if (!aMessage)
    return NULL;

  if (!WinRegisterClass((HAB)0, szClassName, (PFNWP)WinDefWindowProc, 0, sizeof(ULONG))) {
    return NULL;
  }
  
  HWND hWnd = WinCreateWindow(HWND_DESKTOP, szClassName, "", WS_VISIBLE | WS_DISABLED, 
                              0, 0, 0, 0, HWND_DESKTOP, HWND_TOP, 255, (PVOID)NULL, NULL);
  if (!hWnd) {
    ERRORID error = WinGetLastError(0);
    return NULL;
  }
  
  HPS hPS = WinGetPS(hWnd);
  if (!hPS) {
    WinDestroyWindow(hWnd);
    return NULL;
  }

  POINTL ptls;
  GpiQueryTextBox(hPS, strlen(aMessage), aMessage, 1, &ptls);
  WinSetWindowPos(hWnd, HWND_TOP, 0, 0, ptls.x + 4, ptls.y + 2, SWP_SHOW );
  POINTL ptlStart;
  ptlStart.x = 2; ptlStart.y = 1;
  GpiCharStringAt(hPS, &ptlStart, strlen(aMessage), aMessage);
  WinReleasePS(hPS);

  return hWnd;
}

void DestroyLoadStatus(HWND ahWnd)
{
  if (ahWnd)
    WinDestroyWindow(ahWnd);

}
