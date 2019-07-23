




































#include "windows.h"

#include "npupp.h"
#include "npapi.h"
#include "plugin.h"
#include "plugload.h"
#include "dbg.h"

extern NPNetscapeFuncs NPNFuncs;
NPNetscapeFuncs wrapperNPNFuncs;
NPPluginFuncs pluginNPPFuncs;

typedef NPError (__stdcall * NP_GETENTRYPOINTS)(NPPluginFuncs *);
typedef NPError (__stdcall * NP_INITIALIZE)(NPNetscapeFuncs *);
typedef NPError (__stdcall * NP_SHUTDOWN)(void);

NP_SHUTDOWN plugin_NP_Shutdown = NULL;
HINSTANCE pluginLibrary = NULL;

nsPluginThread::nsPluginThread(DWORD aP1) : CThread(),
  mP1(aP1),
  mP2(0),
  mP3(0),
  mP4(0),
  mP5(0),
  mP6(0),
  mP7(0)
{
  dbgOut1("nsPluginThread::nsPluginThread");

  open(this);
}

nsPluginThread::~nsPluginThread()
{
  dbgOut1("nsPluginThread::~nsPluginThread");

  close(this);
}

BOOL nsPluginThread::init()
{
  dbgOut1("nsPluginThread::init");

  
  
  pluginLibrary = LoadRealPlugin((NPMIMEType)mP1);
  if(!pluginLibrary)
    return FALSE;

  NP_GETENTRYPOINTS plugin_NP_GetEntryPoints = (NP_GETENTRYPOINTS)GetProcAddress(pluginLibrary, "NP_GetEntryPoints");
  if(!plugin_NP_GetEntryPoints)
    return FALSE;

  NP_INITIALIZE plugin_NP_Initialize = (NP_INITIALIZE)GetProcAddress(pluginLibrary, "NP_Initialize");
  if(!plugin_NP_Initialize)
    return FALSE;

  plugin_NP_Shutdown = (NP_SHUTDOWN)GetProcAddress(pluginLibrary, "NP_Shutdown");
  if(!plugin_NP_Shutdown)
    return FALSE;

  
  memset(&pluginNPPFuncs, 0, sizeof(NPPluginFuncs));
  pluginNPPFuncs.size = sizeof(NPPluginFuncs);

  plugin_NP_GetEntryPoints(&pluginNPPFuncs);

  
  memset((void *)&wrapperNPNFuncs, 0, sizeof(wrapperNPNFuncs));
  wrapperNPNFuncs.size             = sizeof(wrapperNPNFuncs);
  wrapperNPNFuncs.version          = NPNFuncs.version;
  wrapperNPNFuncs.geturlnotify     = NPN_GetURLNotify;
  wrapperNPNFuncs.geturl           = NPN_GetURL;
  wrapperNPNFuncs.posturlnotify    = NPN_PostURLNotify;
  wrapperNPNFuncs.posturl          = NPN_PostURL;
  wrapperNPNFuncs.requestread      = NPN_RequestRead;
  wrapperNPNFuncs.newstream        = NPN_NewStream;
  wrapperNPNFuncs.write            = NPN_Write;
  wrapperNPNFuncs.destroystream    = NPN_DestroyStream;
  wrapperNPNFuncs.status           = NPN_Status;
  wrapperNPNFuncs.uagent           = NPN_UserAgent;
  wrapperNPNFuncs.memalloc         = NPN_MemAlloc;
  wrapperNPNFuncs.memfree          = NPN_MemFree;
  wrapperNPNFuncs.memflush         = NPN_MemFlush;
  wrapperNPNFuncs.reloadplugins    = NPN_ReloadPlugins;
  wrapperNPNFuncs.getJavaEnv       = NPN_GetJavaEnv;
  wrapperNPNFuncs.getJavaPeer      = NPN_GetJavaPeer;
  wrapperNPNFuncs.getvalue         = NPN_GetValue;
  wrapperNPNFuncs.setvalue         = NPN_SetValue;
  wrapperNPNFuncs.invalidaterect   = NPN_InvalidateRect;
  wrapperNPNFuncs.invalidateregion = NPN_InvalidateRegion;
  wrapperNPNFuncs.forceredraw      = NPN_ForceRedraw;

  plugin_NP_Initialize(&wrapperNPNFuncs);

  return TRUE;
}

void nsPluginThread::shut()
{
  dbgOut1("nsPluginThread::shut");

  if (!plugin_NP_Shutdown)
    plugin_NP_Shutdown();

  if (!pluginLibrary)
    UnloadRealPlugin(pluginLibrary);
}


DWORD nsPluginThread::callNPP(npapiAction aAction, DWORD aP1, DWORD aP2, 
                              DWORD aP3, DWORD aP4, DWORD aP5, 
                              DWORD aP6, DWORD aP7)
{
  
  while (isBusy()) {
    Sleep(0);
  }

  mP1 = aP1;
  mP2 = aP2;
  mP3 = aP3;
  mP4 = aP4;
  mP5 = aP5;
  mP6 = aP6;
  mP7 = aP7;

  doAction(aAction);

  
  while (isBusy()) {
    Sleep(0);
  }

  return NPERR_NO_ERROR;
}
void nsPluginThread::dispatch()
{
  dbgOut2("nsPluginThread::dispatch: %s", FormatAction(mAction));

  switch (mAction) {
    case action_npp_new:
      pluginNPPFuncs.newp((NPMIMEType)mP1, (NPP)mP2, (uint16)mP3, (int16)mP4, (char**)mP5, (char**)mP6, (NPSavedData*)mP7);
      break;
    case action_npp_destroy:
      pluginNPPFuncs.destroy((NPP)mP1, (NPSavedData**)mP2);
      break;
    case action_npp_set_window:
      pluginNPPFuncs.setwindow((NPP)mP1, (NPWindow*)mP2);
      break;
    case action_npp_new_stream:
      pluginNPPFuncs.newstream((NPP)mP1, (NPMIMEType)mP2, (NPStream*)mP3, (NPBool)mP4, (uint16*)mP5);
      break;
    case action_npp_destroy_stream:
    {
      NPStream * stream = (NPStream *)mP2;
      pluginNPPFuncs.destroystream((NPP)mP1, stream, (NPError)mP3);
      break;
    }
    case action_npp_stream_as_file:
    {
      NPStream * stream = (NPStream *)mP2;
      pluginNPPFuncs.asfile((NPP)mP1, stream, (char*)mP3);
      break;
    }
    case action_npp_write_ready:
      pluginNPPFuncs.writeready((NPP)mP1, (NPStream *)mP2);
      break;
    case action_npp_write:
      pluginNPPFuncs.write((NPP)mP1, (NPStream *)mP2, (int32)mP3, (int32)mP4, (void *)mP5);
      break;
    case action_npp_print:
      pluginNPPFuncs.print((NPP)mP1, (NPPrint*)mP2);
      break;
    case action_npp_handle_event:
      pluginNPPFuncs.event((NPP)mP1, (void *)mP2);
      break;
    case action_npp_url_notify:
      pluginNPPFuncs.urlnotify((NPP)mP1, (const char*)mP2, (NPReason)mP3, (void*)mP4);
      break;
    case action_npp_get_java_class:
      
      break;
    case action_npp_get_value:
      pluginNPPFuncs.getvalue((NPP)mP1, (NPPVariable)mP2, (void *)mP3);
      break;
    case action_npp_set_value:
      pluginNPPFuncs.setvalue((NPP)mP1, (NPNVariable)mP2, (void *)mP3);
      break;
    default:
      dbgOut1("Unexpected action!");
      break;
  }
}

