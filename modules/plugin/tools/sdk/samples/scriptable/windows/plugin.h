




































#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#include "pluginbase.h"
#include "nsScriptablePeer.h"

class nsPluginInstance : public nsPluginInstanceBase
{
public:
  nsPluginInstance(NPP aInstance);
  ~nsPluginInstance();

  NPBool init(NPWindow* aWindow);
  void shut();
  NPBool isInitialized();

  
  
  NPError	GetValue(NPPVariable variable, void *value);

  
  void showVersion();
  void clear();

  nsScriptablePeer* getScriptablePeer();

private:
  NPP mInstance;
  NPBool mInitialized;
  HWND mhWnd;
  nsScriptablePeer * mScriptablePeer;

public:
  char mString[128];
};

#endif 
