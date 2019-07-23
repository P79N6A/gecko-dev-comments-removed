




































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
  NPError SetWindow(NPWindow* pNPWindow);
  uint16  HandleEvent(void* event);

private:
  
  const char * getVersion();
  void DoDraw();
  NPBool StartDraw(NPWindow* window);
  void EndDraw(NPWindow* window);
  void DrawString(const unsigned char* text, short width, short height, short centerX, Rect drawRect);

  NPWindow * mWindow;
  NPP mInstance;
  NPBool mInitialized;
  GrafPtr mSavePort;
  RgnHandle mSaveClip;
  Rect mRevealedRect;
  short mSavePortTop;
  short mSavePortLeft;
};

#endif 
