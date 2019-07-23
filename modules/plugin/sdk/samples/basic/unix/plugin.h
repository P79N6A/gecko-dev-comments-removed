




































#ifndef __PLUGIN_H__
#define __PLUGIN_H__


#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/cursorfont.h>

#include "pluginbase.h"

class nsPluginInstance : public nsPluginInstanceBase
{
public:
  nsPluginInstance(NPP aInstance);
  virtual ~nsPluginInstance();

  NPBool init(NPWindow* aWindow);
  void shut();
  NPBool isInitialized() {return mInitialized;}
  NPError GetValue(NPPVariable variable, void *value);
  NPError SetWindow(NPWindow* aWindow);

  
  const char * getVersion();
  void draw();

private:
  NPP mInstance;
  NPBool mInitialized;

  Window mWindow;
  Display *mDisplay;
  Widget mXtwidget;
  int mX, mY;
  int mWidth, mHeight;
  Visual* mVisual;
  Colormap mColormap;
  unsigned int mDepth;
  XFontStruct *mFontInfo;
  GC mGC;
};

#endif 
