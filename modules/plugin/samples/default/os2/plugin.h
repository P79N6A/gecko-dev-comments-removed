




































#ifndef __PLUGIN_HPP__
#define __PLUGIN_HPP__

#include "npapi.h"

class CPlugin
{
private:
  HMODULE   m_hInst;
  NPP       m_pNPInstance;
  SHORT     m_wMode;
  HWND      m_hWnd;
  HWND      m_hWndParent;
  HPOINTER  m_hIcon;
  char *    m_szURLString;

  char *    m_szCommandMessage;
  BOOL      m_bWaitingStreamFromPFS;
  NPStream* m_PFSStream;

public:
  BOOL       m_bHidden;
  NPMIMEType m_pNPMIMEType;
  PSZ        m_szPageURL;       
  PSZ        m_szFileURL;       
  PSZ        m_szFileExtension; 
  HWND       m_hWndDialog;

  
  BOOL m_bOnline;
  BOOL m_bJava;
  BOOL m_bJavaScript;
  BOOL m_bSmartUpdate;

private:
  BOOL useDefaultURL_P();
  BOOL doSmartUpdate_P();
  PSZ createURLString();
  void getPluginSmart();
  void getPluginRegular();

public:
  CPlugin(HMODULE hInst, 
          NPP pNPInstance, 
          SHORT wMode, 
          NPMIMEType pluginType, 
          PSZ szPageURL, 
          PSZ szFileURL, 
          PSZ szFileExtension,
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

#define PLUGINFINDER_COMMAND_BEGINNING "javascript:window.open(\""
#define PLUGINFINDER_COMMAND_END "\",\"plugin\",\"toolbar=no,status=no,resizable=no,scrollbars=no,height=252,width=626\");"
#define DEFAULT_PLUGINFINDER_URL "http://plugindoc.mozdev.org/OS2.html"
#define JVM_SMARTUPDATE_URL "http://plugindoc.mozdev.org/OS2.html"

#define OS2INI_PLACE "Mozilla Default Plugin"
#define GWL_USERDATA        0
#define COLOR_3DSHADOW      COLOR_BTNFACE
#define COLOR_3DLIGHT       COLOR_BTNHIGHLIGHT
#define COLOR_3DDKSHADOW    COLOR_BTNSHADOW

#define CLASS_NULL_PLUGIN "NullPluginClass"
 
BOOL RegisterNullPluginWindowClass();
void UnregisterNullPluginWindowClass();

extern HMODULE hInst;

#endif 
