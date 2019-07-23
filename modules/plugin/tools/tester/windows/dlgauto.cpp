




































#include <windows.h>
#include <windowsx.h>
#include <assert.h>

#include "resource.h"

#include "guihlp.h"
#include "plugin.h"
#include "scripter.h"
#include "logger.h"
#include "xp.h"

extern CLogger * pLogger;
static CScripter * pScripter = NULL;

static void onCommand(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
  CPlugin * pPlugin = (CPlugin *)GetWindowLongPtr(hWnd, DWLP_USER);
  if(!pPlugin)
    return;

  switch (id)
  {
    case IDC_EDIT_SCRIPT_FILE_NAME:
    {
      if(codeNotify != EN_CHANGE)
        break;
      char szString[256];
      Edit_GetText(GetDlgItem(hWnd, IDC_EDIT_SCRIPT_FILE_NAME), szString, sizeof(szString));
      int iLen = strlen(szString);
      EnableWindow(GetDlgItem(hWnd, IDC_BUTTON_GO), (iLen > 0));
      if(iLen <= 0)
        break;

      
      char szLogFileName[256];
      char * p = strchr(szString, '.');

      if(p != NULL)
        *p = '\0';

      strcpy(szLogFileName, szString);
      strcat(szLogFileName, ".log");
      Edit_SetText(GetDlgItem(GetParent(hWnd), IDC_EDIT_LOG_FILE_NAME), szLogFileName);

      
      if(p)
        *p = '.';

      pPlugin->updatePrefs(gp_scriptfile, FALSE, szString);

      break;
    }
    







    case IDC_BUTTON_GO:
    {
      pLogger->blockDumpToFile(FALSE);

      
      
      
      EnableWindowNow(GetDlgItem(hWnd, IDC_BUTTON_GO), FALSE);

      EnableWindowNow(GetDlgItem(hWnd, IDC_EDIT_SCRIPT_FILE_NAME), FALSE);
      EnableWindowNow(GetDlgItem(GetParent(hWnd), IDC_EDIT_LOG_FILE_NAME), FALSE);
      EnableWindowNow(GetDlgItem(GetParent(hWnd), IDC_CHECK_SHOW_LOG), FALSE);
      EnableWindowNow(GetDlgItem(GetParent(hWnd), IDC_CHECK_LOG_TO_FRAME), FALSE);
      EnableWindowNow(GetDlgItem(GetParent(hWnd), IDC_CHECK_LOG_TO_FILE), FALSE);
      BOOL bFlashWasEnabled = FALSE;
      if(IsWindowEnabled(GetDlgItem(GetParent(hWnd), IDC_BUTTON_FLUSH)))
      {
        bFlashWasEnabled = TRUE;
        EnableWindowNow(GetDlgItem(GetParent(hWnd), IDC_BUTTON_FLUSH), FALSE);
      }
      EnableWindowNow(GetDlgItem(GetParent(hWnd), IDC_BUTTON_CLEAR), FALSE);
      EnableWindowNow(GetDlgItem(GetParent(hWnd), IDC_RADIO_MODE_MANUAL), FALSE);
      EnableWindowNow(GetDlgItem(GetParent(hWnd), IDC_RADIO_MODE_AUTO), FALSE);
    
      if(pScripter != NULL)
        delete pScripter;

      pScripter = new CScripter();
      pScripter->associate(pPlugin);

      char szFileName[_MAX_PATH];
      pPlugin ->getModulePath(szFileName, sizeof(szFileName));

      char szFile[_MAX_PATH];
      Edit_GetText(GetDlgItem(hWnd, IDC_EDIT_SCRIPT_FILE_NAME), szFile, sizeof(szFile));
      strcat(szFileName, szFile);

      if(pScripter->createScriptFromFile(szFileName))
      {
        int iRepetitions = pScripter->getCycleRepetitions();
        int iDelay = pScripter->getCycleDelay();
        if(iDelay < 0)
          iDelay = 0;

        assert(pLogger != NULL);
        pLogger->resetStartTime();

        ShowWindowNow(GetDlgItem(hWnd, IDC_STATIC_REPETITIONS_LABEL), SW_SHOW);
        char szLeft[256];

        for(int i = 0; i < iRepetitions; i++)
        {
          wsprintf(szLeft, "%i", iRepetitions - i);
          Static_SetText(GetDlgItem(hWnd, IDC_STATIC_REPETITIONS_LEFT), szLeft);
          pScripter->executeScript();
          






          if(iDelay != 0)
            XP_Sleep(iDelay);
        }
      }
      else
      {
        MessageBox(hWnd, "Script file not found or invalid", "", MB_OK | MB_ICONERROR);
      }

      delete pScripter;
      pScripter = NULL;

      Static_SetText(GetDlgItem(hWnd, IDC_STATIC_REPETITIONS_LEFT), "");
      ShowWindow(GetDlgItem(hWnd, IDC_STATIC_REPETITIONS_LABEL), SW_HIDE);

      
      
      EnableWindow(GetDlgItem(hWnd, IDC_BUTTON_GO), TRUE);

      EnableWindow(GetDlgItem(hWnd, IDC_EDIT_SCRIPT_FILE_NAME), TRUE);
      EnableWindow(GetDlgItem(GetParent(hWnd), IDC_EDIT_LOG_FILE_NAME), TRUE);
      EnableWindow(GetDlgItem(GetParent(hWnd), IDC_CHECK_SHOW_LOG), TRUE);
      EnableWindow(GetDlgItem(GetParent(hWnd), IDC_CHECK_LOG_TO_FRAME), TRUE);
      EnableWindow(GetDlgItem(GetParent(hWnd), IDC_CHECK_LOG_TO_FILE), TRUE);
      if(bFlashWasEnabled)
        EnableWindow(GetDlgItem(GetParent(hWnd), IDC_BUTTON_FLUSH), TRUE);
      EnableWindow(GetDlgItem(GetParent(hWnd), IDC_BUTTON_CLEAR), TRUE);
      EnableWindow(GetDlgItem(GetParent(hWnd), IDC_RADIO_MODE_MANUAL), TRUE);
      EnableWindow(GetDlgItem(GetParent(hWnd), IDC_RADIO_MODE_AUTO), TRUE);
      break;
    }
    default:
      break;
  }
}

static BOOL onInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam)
{
  CPlugin * pPlugin = (CPlugin *)lParam;
  SetWindowLongPtr(hWnd, DWLP_USER, (LONG_PTR)pPlugin);

  int iTopMargin = 188;
  SetWindowPos(hWnd, NULL, 0,iTopMargin, 0,0, SWP_NOZORDER | SWP_NOSIZE);

  if (pPlugin)
    Edit_SetText(GetDlgItem(hWnd, IDC_EDIT_SCRIPT_FILE_NAME), (LPCSTR)pPlugin->m_Pref_szScriptFile);

  Static_SetText(GetDlgItem(hWnd, IDC_STATIC_REPETITIONS_LEFT), "");
  ShowWindow(GetDlgItem(hWnd, IDC_STATIC_REPETITIONS_LABEL), SW_HIDE);

  ShowWindow(GetDlgItem(hWnd, IDC_BUTTON_GO), SW_SHOW);
  ShowWindow(GetDlgItem(hWnd, IDC_BUTTON_STOP), SW_HIDE);

  return TRUE;
}

static void onDestroy(HWND hWnd)
{
  if(pScripter != NULL)
  {
    delete pScripter;
    pScripter = NULL;
  }
}

BOOL CALLBACK NP_LOADDS AutoDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch(msg)
  {
    case WM_INITDIALOG:
      return (BOOL)HANDLE_WM_INITDIALOG(hWnd, wParam, lParam, onInitDialog);
    case WM_COMMAND:
      HANDLE_WM_COMMAND(hWnd, wParam, lParam, onCommand);
      break;
    case WM_DESTROY:
      HANDLE_WM_DESTROY(hWnd, wParam, lParam, onDestroy);
      break;

    default:
      return FALSE;
  }
  return TRUE;
}
