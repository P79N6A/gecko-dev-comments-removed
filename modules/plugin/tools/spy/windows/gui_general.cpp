




































#include "xp.h"
#include <windowsx.h>

#include "windowsxx.h"

#include "resource.h"
#include "logger.h"

static void onApply(HWND hWnd)
{
  Logger * logger = (Logger *)GetWindowLongPtr(hWnd, DWLP_USER);
  if(!logger)
    return;

  logger->bOnTop = (BST_CHECKED == IsDlgButtonChecked(hWnd, IDC_CHECK_ONTOP));
  logger->bSaveSettings = TRUE;
}

static void onNotify(HWND hWnd, int idCtrl, LPNMHDR lpNMHdr)
{
  switch(lpNMHdr->code)
  {
    case PSN_RESET:
      break;
    case PSN_APPLY:
      onApply(hWnd);
      break;
  }
}

static BOOL onInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam)
{
  Logger * logger = NULL;

  if(lParam)
  {
    logger = (Logger *)(((PROPSHEETPAGE *)lParam)->lParam);
    SetWindowLongPtr(hWnd, DWLP_USER, (LONG_PTR)logger);
  }

  if(logger)
  {
    CheckDlgButton(hWnd, IDC_CHECK_ONTOP, logger->bOnTop ? BST_CHECKED : BST_UNCHECKED);
  }

  return TRUE;
}

BOOL CALLBACK GeneralPageProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch(msg)
  {
    case WM_INITDIALOG:
      return (BOOL)HANDLE_WM_INITDIALOG(hWnd, wParam, lParam, onInitDialog);
    case WM_NOTIFY:
      HANDLE_WM_NOTIFY(hWnd, wParam, lParam, onNotify);
      break;

    default:
      return FALSE;
  }
  return TRUE;
}
