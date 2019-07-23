











































#ifndef __PLUGINBASE_H__
#define __PLUGINBASE_H__

#include "xp.h"
#include "action.h"


#define NPAPI_INI_FILE_NAME   "npapi.ini"

#define DEFAULT_DWARG_VALUE   0xDDDDDDDD

class CPluginBase
{
private:
  NPP m_pNPInstance;
  WORD m_wMode;
  NPStream * m_pStream;
  NPStream * m_pScriptStream;
  char m_szScriptCacheFile[_MAX_PATH];

  
  
  BOOL m_bWaitingForScriptStream;

public:
  void * m_pNPNAlloced; 
  void * m_pValue;      
  NPAPI_Action m_firstAction; 

public:
  CPluginBase(NPP pNPInstance, WORD wMode);
  ~CPluginBase();

  
  virtual BOOL initEmbed(DWORD dwInitData) = 0;
  virtual void shutEmbed() = 0;
  virtual BOOL isInitialized() = 0; 
  virtual void getModulePath(LPSTR szPath, int iSize) = 0;
  virtual int messageBox(LPSTR szMessage, LPSTR szTitle, UINT uStyle) = 0;
  virtual BOOL isStandAlone() = 0; 
  virtual BOOL initStandAlone() = 0; 
  virtual void shutStandAlone() = 0; 
  virtual void outputToNativeWindow(LPSTR szString) = 0; 

  
  virtual BOOL initFull(DWORD dwInitData);
  virtual void shutFull();
  virtual BOOL init(DWORD dwInitData);
  virtual void shut();
  virtual void getLogFileName(LPSTR szLogFileName, int iSize);
  virtual void autoStartScriptIfNeeded();

  const NPP getNPInstance();
  const WORD getMode();
  const NPStream * getNPStream();

  
  void onNPP_NewStream(NPP pInstance, LPSTR szMIMEType, NPStream * pStream, NPBool bSeekable, uint16 * puType);
  void onNPP_StreamAsFile(NPP pInstance, NPStream * pStream, const char * szFileName);
  void onNPP_DestroyStream(NPStream * pStream);
 
  
  virtual DWORD makeNPNCall(NPAPI_Action = action_invalid, 
                            DWORD dw1 = 0L, DWORD dw2 = 0L, 
                            DWORD dw3 = 0L, DWORD dw4 = 0L, 
                            DWORD dw5 = 0L, DWORD dw6 = 0L, 
                            DWORD dw7 = 0L);
};


CPluginBase * CreatePlugin(NPP instance, uint16 mode);
void DestroyPlugin(CPluginBase * pPlugin);

#endif 
