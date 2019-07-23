




































#include "npapi.h"
#include "npupp.h"
#include "dbg.h"

NPNetscapeFuncs NPNFuncs;

NPError WINAPI NP_GetEntryPoints(NPPluginFuncs* aPluginFuncs)
{
  dbgOut1("wrapper: NP_GetEntryPoints");

  if(aPluginFuncs == NULL)
    return NPERR_INVALID_FUNCTABLE_ERROR;

  if(aPluginFuncs->size < sizeof(NPPluginFuncs))
    return NPERR_INVALID_FUNCTABLE_ERROR;

  aPluginFuncs->version       = (NP_VERSION_MAJOR << 8) | NP_VERSION_MINOR;
  aPluginFuncs->newp          = NPP_New;
  aPluginFuncs->destroy       = NPP_Destroy;
  aPluginFuncs->setwindow     = NPP_SetWindow;
  aPluginFuncs->newstream     = NPP_NewStream;
  aPluginFuncs->destroystream = NPP_DestroyStream;
  aPluginFuncs->asfile        = NPP_StreamAsFile;
  aPluginFuncs->writeready    = NPP_WriteReady;
  aPluginFuncs->write         = NPP_Write;
  aPluginFuncs->print         = NPP_Print;
  aPluginFuncs->event         = NPP_HandleEvent;
  aPluginFuncs->urlnotify     = NPP_URLNotify;
  aPluginFuncs->getvalue      = NPP_GetValue;
  aPluginFuncs->setvalue      = NPP_SetValue;
  aPluginFuncs->javaClass     = NULL;

  return NPERR_NO_ERROR;
}

NPError WINAPI NP_Initialize(NPNetscapeFuncs* aNetscapeFuncs)
{
  dbgOut1("wrapper: NP_Initialize");

  if(aNetscapeFuncs == NULL)
    return NPERR_INVALID_FUNCTABLE_ERROR;

  if(HIBYTE(aNetscapeFuncs->version) > NP_VERSION_MAJOR)
    return NPERR_INCOMPATIBLE_VERSION_ERROR;

  if(aNetscapeFuncs->size < sizeof NPNetscapeFuncs)
    return NPERR_INVALID_FUNCTABLE_ERROR;

  NPNFuncs.size             = aNetscapeFuncs->size;
  NPNFuncs.version          = aNetscapeFuncs->version;
  NPNFuncs.geturlnotify     = aNetscapeFuncs->geturlnotify;
  NPNFuncs.geturl           = aNetscapeFuncs->geturl;
  NPNFuncs.posturlnotify    = aNetscapeFuncs->posturlnotify;
  NPNFuncs.posturl          = aNetscapeFuncs->posturl;
  NPNFuncs.requestread      = aNetscapeFuncs->requestread;
  NPNFuncs.newstream        = aNetscapeFuncs->newstream;
  NPNFuncs.write            = aNetscapeFuncs->write;
  NPNFuncs.destroystream    = aNetscapeFuncs->destroystream;
  NPNFuncs.status           = aNetscapeFuncs->status;
  NPNFuncs.uagent           = aNetscapeFuncs->uagent;
  NPNFuncs.memalloc         = aNetscapeFuncs->memalloc;
  NPNFuncs.memfree          = aNetscapeFuncs->memfree;
  NPNFuncs.memflush         = aNetscapeFuncs->memflush;
  NPNFuncs.reloadplugins    = aNetscapeFuncs->reloadplugins;
  NPNFuncs.getJavaEnv       = aNetscapeFuncs->getJavaEnv;
  NPNFuncs.getJavaPeer      = aNetscapeFuncs->getJavaPeer;
  NPNFuncs.getvalue         = aNetscapeFuncs->getvalue;
  NPNFuncs.setvalue         = aNetscapeFuncs->setvalue;
  NPNFuncs.invalidaterect   = aNetscapeFuncs->invalidaterect;
  NPNFuncs.invalidateregion = aNetscapeFuncs->invalidateregion;
  NPNFuncs.forceredraw      = aNetscapeFuncs->forceredraw;

  return NPERR_NO_ERROR;
}

NPError WINAPI NP_Shutdown()
{
  dbgOut1("wrapper:NP_Shutdown");

  return NPERR_NO_ERROR;
}
