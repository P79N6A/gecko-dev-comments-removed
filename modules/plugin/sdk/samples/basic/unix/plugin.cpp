




































#include "plugin.h"

#define MIME_TYPES_HANDLED  "application/basic-plugin"
#define PLUGIN_NAME         "Basic Example Plug-in for Mozilla"
#define MIME_TYPES_DESCRIPTION  MIME_TYPES_HANDLED":bsc:"PLUGIN_NAME
#define PLUGIN_DESCRIPTION  PLUGIN_NAME " (Plug-ins SDK sample)"


char* NPP_GetMIMEDescription(void)
{
  return(MIME_TYPES_DESCRIPTION);
}




NPError NS_PluginInitialize()
{
  return NPERR_NO_ERROR;
}

void NS_PluginShutdown()
{
}


NPError NS_PluginGetValue(NPPVariable aVariable, void *aValue)
{
  NPError err = NPERR_NO_ERROR;
  switch (aVariable) {
    case NPPVpluginNameString:
      *((char **)aValue) = PLUGIN_NAME;
      break;
    case NPPVpluginDescriptionString:
      *((char **)aValue) = PLUGIN_DESCRIPTION;
      break;
    default:
      err = NPERR_INVALID_PARAM;
      break;
  }
  return err;
}





nsPluginInstanceBase * NS_NewPluginInstance(nsPluginCreateData * aCreateDataStruct)
{
  if(!aCreateDataStruct)
    return NULL;

  nsPluginInstance * plugin = new nsPluginInstance(aCreateDataStruct->instance);
  return plugin;
}

void NS_DestroyPluginInstance(nsPluginInstanceBase * aPlugin)
{
  if(aPlugin)
    delete (nsPluginInstance *)aPlugin;
}





nsPluginInstance::nsPluginInstance(NPP aInstance) : nsPluginInstanceBase(),
  mInstance(aInstance),
  mInitialized(FALSE),
  mWindow(0),
  mXtwidget(0),
  mFontInfo(0)
{
}

nsPluginInstance::~nsPluginInstance()
{
}

static void
xt_event_handler(Widget xtwidget, nsPluginInstance *plugin, XEvent *xevent, Boolean *b)
{
  switch (xevent->type) {
    case Expose:
      
      if (plugin) {
        
        plugin->draw();
      }
      default:
        break;
  }
}

void nsPluginInstance::draw()
{
  unsigned int h = mHeight/2;
  unsigned int w = 3 * mWidth/4;
  int x = (mWidth - w)/2; 
  int y = h/2;
  if (x >= 0 && y >= 0) {
    GC gc = XCreateGC(mDisplay, mWindow, 0, NULL);
    if (!gc) 
      return;
    XDrawRectangle(mDisplay, mWindow, gc, x, y, w, h);
  const char *string = getVersion();
  if (string && *string) {
    int l = strlen(string);
    int fmba = mFontInfo->max_bounds.ascent;
    int fmbd = mFontInfo->max_bounds.descent;
    int fh = fmba + fmbd;
    y += fh;
    x += 32;
      XDrawString(mDisplay, mWindow, gc, x, y, string, l); 
    }
    XFreeGC(mDisplay, gc);
  }
}

NPBool nsPluginInstance::init(NPWindow* aWindow)
{
  if(aWindow == NULL)
    return FALSE;
  
  if (SetWindow(aWindow))
  mInitialized = TRUE;
	
  return mInitialized;
}

void nsPluginInstance::shut()
{
  mInitialized = FALSE;
}

const char * nsPluginInstance::getVersion()
{
  return NPN_UserAgent(mInstance);
}

NPError nsPluginInstance::GetValue(NPPVariable aVariable, void *aValue)
{
  NPError err = NPERR_NO_ERROR;
  switch (aVariable) {
    case NPPVpluginNameString:
    case NPPVpluginDescriptionString:
      return NS_PluginGetValue(aVariable, aValue) ;
      break;
    default:
      err = NPERR_INVALID_PARAM;
      break;
  }
  return err;

}

NPError nsPluginInstance::SetWindow(NPWindow* aWindow)
{
  if(aWindow == NULL)
    return FALSE;

  mX = aWindow->x;
  mY = aWindow->y;
  mWidth = aWindow->width;
  mHeight = aWindow->height;
  if (mWindow == (Window) aWindow->window) {
    
    
    
  } else {
    mWindow = (Window) aWindow->window;
    NPSetWindowCallbackStruct *ws_info = (NPSetWindowCallbackStruct *)aWindow->ws_info;
    mDisplay = ws_info->display;
    mVisual = ws_info->visual;
    mDepth = ws_info->depth;
    mColormap = ws_info->colormap;

    if (!mFontInfo) {
      if (!(mFontInfo = XLoadQueryFont(mDisplay, "9x15")))
        printf("Cannot open 9X15 font\n");
    }
    
    Widget xtwidget = XtWindowToWidget(mDisplay, mWindow);
    if (xtwidget && mXtwidget != xtwidget) {
      mXtwidget = xtwidget;
      long event_mask = ExposureMask;
      XSelectInput(mDisplay, mWindow, event_mask);
      XtAddEventHandler(xtwidget, event_mask, False, (XtEventHandler)xt_event_handler, this);
    }
  }
  draw();
  return TRUE;
}
