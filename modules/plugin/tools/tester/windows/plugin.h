




































#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#include "plugbase.h"

typedef enum
{
  sg_manual = 1,
  sg_auto
} ShowGUI;

typedef enum
{
  gp_mode = 1,
  gp_logfile,
  gp_scriptfile,
  gp_tofile,
  gp_toframe,
  gp_flush,
  gp_rememberlast,
  gp_standalone,
  gp_autostartscript
}GUIPrefs;

class CPlugin : public CPluginBase
{

public:
  CPlugin(NPP pNPInstance, WORD wMode);
  ~CPlugin();

  BOOL initEmbed(DWORD dwInitData);
  BOOL initFull(DWORD dwInitData);
  void shutEmbed();
  void shutFull();
  BOOL isInitialized();
  void getModulePath(LPSTR szPath, int iSize);
  int messageBox(LPSTR szMessage, LPSTR szTitle, UINT uStyle);
  void getLogFileName(LPSTR szLogFileName, int iSize);
  BOOL initStandAlone(); 
  void shutStandAlone(); 
  BOOL isStandAlone(); 
  void outputToNativeWindow(LPSTR szString); 

  void autoStartScriptIfNeeded();

  DWORD makeNPNCall(NPAPI_Action = action_invalid, 
                    DWORD dw1 = 0L, DWORD dw2 = 0L, 
                    DWORD dw3 = 0L, DWORD dw4 = 0L, 
                    DWORD dw5 = 0L, DWORD dw6 = 0L, 
                    DWORD dw7 = 0L);

  
  
  
  const HINSTANCE getInstance();
  const HWND getWindow();
  const HWND getParent();

  
  BOOL createTester();
  void destroyTester();
  void showGUI(ShowGUI sg);

  
  void restorePreferences();
  void savePreferences();
  void updatePrefs(GUIPrefs prefs, int iValue, char * szValue = NULL);

  
  void onInit(HWND hWnd, HWND hWndManual, HWND hWndAuto);
  void onDestroy();
  void onLogToFile(BOOL bLofToFile);

private:
  HINSTANCE m_hInst;
  HWND m_hWnd;
  HWND m_hWndParent;
  HWND m_hWndStandAloneLogWindow;
  HWND m_hWndManual;
  HWND m_hWndAuto;
  BOOL m_bPluginReady;


public:
  HWND m_hWndLastEditFocus;
  int m_iWidth;
  int m_iHeight;


  ShowGUI m_Pref_ShowGUI;
  char m_Pref_szLogFile[256];
  char m_Pref_szScriptFile[256];
  BOOL m_Pref_bToFile;
  BOOL m_Pref_bToFrame;
  BOOL m_Pref_bFlushNow;
  BOOL m_Pref_bRememberLastCall;
  BOOL m_Pref_bStandAlone;
  BOOL m_Pref_bAutoStartScript;
};

#endif 
