




































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
  NPError SetWindow(NPWindow* pNPWindow);
  uint16  HandleEvent(void* event);
  NPError	GetValue(NPPVariable variable, void *value);

  void showVersion();
  void clear();
  nsScriptablePeer* getScriptablePeer();  

  char mString[128];

private:
  
  void DoDraw();
  NPBool StartDraw(NPWindow* window);
  void EndDraw(NPWindow* window);
  void DrawString(const unsigned char* text, short width, short height, short centerX, Rect drawRect);

  nsScriptablePeer * mScriptablePeer;
  
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
