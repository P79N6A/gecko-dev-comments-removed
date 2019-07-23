




































#include <windows.h>
#include <windowsx.h>
#include <assert.h>

#include "resource.h"

#include "plugin.h"
#include "helpers.h"
#include "guihlp.h"
#include "logger.h"
#include "scripter.h"
#include "guiprefs.h"
#include "winutils.h"

extern HINSTANCE hInst;
extern CLogger * pLogger;







CPlugin::CPlugin(NPP pNPInstance, WORD wMode) :
  CPluginBase(pNPInstance, wMode),
  m_hInst(hInst),
  m_hWnd(NULL),
  m_hWndManual(NULL),
  m_hWndAuto(NULL),
  m_hWndParent(NULL),
  m_hWndStandAloneLogWindow(NULL),
  m_bPluginReady(FALSE),
  m_hWndLastEditFocus(NULL),
  m_iWidth(200),
  m_iHeight(500)
{
  restorePreferences();
  pLogger->setLogToFileFlag(m_Pref_bToFile);
  pLogger->blockDumpToFile(FALSE);
}

CPlugin::~CPlugin()
{
  savePreferences();
}

static char szSection[] = SECTION_PREFERENCES;
static char szYes[] = ENTRY_YES;
static char szNo[] = ENTRY_NO;

void CPlugin::restorePreferences()
{
  char szFileName[_MAX_PATH];
  GetINIFileName(m_hInst, szFileName, sizeof(szFileName));

  char sz[256];
  XP_GetPrivateProfileString(szSection, KEY_AUTO_MODE, ENTRY_NO, sz, sizeof(sz), szFileName);
  m_Pref_ShowGUI = (strcmpi(sz, ENTRY_YES) == 0) ? sg_auto : sg_manual;

  XP_GetPrivateProfileString(szSection, KEY_LOG_FILE, "Test.log", sz, sizeof(sz), szFileName);
  strcpy(m_Pref_szLogFile, sz);

  XP_GetPrivateProfileString(szSection, KEY_SCRIPT_FILE, "Test.pts", sz, sizeof(sz), szFileName);
  strcpy(m_Pref_szScriptFile, sz);

  XP_GetPrivateProfileString(szSection, KEY_TO_FILE, ENTRY_NO, sz, sizeof(sz), szFileName);
  m_Pref_bToFile = (strcmpi(sz, ENTRY_YES) == 0) ? TRUE : FALSE;

  XP_GetPrivateProfileString(szSection, KEY_TO_FRAME, ENTRY_YES, sz, sizeof(sz), szFileName);
  m_Pref_bToFrame = (strcmpi(sz, ENTRY_YES) == 0) ? TRUE : FALSE;

  XP_GetPrivateProfileString(szSection, KEY_FLUSH_NOW, ENTRY_YES, sz, sizeof(sz), szFileName);
  m_Pref_bFlushNow = (strcmpi(sz, ENTRY_YES) == 0) ? TRUE : FALSE;

  XP_GetPrivateProfileString(szSection, KEY_REMEMBER_LAST_API_CALL, ENTRY_YES, sz, sizeof(sz), szFileName);
  m_Pref_bRememberLastCall = (strcmpi(sz, ENTRY_YES) == 0) ? TRUE : FALSE;

  XP_GetPrivateProfileString(szSection, KEY_STAND_ALONE, ENTRY_NO, sz, sizeof(sz), szFileName);
  m_Pref_bStandAlone = (strcmpi(sz, ENTRY_YES) == 0) ? TRUE : FALSE;

  XP_GetPrivateProfileString(szSection, KEY_AUTOSTART_SCRIPT, ENTRY_NO, sz, sizeof(sz), szFileName);
  m_Pref_bAutoStartScript = (strcmpi(sz, ENTRY_YES) == 0) ? TRUE : FALSE;
}

void CPlugin::savePreferences()
{
  char szFileName[_MAX_PATH];
  GetINIFileName(m_hInst, szFileName, sizeof(szFileName));

  XP_WritePrivateProfileString(szSection, KEY_AUTO_MODE, (m_Pref_ShowGUI == sg_auto) ? szYes : szNo, szFileName);
  XP_WritePrivateProfileString(szSection, KEY_LOG_FILE, m_Pref_szLogFile, szFileName);
  XP_WritePrivateProfileString(szSection, KEY_SCRIPT_FILE, m_Pref_szScriptFile, szFileName);
  XP_WritePrivateProfileString(szSection, KEY_TO_FILE, m_Pref_bToFile ? szYes : szNo, szFileName);
  XP_WritePrivateProfileString(szSection, KEY_TO_FRAME, m_Pref_bToFrame ? szYes : szNo, szFileName);
  XP_WritePrivateProfileString(szSection, KEY_FLUSH_NOW, m_Pref_bFlushNow ? szYes : szNo, szFileName);
  XP_WritePrivateProfileString(szSection, KEY_REMEMBER_LAST_API_CALL, m_Pref_bRememberLastCall ? szYes : szNo, szFileName);
  XP_WritePrivateProfileString(szSection, KEY_STAND_ALONE, m_Pref_bStandAlone ? szYes : szNo, szFileName);
  XP_WritePrivateProfileString(szSection, KEY_AUTOSTART_SCRIPT, m_Pref_bAutoStartScript ? szYes : szNo, szFileName);
}

void CPlugin::updatePrefs(GUIPrefs prefs, int iValue, char * szValue)
{
  switch(prefs)
  {
    case gp_mode:
      m_Pref_ShowGUI = (ShowGUI)iValue;
      break;
    case gp_logfile:
      if(szValue && (strlen(szValue) < sizeof(m_Pref_szLogFile)))
        strcpy(m_Pref_szLogFile, szValue);
      break;
    case gp_scriptfile:
      if(szValue && (strlen(szValue) < sizeof(m_Pref_szScriptFile)))
        strcpy(m_Pref_szScriptFile, szValue);
      break;
    case gp_tofile:
      m_Pref_bToFile = (BOOL)iValue;
      break;
    case gp_toframe:
      m_Pref_bToFrame = (BOOL)iValue;
      break;
    case gp_flush:
      m_Pref_bFlushNow = (BOOL)iValue;
      break;
    case gp_rememberlast:
      m_Pref_bRememberLastCall = (BOOL)iValue;
      break;
    case gp_standalone:
      m_Pref_bStandAlone = (BOOL)iValue;
      break;
    case gp_autostartscript:
      m_Pref_bAutoStartScript = (BOOL)iValue;
      break;
    default:
      break;
  }
}

void CPlugin::getModulePath(LPSTR szPath, int iSize)
{
  GetModulePath(m_hInst, szPath, iSize);
}

void CPlugin::getLogFileName(LPSTR szLogFileName, int iSize)
{
  if(getMode() == NP_EMBED)
  {
    char sz[256];
    getModulePath(szLogFileName, iSize);
    Edit_GetText(GetDlgItem(m_hWnd, IDC_EDIT_LOG_FILE_NAME), sz, sizeof(sz));
    if(!strlen(sz))
      strcpy(sz, m_Pref_szLogFile);
    strcat(szLogFileName, sz);
  }
  else
    CPluginBase::getLogFileName(szLogFileName, iSize);
}

BOOL CALLBACK NP_LOADDS TesterDlgProc(HWND, UINT, WPARAM, LPARAM);

static char szStandAlonePluginWindowClassName[] = "StandAloneWindowClass";

BOOL CPlugin::initStandAlone()
{
  HWND hWndParent = NULL;

  
  WNDCLASS wc;
  wc.style         = 0; 
  wc.lpfnWndProc   = DefWindowProc; 
  wc.cbClsExtra    = 0; 
  wc.cbWndExtra    = 0; 
  wc.hInstance     = hInst; 
  wc.hIcon         = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON_APP)); 
  wc.hCursor       = LoadCursor(0, IDC_ARROW); 
  wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wc.lpszMenuName  = NULL; 
  wc.lpszClassName = szStandAlonePluginWindowClassName;

  
  
  RegisterClass(&wc);

  hWndParent = CreateWindow(szStandAlonePluginWindowClassName, 
                            "The Tester Plugin", 
                            WS_POPUPWINDOW | WS_CAPTION | WS_VISIBLE | WS_MINIMIZEBOX, 
                            0, 0, 800, 600, 
                            GetDesktopWindow(), NULL, m_hInst, NULL);


  
    
      

  m_hWndStandAloneLogWindow = CreateWindow("EDIT", "", 
                                            WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_READONLY,
                                            200, 3, 590, 562, 
                                            hWndParent, NULL, m_hInst, NULL);
  if(!IsWindow(hWndParent))
    return FALSE;

  m_hWndParent = hWndParent;

  HFONT hFont = GetStockFont(DEFAULT_GUI_FONT);
  SetWindowFont(m_hWndStandAloneLogWindow, hFont, FALSE);

  CreateDialogParam(m_hInst, MAKEINTRESOURCE(IDD_DIALOG_TESTER), m_hWndParent, (DLGPROC)TesterDlgProc, (LPARAM)this);

  m_bPluginReady = (m_hWnd != NULL);

  return m_bPluginReady;
}

void CPlugin::shutStandAlone()
{
  

  if (isStandAlone() && m_hWndParent) 
    DestroyWindow(m_hWndParent);

  m_bPluginReady = FALSE;
}

BOOL CPlugin::initEmbed(DWORD dwInitData)
{
  if (m_bPluginReady)
    return TRUE;

  HWND hWndParent = (HWND)dwInitData;

  if(!IsWindow(hWndParent))
    return FALSE;

  m_hWndParent = hWndParent;

  CreateDialogParam(m_hInst, MAKEINTRESOURCE(IDD_DIALOG_TESTER), m_hWndParent, (DLGPROC)TesterDlgProc, (LPARAM)this);

  m_bPluginReady = (m_hWnd != NULL);

  return m_bPluginReady;
}

BOOL CPlugin::initFull(DWORD dwInitData)
{
  m_bPluginReady = CPluginBase::initFull(dwInitData);
  return m_bPluginReady;
}

void CPlugin::shutEmbed()
{
  if(m_hWnd != NULL)
  {
    DestroyWindow(m_hWnd);
    m_hWnd = NULL;
  }

  m_bPluginReady = FALSE;
}

void CPlugin::shutFull()
{
  CPluginBase::shutFull();
  m_bPluginReady = FALSE;
}

BOOL CPlugin::isInitialized()
{
  return m_bPluginReady;
}

void CPlugin::onInit(HWND hWnd, HWND hWndManual, HWND hWndAuto)
{
  assert(hWnd != NULL);
  assert(hWndManual != NULL);
  assert(hWndAuto != NULL);

  m_hWnd = hWnd;
  m_hWndManual = hWndManual;
  m_hWndAuto = hWndAuto;

  pLogger->setShowImmediatelyFlag(IsDlgButtonChecked(m_hWnd, IDC_CHECK_SHOW_LOG) == BST_CHECKED);
  pLogger->setLogToFrameFlag(IsDlgButtonChecked(m_hWnd, IDC_CHECK_LOG_TO_FRAME) == BST_CHECKED);
  pLogger->setLogToFileFlag(IsDlgButtonChecked(m_hWnd, IDC_CHECK_LOG_TO_FILE) == BST_CHECKED);
}

BOOL CPlugin::isStandAlone()
{
  return m_Pref_bStandAlone;
}

void CPlugin::outputToNativeWindow(LPSTR szString)
{
  if (!m_hWndStandAloneLogWindow)
    return;

  

  static char text[16384];

  if (strlen(szString)) {
    int length = Edit_GetTextLength(m_hWndStandAloneLogWindow);
    if ((length + strlen(szString)) > sizeof(text))
      strcpy(text, szString);
    else {
      Edit_GetText(m_hWndStandAloneLogWindow, text, sizeof(text));
      strcat(text, szString);
    }

    Edit_SetText(m_hWndStandAloneLogWindow, text);
    int lines = Edit_GetLineCount(m_hWndStandAloneLogWindow);
    Edit_Scroll(m_hWndStandAloneLogWindow, lines, 0);
  }
  else
    Edit_SetText(m_hWndStandAloneLogWindow, ""); 






































}

int CPlugin::messageBox(LPSTR szMessage, LPSTR szTitle, UINT uStyle)
{
  return MessageBox(m_hWnd, szMessage, szTitle, uStyle);
}

void CPlugin::onDestroy()
{
  m_hWnd = NULL;
}

void CPlugin::onLogToFile(BOOL bLogToFile)
{
  pLogger->setLogToFileFlag(bLogToFile);
  if(!bLogToFile)
    pLogger->closeLogToFile();
}

const HINSTANCE CPlugin::getInstance()
{
  return m_hInst;
}

const HWND CPlugin::getWindow()
{
  return m_hWnd;
}

const HWND CPlugin::getParent()
{
  return m_hWndParent;
}

void CPlugin::showGUI(ShowGUI sg)
{
  switch (sg)
  {
    case sg_manual:
      ShowWindow(m_hWndManual, SW_SHOW);
      ShowWindow(m_hWndAuto, SW_HIDE);
      break;
    case sg_auto:
      ShowWindow(m_hWndManual, SW_HIDE);
      ShowWindow(m_hWndAuto, SW_SHOW);
      break;
    default:
      assert(0);
      break;
  }
}

DWORD CPlugin::makeNPNCall(NPAPI_Action action, DWORD dw1, DWORD dw2, DWORD dw3, 
                           DWORD dw4, DWORD dw5, DWORD dw6, DWORD dw7)
{
  DWORD dwRet = CPluginBase::makeNPNCall(action, dw1, dw2, dw3, dw4, dw5, dw6, dw7);

    
  if((getMode() == NP_EMBED) && (IsWindowVisible(m_hWndManual)))
  {
    switch (action)
    {
      case action_npn_new_stream:
      case action_npn_destroy_stream:
      case action_npn_mem_alloc:
      case action_npn_mem_free:
        updateUI(m_hWndManual);
        break;
      default:
        break;
    }
  }

  return dwRet;
}

void CPlugin::autoStartScriptIfNeeded()
{
  if (!m_Pref_bAutoStartScript)
    return;

  CScripter scripter;
  scripter.associate(this);

  char szFileName[_MAX_PATH];
  getModulePath(szFileName, sizeof(szFileName));
  strcat(szFileName, m_Pref_szScriptFile);

  if(scripter.createScriptFromFile(szFileName))
  {
    int iRepetitions = scripter.getCycleRepetitions();
    int iDelay = scripter.getCycleDelay();
    if(iDelay < 0)
      iDelay = 0;

    assert(pLogger != NULL);
    pLogger->resetStartTime();

    for(int i = 0; i < iRepetitions; i++)
    {
      scripter.executeScript();
      if(iDelay != 0)
        XP_Sleep(iDelay);
    }
  }
  else
  {
    MessageBox(NULL, "Script file not found or invalid", "", MB_OK | MB_ICONERROR);
  }
}


CPluginBase * CreatePlugin(NPP instance, uint16 mode)
{
  CPlugin * pPlugin = new CPlugin(instance, mode);
  return (CPluginBase *)pPlugin;
}

void DestroyPlugin(CPluginBase * pPlugin)
{
  if(pPlugin != NULL)
    delete (CPlugin *)pPlugin;
}
