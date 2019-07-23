




































#include <windows.h>
#include <windowsx.h>

#include "plugin.h"
#include "nsIServiceManager.h"
#include "nsISupportsUtils.h" 





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
  mScriptablePeer(NULL)
{
  mhWnd = NULL;
  mString[0] = '\0';
}

nsPluginInstance::~nsPluginInstance()
{
  
  
  
  
  mScriptablePeer->SetInstance(NULL);
  NS_IF_RELEASE(mScriptablePeer);
}

static LRESULT CALLBACK PluginWinProc(HWND, UINT, WPARAM, LPARAM);
static WNDPROC lpOldProc = NULL;

NPBool nsPluginInstance::init(NPWindow* aWindow)
{
  if(aWindow == NULL)
    return FALSE;

  mhWnd = (HWND)aWindow->window;
  if(mhWnd == NULL)
    return FALSE;

  
  
  lpOldProc = SubclassWindow(mhWnd, (WNDPROC)PluginWinProc);

  
  
  SetWindowLong(mhWnd, GWL_USERDATA, (LONG)this);

  mInitialized = TRUE;
  return TRUE;
}

void nsPluginInstance::shut()
{
  
  SubclassWindow(mhWnd, lpOldProc);
  mhWnd = NULL;
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
  InvalidateRect(mhWnd, NULL, TRUE);
  UpdateWindow(mhWnd);
}


void nsPluginInstance::clear()
{
  strcpy(mString, "");
  InvalidateRect(mhWnd, NULL, TRUE);
  UpdateWindow(mhWnd);
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
    static nsIID scriptableIID = NS_ISCRIPTABLEPLUGINSAMPLE_IID;
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

static LRESULT CALLBACK PluginWinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg) {
    case WM_PAINT:
      {
        
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc;
        GetClientRect(hWnd, &rc);
        FrameRect(hdc, &rc, GetStockBrush(BLACK_BRUSH));

        
        nsPluginInstance *plugin = (nsPluginInstance *)GetWindowLong(hWnd, GWL_USERDATA);
        if (plugin)
          DrawText(hdc, plugin->mString, strlen(plugin->mString), &rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        else {
          char string[] = "Error occured";
          DrawText(hdc, string, strlen(string), &rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        }

        EndPaint(hWnd, &ps);
      }
      break;
    default:
      break;
  }

  return DefWindowProc(hWnd, msg, wParam, lParam);
}
