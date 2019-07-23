




































#include "xp.h"
#include <windowsx.h>

#include "windowsxx.h"

#include "resource.h"
#include "logger.h"

static void onCommand(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
  switch (id)
  {
    case IDC_BUTTON_CHECKALL:
    {
      for(int i = IDC_CHECK_NPN_VERSION; i < IDC_CHECK_NPN_VERSION + TOTAL_NUMBER_OF_API_CALLS - 1; i++)
        CheckDlgButton(hWnd, i, BST_CHECKED);
      break;
    }
    case IDC_BUTTON_CLEARALL:
    {
      for(int i = IDC_CHECK_NPN_VERSION; i < IDC_CHECK_NPN_VERSION + TOTAL_NUMBER_OF_API_CALLS - 1; i++)
        CheckDlgButton(hWnd, i, BST_UNCHECKED);
      break;
    }
    default:
      break;
  }
}

static void onApply(HWND hWnd)
{
  Logger * logger = (Logger *)GetWindowLong(hWnd, DWL_USER);
  if(!logger)
    return;

  BOOL mutedcalls[TOTAL_NUMBER_OF_API_CALLS];

  mutedcalls[0] = FALSE; 

  
  for(int i = IDC_CHECK_NPN_VERSION; i < IDC_CHECK_NPN_VERSION + TOTAL_NUMBER_OF_API_CALLS - 1; i++)
    mutedcalls[i - IDC_CHECK_NPN_VERSION + 1] = (BST_UNCHECKED == IsDlgButtonChecked(hWnd, i));

  logger->setMutedCalls(&mutedcalls[0]);
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
    SetWindowLong(hWnd, DWL_USER, (long)logger);
  }

  BOOL * mutedcalls = logger->getMutedCalls();

  if(mutedcalls)
  {
    
    for(int i = IDC_CHECK_NPN_VERSION; i < IDC_CHECK_NPN_VERSION + TOTAL_NUMBER_OF_API_CALLS - 1; i++)
      CheckDlgButton(hWnd, i, mutedcalls[i - IDC_CHECK_NPN_VERSION + 1] ? BST_UNCHECKED : BST_CHECKED);
  }
  else
  {
    for(int i = IDC_CHECK_NPN_VERSION; i < IDC_CHECK_NPN_VERSION + TOTAL_NUMBER_OF_API_CALLS - 1; i++)
      CheckDlgButton(hWnd, i, BST_CHECKED);
  }

  return TRUE;
}

BOOL CALLBACK FilterPageProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch(msg)
  {
    case WM_INITDIALOG:
      return (BOOL)HANDLE_WM_INITDIALOG(hWnd, wParam, lParam, onInitDialog);
    case WM_COMMAND:
      HANDLE_WM_COMMAND(hWnd, wParam, lParam, onCommand);
      break;
    case WM_NOTIFY:
      HANDLE_WM_NOTIFY(hWnd, wParam, lParam, onNotify);
      break;

    default:
      return FALSE;
  }
  return TRUE;
}
