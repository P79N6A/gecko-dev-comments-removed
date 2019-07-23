




































#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#include "npapi.h"
#include "npruntime.h"

class CPlugin
{
private:
  NPP m_pNPInstance;

#ifdef XP_WIN
  HWND m_hWnd; 
#endif

  NPWindow * m_Window;
  
  NPStream * m_pNPStream;
  NPBool m_bInitialized;

  NPObject *m_pScriptableObject;

public:
  char m_String[128];

public:
  CPlugin(NPP pNPInstance);
  ~CPlugin();

  NPBool init(NPWindow* pNPWindow);
  void shut();
  NPBool isInitialized();
  
  int16 handleEvent(void* event);

  void showVersion();
  void clear();
  void getVersion(char* *aVersion);

  NPObject *GetScriptableObject();
};

#endif 
