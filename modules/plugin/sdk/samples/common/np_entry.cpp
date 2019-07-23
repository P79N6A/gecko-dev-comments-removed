






































#include "npplat.h"
#include "pluginbase.h"

NPNetscapeFuncs NPNFuncs;

NPError OSCALL NP_Shutdown()
{
  NS_PluginShutdown();
  return NPERR_NO_ERROR;
}

static NPError fillPluginFunctionTable(NPPluginFuncs* aNPPFuncs)
{
  if (!aNPPFuncs)
    return NPERR_INVALID_FUNCTABLE_ERROR;

  
  aNPPFuncs->version       = (NP_VERSION_MAJOR << 8) | NP_VERSION_MINOR;
  aNPPFuncs->newp          = NPP_New;
  aNPPFuncs->destroy       = NPP_Destroy;
  aNPPFuncs->setwindow     = NPP_SetWindow;
  aNPPFuncs->newstream     = NPP_NewStream;
  aNPPFuncs->destroystream = NPP_DestroyStream;
  aNPPFuncs->asfile        = NPP_StreamAsFile;
  aNPPFuncs->writeready    = NPP_WriteReady;
  aNPPFuncs->write         = NPP_Write;
  aNPPFuncs->print         = NPP_Print;
  aNPPFuncs->event         = NPP_HandleEvent;
  aNPPFuncs->urlnotify     = NPP_URLNotify;
  aNPPFuncs->getvalue      = NPP_GetValue;
  aNPPFuncs->setvalue      = NPP_SetValue;

  return NPERR_NO_ERROR;
}

static NPError fillNetscapeFunctionTable(NPNetscapeFuncs* aNPNFuncs)
{
  if (!aNPNFuncs)
    return NPERR_INVALID_FUNCTABLE_ERROR;

  if (HIBYTE(aNPNFuncs->version) > NP_VERSION_MAJOR)
    return NPERR_INCOMPATIBLE_VERSION_ERROR;

  if (aNPNFuncs->size < sizeof(NPNetscapeFuncs))
    return NPERR_INVALID_FUNCTABLE_ERROR;

  NPNFuncs.size             = aNPNFuncs->size;
  NPNFuncs.version          = aNPNFuncs->version;
  NPNFuncs.geturlnotify     = aNPNFuncs->geturlnotify;
  NPNFuncs.geturl           = aNPNFuncs->geturl;
  NPNFuncs.posturlnotify    = aNPNFuncs->posturlnotify;
  NPNFuncs.posturl          = aNPNFuncs->posturl;
  NPNFuncs.requestread      = aNPNFuncs->requestread;
  NPNFuncs.newstream        = aNPNFuncs->newstream;
  NPNFuncs.write            = aNPNFuncs->write;
  NPNFuncs.destroystream    = aNPNFuncs->destroystream;
  NPNFuncs.status           = aNPNFuncs->status;
  NPNFuncs.uagent           = aNPNFuncs->uagent;
  NPNFuncs.memalloc         = aNPNFuncs->memalloc;
  NPNFuncs.memfree          = aNPNFuncs->memfree;
  NPNFuncs.memflush         = aNPNFuncs->memflush;
  NPNFuncs.reloadplugins    = aNPNFuncs->reloadplugins;
  NPNFuncs.getvalue         = aNPNFuncs->getvalue;
  NPNFuncs.setvalue         = aNPNFuncs->setvalue;
  NPNFuncs.invalidaterect   = aNPNFuncs->invalidaterect;
  NPNFuncs.invalidateregion = aNPNFuncs->invalidateregion;
  NPNFuncs.forceredraw      = aNPNFuncs->forceredraw;

  return NPERR_NO_ERROR;
}





#ifdef XP_WIN
NPError OSCALL NP_Initialize(NPNetscapeFuncs* aNPNFuncs)
{
  NPError rv = fillNetscapeFunctionTable(aNPNFuncs);
  if (rv != NPERR_NO_ERROR)
    return rv;

  return NS_PluginInitialize();
}

NPError OSCALL NP_GetEntryPoints(NPPluginFuncs* aNPPFuncs)
{
  return fillPluginFunctionTable(aNPPFuncs);
}
#endif 

#ifdef XP_UNIX
NPError NP_Initialize(NPNetscapeFuncs* aNPNFuncs, NPPluginFuncs* aNPPFuncs)
{
  NPError rv = fillNetscapeFunctionTable(aNPNFuncs);
  if (rv != NPERR_NO_ERROR)
    return rv;

  rv = fillPluginFunctionTable(aNPPFuncs);
  if (rv != NPERR_NO_ERROR)
    return rv;

  return NS_PluginInitialize();
}

char * NP_GetMIMEDescription(void)
{
  return NPP_GetMIMEDescription();
}

NPError NP_GetValue(void *future, NPPVariable aVariable, void *aValue)
{
  return NS_PluginGetValue(aVariable, aValue);
}
#endif 

#ifdef XP_MAC
short gResFile; 

NPError Private_Initialize(void)
{
  NPError rv = NS_PluginInitialize();
  return rv;
}

void Private_Shutdown(void)
{
  NS_PluginShutdown();
  __destroy_global_chain();
}

NPError main(NPNetscapeFuncs* aNPNFuncs, NPPluginFuncs* aNPPFuncs, NPP_ShutdownUPP* aUnloadUpp)
{
  NPError rv = NPERR_NO_ERROR;

  if (!aUnloadUpp)
    rv = NPERR_INVALID_FUNCTABLE_ERROR;

  if (rv == NPERR_NO_ERROR)
    rv = fillNetscapeFunctionTable(aNPNFuncs);

  if (rv == NPERR_NO_ERROR) {
    
    __InitCode__();
    rv = fillPluginFunctionTable(aNPPFuncs);
  }

  *aUnloadUpp = NewNPP_ShutdownProc(Private_Shutdown);
  gResFile = CurResFile();
  rv = Private_Initialize();

  return rv;
}
#endif 
