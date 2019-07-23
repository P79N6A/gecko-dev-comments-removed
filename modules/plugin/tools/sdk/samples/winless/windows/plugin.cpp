





































#include <windows.h>
#include <windowsx.h>

#include "plugin.h"





NPError NS_PluginInitialize()
{
  return NPERR_NO_ERROR;
}

void NS_PluginShutdown()
{
}





nsPluginInstanceBase * NS_NewPluginInstance(nsPluginCreateData * aCreateDataStruct)
{
  if(!aCreateDataStruct)
    return NULL;

  nsPluginInstance * plugin = new nsPluginInstance(aCreateDataStruct->instance);

  
  NPN_SetValue(aCreateDataStruct->instance, NPPVpluginWindowBool, NULL);

  return plugin;
}

void NS_DestroyPluginInstance(nsPluginInstanceBase * aPlugin)
{
  if(aPlugin)
    delete (nsPluginInstance *)aPlugin;
}





nsPluginInstance::nsPluginInstance(NPP aInstance) : nsPluginInstanceBase(),
  mInstance(aInstance),
  mInitialized(FALSE)
{
}

nsPluginInstance::~nsPluginInstance()
{
}

NPBool nsPluginInstance::init(NPWindow* aWindow)
{
  
  mInitialized = TRUE;
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

NPError nsPluginInstance::SetWindow(NPWindow* aWindow)
{
  
  mWindow = aWindow;
  return NPERR_NO_ERROR;
}

uint16 nsPluginInstance::HandleEvent(void* aEvent)
{
  NPEvent * event = (NPEvent *)aEvent;
  switch (event->event) {
    case WM_PAINT: 
    {
      if(!mWindow)
        break;

      
      RECT * drc = (RECT *)event->lParam;
      if(drc)
        FillRect((HDC)event->wParam, drc, (HBRUSH)(COLOR_ACTIVECAPTION+1));
      else {
        RECT rc;
        rc.bottom = mWindow->y + mWindow->height;
        rc.left   = mWindow->x;
        rc.right  = mWindow->x + mWindow->width;
        rc.top    = mWindow->y;
        FillRect((HDC)event->wParam, &rc, (HBRUSH)(COLOR_ACTIVECAPTION+1));
      }
      break;
    }
    case WM_KEYDOWN:
    {
      Beep(1000,100);
      break;
    }
    default:
      return 0;
  }
  return 1;
}