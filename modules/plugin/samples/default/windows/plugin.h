




































#ifndef __PLUGIN_HPP__
#define __PLUGIN_HPP__

#include "npapi.h"

class CPlugin
{
private:
  HINSTANCE m_hInst;
  NPP       m_pNPInstance;
  WORD      m_wMode;
  HWND      m_hWnd;
  HWND      m_hWndParent;
  HICON     m_hIcon;
  char*     m_szURLString;

  char*     m_szCommandMessage;
  BOOL      m_bWaitingStreamFromPFS;
  NPStream* m_PFSStream;

public:
  BOOL       m_bHidden;
  NPMIMEType m_pNPMIMEType;
  LPSTR      m_szPageURL;       
  LPSTR      m_szFileURL;       
  LPSTR      m_szFileExtension; 
  HWND       m_hWndDialog;

  
  BOOL m_bOnline;
  BOOL m_bJava;
  BOOL m_bJavaScript;
  BOOL m_bSmartUpdate;

private:
  BOOL useDefaultURL_P();
  BOOL doSmartUpdate_P();
  LPSTR createURLString();
  void getPluginSmart();
  void getPluginRegular();

public:
  CPlugin(HINSTANCE hInst, 
          NPP pNPInstance, 
          WORD wMode, 
          NPMIMEType pluginType, 
          LPSTR szPageURL, 
          LPSTR szFileURL, 
          LPSTR szFileExtension,
          BOOL bHidden);
  ~CPlugin();

  BOOL init(HWND hWnd);
  void shut();
  HWND getWindow();
  void showGetPluginDialog();
  void getPlugin();
  BOOL readyToRefresh();

  
  void print(NPPrint * pNPPrint);
  void URLNotify(const char * szURL);
  NPError newStream(NPMIMEType type, NPStream *stream, NPBool seekable, uint16 *stype);
  NPError destroyStream(NPStream *stream, NPError reason);

  
  void onCreate(HWND hWnd);
  void onLButtonUp(HWND hWnd, int x, int y, UINT keyFlags);
  void onRButtonUp(HWND hWnd, int x, int y, UINT keyFlags);
  void onPaint(HWND hWnd);

  void resize();
};


#define PAGE_URL_FOR_JAVASCRIPT "http://plugindoc.mozdev.org/winmime.html"

#define PLUGINFINDER_COMMAND_BEGINNING ""
#define PLUGINFINDER_COMMAND_END ""
#define DEFAULT_PLUGINFINDER_URL "http://plugindoc.mozdev.org/winmime.html"
#define JVM_SMARTUPDATE_URL "http://java.com/download"

#ifdef WIN32
#define REGISTRY_PLACE "Software\\Netscape\\Netscape Navigator\\Default Plugin"
#else
#define GWL_USERDATA        0
#define COLOR_3DSHADOW      COLOR_BTNFACE
#define COLOR_3DLIGHT       COLOR_BTNHIGHLIGHT
#define COLOR_3DDKSHADOW    COLOR_BTNSHADOW
#endif

#define CLASS_NULL_PLUGIN "NullPluginClass"
 
BOOL RegisterNullPluginWindowClass();
void UnregisterNullPluginWindowClass();

extern HINSTANCE hInst;

#endif 
