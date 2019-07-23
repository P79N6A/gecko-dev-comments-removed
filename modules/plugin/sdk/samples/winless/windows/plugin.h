




































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

  NPError SetWindow(NPWindow* aWindow);
  uint16 HandleEvent(void* aEvent);

private:
  NPP mInstance;
  NPBool mInitialized;
  NPWindow * mWindow;
};

#endif 
