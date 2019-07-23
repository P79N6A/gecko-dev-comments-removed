




































#include "xp.h"

#include "npapi.h"
#include "npupp.h"
#include "epmanager.h"
#include "logger.h"




NPPEntryPointManager * epManager = NULL;

Logger * logger = NULL;

NPNetscapeFuncs NPNFuncs;

NPError WINAPI NP_GetEntryPoints(NPPluginFuncs* pFuncs)
{
  
  if(!logger)
  {
    logger = NewLogger();
    if(logger)
    {
      logger->platformInit();
      logger->init();
    }
  }

  if(logger)
    logger->logNS_NP_GetEntryPoints();

  if(pFuncs == NULL)
    return NPERR_INVALID_FUNCTABLE_ERROR;

  if(pFuncs->size < sizeof(NPPluginFuncs))
    return NPERR_INVALID_FUNCTABLE_ERROR;

  pFuncs->version       = (NP_VERSION_MAJOR << 8) | NP_VERSION_MINOR;
  pFuncs->newp          = NPP_New;
  pFuncs->destroy       = NPP_Destroy;
  pFuncs->setwindow     = NPP_SetWindow;
  pFuncs->newstream     = NPP_NewStream;
  pFuncs->destroystream = NPP_DestroyStream;
  pFuncs->asfile        = NPP_StreamAsFile;
  pFuncs->writeready    = NPP_WriteReady;
  pFuncs->write         = NPP_Write;
  pFuncs->print         = NPP_Print;
  pFuncs->event         = NPP_HandleEvent;
  pFuncs->urlnotify     = NPP_URLNotify;
  pFuncs->getvalue      = NPP_GetValue;
  pFuncs->setvalue      = NPP_SetValue;
  pFuncs->javaClass     = NULL;

  return NPERR_NO_ERROR;
}

NPError WINAPI NP_Initialize(NPNetscapeFuncs* pFuncs)
{
  
  if(!logger)
  {
    logger = NewLogger();
    if(logger)
    {
      logger->platformInit();
      logger->init();
    }
  }

  if(logger)
    logger->logNS_NP_Initialize();

  if(pFuncs == NULL)
    return NPERR_INVALID_FUNCTABLE_ERROR;

  if(HIBYTE(pFuncs->version) > NP_VERSION_MAJOR)
    return NPERR_INCOMPATIBLE_VERSION_ERROR;

  if(pFuncs->size < sizeof NPNetscapeFuncs)
    return NPERR_INVALID_FUNCTABLE_ERROR;

  NPNFuncs.size             = pFuncs->size;
  NPNFuncs.version          = pFuncs->version;
  NPNFuncs.geturlnotify     = pFuncs->geturlnotify;
  NPNFuncs.geturl           = pFuncs->geturl;
  NPNFuncs.posturlnotify    = pFuncs->posturlnotify;
  NPNFuncs.posturl          = pFuncs->posturl;
  NPNFuncs.requestread      = pFuncs->requestread;
  NPNFuncs.newstream        = pFuncs->newstream;
  NPNFuncs.write            = pFuncs->write;
  NPNFuncs.destroystream    = pFuncs->destroystream;
  NPNFuncs.status           = pFuncs->status;
  NPNFuncs.uagent           = pFuncs->uagent;
  NPNFuncs.memalloc         = pFuncs->memalloc;
  NPNFuncs.memfree          = pFuncs->memfree;
  NPNFuncs.memflush         = pFuncs->memflush;
  NPNFuncs.reloadplugins    = pFuncs->reloadplugins;
  NPNFuncs.getJavaEnv       = pFuncs->getJavaEnv;
  NPNFuncs.getJavaPeer      = pFuncs->getJavaPeer;
  NPNFuncs.getvalue         = pFuncs->getvalue;
  NPNFuncs.setvalue         = pFuncs->setvalue;
  NPNFuncs.invalidaterect   = pFuncs->invalidaterect;
  NPNFuncs.invalidateregion = pFuncs->invalidateregion;
  NPNFuncs.forceredraw      = pFuncs->forceredraw;

  
  epManager = new NPPEntryPointManager();
  if(!epManager)
    return NPERR_GENERIC_ERROR;

  return NPERR_NO_ERROR;
}

NPError WINAPI NP_Shutdown()
{
  
  if(epManager)
    epManager->callNP_ShutdownAll(); 

  if(logger)
  {
    logger->shut();
    logger->platformShut();
    DeleteLogger(logger);
    logger = NULL;
  }

  delete epManager;

  return NPERR_NO_ERROR;
}
