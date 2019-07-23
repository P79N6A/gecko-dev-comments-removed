




































#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#include "pluginbase.h"

class nsPluginInstance : public nsPluginInstanceBase
{
public:
  nsPluginInstance(NPP aInstance);
  ~nsPluginInstance();

  NPBool init(NPWindow* aWindow);
  void shut();
  NPBool isInitialized();

  
  const char * getVersion();

private:
  NPP mInstance;
  NPBool mInitialized;
  HWND mhWnd;
};

#endif 
