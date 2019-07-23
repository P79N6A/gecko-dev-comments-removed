




































#define INCL_WIN
#include <os2.h>
#include <assert.h>

#include "resource.h"

#include "guihlp.h"
#include "plugin.h"
#include "scripter.h"
#include "logger.h"
#include "xp.h"

extern CLogger * pLogger;
static CScripter * pScripter = NULL;

static void onCommand(HWND hWnd, int id, HWND hWndCtl, USHORT codeNotify)
{
  CPlugin * pPlugin = (CPlugin *)WinQueryWindowULong(hWnd, QWL_USER);
  if(!pPlugin)
    return;

  switch (id)
  {
    case IDC_EDIT_SCRIPT_FILE_NAME:
    {
      if(codeNotify != EN_CHANGE)
        break;
      char szString[256];
      WinQueryWindowText(WinWindowFromID(hWnd, IDC_EDIT_SCRIPT_FILE_NAME), sizeof(szString), szString);
      int iLen = strlen(szString);
      WinEnableWindow(WinWindowFromID(hWnd, IDC_BUTTON_GO), (iLen > 0));
      if(iLen <= 0)
        break;

      
      char szLogFileName[256];
      char * p = strchr(szString, '.');

      if(p != NULL)
        *p = '\0';

      strcpy(szLogFileName, szString);
      strcat(szLogFileName, ".log");
      WinSetWindowText(WinWindowFromID(WinQueryWindow(hWnd, QW_PARENT), IDC_EDIT_LOG_FILE_NAME), szLogFileName);

      
      if(p)
        *p = '.';

      pPlugin->updatePrefs(gp_scriptfile, FALSE, szString);

      break;
    }
    







    case IDC_BUTTON_GO:
    {
      pLogger->blockDumpToFile(FALSE);

      
      
      
      EnableWindowNow(WinWindowFromID(hWnd, IDC_BUTTON_GO), FALSE);

      EnableWindowNow(WinWindowFromID(hWnd, IDC_EDIT_SCRIPT_FILE_NAME), FALSE);
      EnableWindowNow(WinWindowFromID(WinQueryWindow(hWnd, QW_PARENT), IDC_EDIT_LOG_FILE_NAME), FALSE);
      EnableWindowNow(WinWindowFromID(WinQueryWindow(hWnd, QW_PARENT), IDC_CHECK_SHOW_LOG), FALSE);
      EnableWindowNow(WinWindowFromID(WinQueryWindow(hWnd, QW_PARENT), IDC_CHECK_LOG_TO_FRAME), FALSE);
      EnableWindowNow(WinWindowFromID(WinQueryWindow(hWnd, QW_PARENT), IDC_CHECK_LOG_TO_FILE), FALSE);
      BOOL bFlashWasEnabled = FALSE;
      if(WinIsWindowEnabled(WinWindowFromID(WinQueryWindow(hWnd, QW_PARENT), IDC_BUTTON_FLUSH)))
      {
        bFlashWasEnabled = TRUE;
        EnableWindowNow(WinWindowFromID(WinQueryWindow(hWnd, QW_PARENT), IDC_BUTTON_FLUSH), FALSE);
      }
      EnableWindowNow(WinWindowFromID(WinQueryWindow(hWnd, QW_PARENT), IDC_BUTTON_CLEAR), FALSE);
      EnableWindowNow(WinWindowFromID(WinQueryWindow(hWnd, QW_PARENT), IDC_RADIO_MODE_MANUAL), FALSE);
      EnableWindowNow(WinWindowFromID(WinQueryWindow(hWnd, QW_PARENT), IDC_RADIO_MODE_AUTO), FALSE);
    
      if(pScripter != NULL)
        delete pScripter;

      pScripter = new CScripter();
      pScripter->associate(pPlugin);

      char szFileName[_MAX_PATH];
      pPlugin ->getModulePath(szFileName, sizeof(szFileName));

      char szFile[_MAX_PATH];
      WinQueryWindowText(WinWindowFromID(hWnd, IDC_EDIT_SCRIPT_FILE_NAME), sizeof(szFile), szFile);
      strcat(szFileName, szFile);

      if(pScripter->createScriptFromFile(szFileName))
      {
        int iRepetitions = pScripter->getCycleRepetitions();
        int iDelay = pScripter->getCycleDelay();
        if(iDelay < 0)
          iDelay = 0;

        assert(pLogger != NULL);
        pLogger->resetStartTime();

        ShowWindowNow(WinWindowFromID(hWnd, IDC_STATIC_REPETITIONS_LABEL), TRUE);
        char szLeft[256];

        for(int i = 0; i < iRepetitions; i++)
        {
          sprintf(szLeft, "%i", iRepetitions - i);
          WinSetWindowText(WinWindowFromID(hWnd, IDC_STATIC_REPETITIONS_LEFT), szLeft);
          pScripter->executeScript();
          






          if(iDelay != 0)
            XP_Sleep(iDelay);
        }
      }
      else
      {
        WinMessageBox(HWND_DESKTOP, hWnd, "Script file not found or invalid", "", 0, MB_OK | MB_ERROR);
      }

      delete pScripter;
      pScripter = NULL;

      WinSetWindowText(WinWindowFromID(hWnd, IDC_STATIC_REPETITIONS_LEFT), "");
      WinShowWindow(WinWindowFromID(hWnd, IDC_STATIC_REPETITIONS_LABEL), FALSE);

      
      
      WinEnableWindow(WinWindowFromID(hWnd, IDC_BUTTON_GO), TRUE);

      WinEnableWindow(WinWindowFromID(hWnd, IDC_EDIT_SCRIPT_FILE_NAME), TRUE);
      WinEnableWindow(WinWindowFromID(WinQueryWindow(hWnd, QW_PARENT), IDC_EDIT_LOG_FILE_NAME), TRUE);
      WinEnableWindow(WinWindowFromID(WinQueryWindow(hWnd, QW_PARENT), IDC_CHECK_SHOW_LOG), TRUE);
      WinEnableWindow(WinWindowFromID(WinQueryWindow(hWnd, QW_PARENT), IDC_CHECK_LOG_TO_FRAME), TRUE);
      WinEnableWindow(WinWindowFromID(WinQueryWindow(hWnd, QW_PARENT), IDC_CHECK_LOG_TO_FILE), TRUE);
      if(bFlashWasEnabled)
        WinEnableWindow(WinWindowFromID(WinQueryWindow(hWnd, QW_PARENT), IDC_BUTTON_FLUSH), TRUE);
      WinEnableWindow(WinWindowFromID(WinQueryWindow(hWnd, QW_PARENT), IDC_BUTTON_CLEAR), TRUE);
      WinEnableWindow(WinWindowFromID(WinQueryWindow(hWnd, QW_PARENT), IDC_RADIO_MODE_MANUAL), TRUE);
      WinEnableWindow(WinWindowFromID(WinQueryWindow(hWnd, QW_PARENT), IDC_RADIO_MODE_AUTO), TRUE);
      break;
    }
    default:
      break;
  }
}

static BOOL onInitDialog(HWND hWnd, HWND hWndFocus, MPARAM mParam)
{
  CPlugin * pPlugin = (CPlugin *)mParam;
  WinSetWindowPtr(hWnd, QWL_USER, (PVOID)pPlugin);

  int iTopMargin = 188;
  WinSetWindowPos(hWnd, NULL, 0, iTopMargin, 0, 0, SWP_SHOW );  

  WinSetWindowText(WinWindowFromID(hWnd, IDC_EDIT_SCRIPT_FILE_NAME), pPlugin->m_Pref_szScriptFile);

  WinSetWindowText(WinWindowFromID(hWnd, IDC_STATIC_REPETITIONS_LEFT), "");
  WinShowWindow(WinWindowFromID(hWnd, IDC_STATIC_REPETITIONS_LABEL), FALSE);

  WinShowWindow(WinWindowFromID(hWnd, IDC_BUTTON_GO), TRUE);
  WinShowWindow(WinWindowFromID(hWnd, IDC_BUTTON_STOP), FALSE);

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

MRESULT EXPENTRY NP_LOADDS AutoDlgProc(HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  switch(msg)
  {
    case WM_INITDLG:
      onInitDialog(hWnd, 0, mp2);
      return (MRESULT)FALSE;
    case WM_COMMAND:
    case WM_CONTROL:
      onCommand(hWnd, SHORT1FROMMP(mp1), 0, SHORT2FROMMP(mp1));
      break;
    case WM_DESTROY:
      onDestroy(hWnd);
      break;

    default:
      return WinDefDlgProc(hWnd, msg, mp1, mp2);
  }
  return (MRESULT)TRUE;
}
