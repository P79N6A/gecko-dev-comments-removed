




































#include "xp.h"
#include <windowsx.h>

#include "resource.h"

static void onCommand(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
  switch (id)
  {
    case IDOK:
      EndDialog(hWnd, IDOK);
      break;
    case IDCANCEL:
      EndDialog(hWnd, IDCANCEL);
      break;
    default:
      break;
  }
}

static BOOL onInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam)
{
  SetWindowPos(hWnd, NULL, 0,0,0,0, SWP_NOZORDER);
  return TRUE;
}

BOOL CALLBACK PauseDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch(msg)
  {
    case WM_INITDIALOG:
      return (BOOL)HANDLE_WM_INITDIALOG(hWnd, wParam, lParam, onInitDialog);
    case WM_COMMAND:
      HANDLE_WM_COMMAND(hWnd, wParam, lParam, onCommand);
      break;
    case WM_CLOSE:
      EndDialog(hWnd, IDCANCEL);
      break;

    default:
      return FALSE;
  }
  return TRUE;
}
