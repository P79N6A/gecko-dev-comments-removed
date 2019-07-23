




































#include "xp.h"
#include <windowsx.h>

#include "windowsxx.h"

#include "resource.h"
#include "logger.h"
#include "dirpick.h"

static void onChooseDir(HWND hWnd)
{
  Logger * logger = (Logger *)GetWindowLong(hWnd, DWL_USER);
  if(!logger)
    return;

  char string[_MAX_PATH];
  char name[_MAX_PATH];

  GetDlgItemText(hWnd, IDC_EDIT_FILE, string, strlen(string));
  char * p = NULL;
  p = strrchr(string, '\\');
  if(p)
  {
    strcpy(name, p);
    *p = '\0';
  }

  if(PickupDirectory(hWnd, string))
  {
    
    if(string[strlen(string) - 1] == '\\')
      string[strlen(string) - 1] = '\0';

    if(p)
      strcat(string, name);
    else
    {
      strcat(string, "\\");
      strcat(string, DEFAULT_LOG_FILE_NAME);
    }
    SetDlgItemText(hWnd, IDC_EDIT_FILE, string);
  }
}

static void onCommand(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
  switch (id)
  {
    case IDC_CHECK_TOFILE:
      EnableWindow(GetDlgItem(hWnd, IDC_EDIT_FILE), (BST_CHECKED == IsDlgButtonChecked(hWnd, IDC_CHECK_TOFILE)));
      EnableWindow(GetDlgItem(hWnd, IDC_BUTTON_CHOOSEDIR), (BST_CHECKED == IsDlgButtonChecked(hWnd, IDC_CHECK_TOFILE)));
      break;
    case IDC_BUTTON_CHOOSEDIR:
      onChooseDir(hWnd);
      break;
    default:
      break;
  }
}

static void onApply(HWND hWnd)
{
  Logger * logger = (Logger *)GetWindowLong(hWnd, DWL_USER);
  if(!logger)
    return;

  logger->bToWindow = (BST_CHECKED == IsDlgButtonChecked(hWnd, IDC_CHECK_TOWINDOW));
  logger->bToConsole = (BST_CHECKED == IsDlgButtonChecked(hWnd, IDC_CHECK_TOCONSOLE));

  char filename[_MAX_PATH];
  GetDlgItemText(hWnd, IDC_EDIT_FILE, filename, strlen(filename));
  logger->setToFile(BST_CHECKED == IsDlgButtonChecked(hWnd, IDC_CHECK_TOFILE), filename);
  
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

  if(logger)
  {
    CheckDlgButton(hWnd, IDC_CHECK_TOWINDOW, logger->bToWindow ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hWnd, IDC_CHECK_TOCONSOLE, logger->bToConsole ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hWnd, IDC_CHECK_TOFILE, logger->bToFile ? BST_CHECKED : BST_UNCHECKED);

    SetDlgItemText(hWnd, IDC_EDIT_FILE, logger->szFile);
    EnableWindow(GetDlgItem(hWnd, IDC_EDIT_FILE), (BST_CHECKED == IsDlgButtonChecked(hWnd, IDC_CHECK_TOFILE)));
    EnableWindow(GetDlgItem(hWnd, IDC_BUTTON_CHOOSEDIR), (BST_CHECKED == IsDlgButtonChecked(hWnd, IDC_CHECK_TOFILE)));
  }

  return TRUE;
}

BOOL CALLBACK LogPageProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
