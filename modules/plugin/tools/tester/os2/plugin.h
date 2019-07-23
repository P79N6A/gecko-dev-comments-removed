




































#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#include "plugbase.h"

#define BST_UNCHECKED     0
#define BST_CHECKED       1
#define BST_INDETERMINATE 2

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
  gp_standalone
}GUIPrefs;

class CPlugin : public CPluginBase
{
private:
  HMODULE m_hInst;
  HWND m_hWnd;
  HWND m_hWndParent;
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


public:
  CPlugin(NPP pNPInstance, USHORT wMode);
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

  ULONG makeNPNCall(NPAPI_Action = action_invalid, 
                    ULONG dw1 = 0L, ULONG dw2 = 0L, 
                    ULONG dw3 = 0L, ULONG dw4 = 0L, 
                    ULONG dw5 = 0L, ULONG dw6 = 0L, 
                    ULONG dw7 = 0L);

  
  
  
  const HMODULE getInstance();
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
};

#endif 
