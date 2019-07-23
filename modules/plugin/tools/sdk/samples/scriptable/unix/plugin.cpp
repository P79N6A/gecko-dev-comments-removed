





































#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/cursorfont.h>

#include "plugin.h"
#include "nsIServiceManager.h"
#include "nsISupportsUtils.h" 

#define MIME_TYPES_HANDLED  "application/scriptable-plugin"
#define PLUGIN_NAME         "Scriptable Example Plugin for Mozilla"
#define MIME_TYPES_DESCRIPTION  MIME_TYPES_HANDLED":scr:"PLUGIN_NAME
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
  mFontInfo(0),
  mScriptablePeer(NULL)
{
  mString[0] = '\0';
}

nsPluginInstance::~nsPluginInstance()
{
  
  
  
  
  mScriptablePeer->SetInstance(NULL);
  NS_IF_RELEASE(mScriptablePeer);
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
  XClearArea(mDisplay, mWindow, 0, 0, 0, 0, False);
  if (x >= 0 && y >= 0) {
    GC gc = XCreateGC(mDisplay, mWindow, 0, NULL);
    if (!gc)
      return;
    XDrawRectangle(mDisplay, mWindow, gc, x, y, w, h);
    if (mString[0]) {
  int l = strlen(mString);
  int fmba = mFontInfo->max_bounds.ascent;
  int fmbd = mFontInfo->max_bounds.descent;
  int fh = fmba + fmbd;
  y += fh;
  x += 32;
      XDrawString(mDisplay, mWindow, gc, x, y, mString, l);
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

NPError nsPluginInstance::SetWindow(NPWindow* aWindow)
{
  if(aWindow == NULL)
    return FALSE;

  mX = aWindow->x;
  mY = aWindow->y;
  mWidth = aWindow->width;
  mHeight = aWindow->height;
  if (mWindow != (Window) aWindow->window) {
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


void nsPluginInstance::shut()
{
  mInitialized = FALSE;
}

NPBool nsPluginInstance::isInitialized()
{
  return mInitialized;
}


void nsPluginInstance::showVersion()
{
  const char *ua = NPN_UserAgent(mInstance);
  strcpy(mString, ua);
  draw();
}


void nsPluginInstance::clear()
{
  strcpy(mString, "");
  draw();
}










NPError	nsPluginInstance::GetValue(NPPVariable aVariable, void *aValue)
{
  NPError rv = NPERR_NO_ERROR;

  if (aVariable == NPPVpluginScriptableInstance) {
      
    nsIScriptablePluginSample * scriptablePeer = getScriptablePeer();
      if (scriptablePeer) {
        *(nsISupports **)aValue = scriptablePeer;
      } else
        rv = NPERR_OUT_OF_MEMORY_ERROR;
    }
  else if (aVariable == NPPVpluginScriptableIID) {
    static nsIID scriptableIID =  NS_ISCRIPTABLEPLUGINSAMPLE_IID;
      nsIID* ptr = (nsIID *)NPN_MemAlloc(sizeof(nsIID));
      if (ptr) {
          *ptr = scriptableIID;
          *(nsIID **)aValue = ptr;
      } else
        rv = NPERR_OUT_OF_MEMORY_ERROR;
  }

  return rv;
}






nsScriptablePeer* nsPluginInstance::getScriptablePeer()
{
  if (!mScriptablePeer) {
    mScriptablePeer = new nsScriptablePeer(this);
    if(!mScriptablePeer)
      return NULL;

    NS_ADDREF(mScriptablePeer);
  }

  
  NS_ADDREF(mScriptablePeer);
  return mScriptablePeer;
}
