




































#include <windows.h>
#include <assert.h>

#include "resource.h"

#include "plugin.h" 
#include "utils.h"
#include "dbg.h"

char szAppName[] = "NPNULL";




NPError NPP_Initialize(void)
{
  RegisterNullPluginWindowClass();
  return NPERR_NO_ERROR;
}




void NPP_Shutdown(void)
{
  UnregisterNullPluginWindowClass();
}




NPError NP_LOADDS NPP_New(NPMIMEType pluginType,
                          NPP pInstance,
                          uint16 mode,
                          int16 argc,
                          char* argn[],
                          char* argv[],
                          NPSavedData* saved)
{
  if(pInstance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  
  char * szPageURL = NULL;
  char * szFileURL = NULL;
  char * szFileExtension = NULL;
  char * buf = NULL;
  BOOL bHidden = FALSE;

  for(int i = 0; i < argc; i++)
  {
    if(lstrcmpi(argn[i],"pluginspage") == 0 && argv[i] != NULL)
      szPageURL = (char *)argv[i];
    else if(lstrcmpi(argn[i],"codebase") == 0 && argv[i] != NULL)
      szPageURL = (char *)argv[i];
    else if(lstrcmpi(argn[i],"pluginurl") == 0 && argv[i] != NULL)
      szFileURL = (char *)argv[i];
    else if(lstrcmpi(argn[i],"classid") == 0 && argv[i] != NULL)
      szFileURL = (char *)argv[i];
    else if(lstrcmpi(argn[i],"SRC") == 0 && argv[i] != NULL)
      buf = (char *)argv[i];
    else if(lstrcmpi(argn[i],"HIDDEN") == 0 && argv[i] != NULL)
      bHidden = (lstrcmp((char *)argv[i], "TRUE") == 0);
  }

  
  if(buf != NULL)
  {
    buf = strrchr(buf, '.');
    if(buf)
      szFileExtension = ++buf;
  }

  CPlugin * pPlugin = new CPlugin(hInst, 
                                  pInstance, 
                                  mode, 
                                  pluginType, 
                                  szPageURL, 
                                  szFileURL, 
                                  szFileExtension,
                                  bHidden);
  if(pPlugin == NULL)
    return NPERR_OUT_OF_MEMORY_ERROR;

  if(bHidden)
  {
    if(!pPlugin->init(NULL))
    {
      delete pPlugin;
      pPlugin = NULL;
      return NPERR_MODULE_LOAD_FAILED_ERROR;
    }
  }

  pInstance->pdata = (void *)pPlugin;

  return NPERR_NO_ERROR;
}




NPError NP_LOADDS
NPP_Destroy(NPP pInstance, NPSavedData** save)
{
  dbgOut1("NPP_Destroy");
  if(pInstance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  CPlugin * pPlugin = (CPlugin *)pInstance->pdata;
  if(pPlugin != NULL)
  {
    pPlugin->shut();
    delete pPlugin;
  }

  return NPERR_NO_ERROR;
}




NPError NP_LOADDS NPP_SetWindow(NPP pInstance, NPWindow * pNPWindow)
{
  if(pInstance == NULL)
  {
    dbgOut1("NPP_SetWindow returns NPERR_INVALID_INSTANCE_ERROR");
    return NPERR_INVALID_INSTANCE_ERROR;
  }

  if(pNPWindow == NULL)
  {
    dbgOut1("NPP_SetWindow returns NPERR_GENERIC_ERROR");
    return NPERR_GENERIC_ERROR;
  }

  HWND hWnd = (HWND)(DWORD)pNPWindow->window;

  CPlugin * pPlugin = (CPlugin *)pInstance->pdata;
  assert(pPlugin != NULL);

  if(pPlugin == NULL) 
  {
    dbgOut1("NPP_SetWindow returns NPERR_GENERIC_ERROR");
    return NPERR_GENERIC_ERROR;
  }

  if((hWnd == NULL) && (pPlugin->getWindow() == NULL)) 
  {
    dbgOut1("NPP_SetWindow just returns with NPERR_NO_ERROR");
    return NPERR_NO_ERROR;
  }

  if((hWnd == NULL) && (pPlugin->getWindow() != NULL))
  { 
    dbgOut1("NPP_SetWindow, going away...");
    pPlugin->shut();
    return NPERR_NO_ERROR;
  }

  if((pPlugin->getWindow() == NULL) && (hWnd != NULL))
  { 
    dbgOut1("NPP_SetWindow, first time");

    if(!pPlugin->init(hWnd))
    {
      delete pPlugin;
      pPlugin = NULL;
      return NPERR_MODULE_LOAD_FAILED_ERROR;
    }
  }

  if((pPlugin->getWindow() != NULL) && (hWnd != NULL))
  { 
    dbgOut1("NPP_SetWindow, resizing");
    pPlugin->resize();
  }

  return NPERR_NO_ERROR;
}




NPError NP_LOADDS
NPP_NewStream(NPP pInstance,
              NPMIMEType type,
              NPStream *stream, 
              NPBool seekable,
              uint16 *stype)
{
  dbgOut1("NPP_NewStream");
  if(pInstance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  CPlugin * pPlugin = (CPlugin *)pInstance->pdata;
  assert(pPlugin != NULL);

  if (!pPlugin)
    return NPERR_GENERIC_ERROR;

  return pPlugin->newStream(type, stream, seekable, stype);
}




int32 NP_LOADDS
NPP_WriteReady(NPP pInstance, NPStream *stream)
{
  dbgOut1("NPP_WriteReady");
  if(pInstance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  CPlugin * pPlugin = (CPlugin *)pInstance->pdata;
  assert(pPlugin != NULL);

  
  NPN_DestroyStream(pInstance, stream, NPRES_DONE);

  return -1L;   
}




int32 NP_LOADDS
NPP_Write(NPP pInstance, NPStream *stream, int32 offset, int32 len, void *buffer)
{
  
  if(pInstance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  CPlugin * pPlugin = (CPlugin *)pInstance->pdata;
  assert(pPlugin != NULL);

  
  NPN_DestroyStream(pInstance, stream, NPRES_DONE);

  return -1;   
}




NPError NP_LOADDS
NPP_DestroyStream(NPP pInstance, NPStream *stream, NPError reason)
{
  dbgOut1("NPP_DestroyStream");
  if(pInstance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  CPlugin * pPlugin = (CPlugin *)pInstance->pdata;
  assert(pPlugin != NULL);

  if (!pPlugin)
    return NPERR_GENERIC_ERROR;

  return pPlugin->destroyStream(stream, reason);
}




void NP_LOADDS
NPP_StreamAsFile(NPP instance, NPStream *stream, const char* fname)
{
  dbgOut1("NPP_StreamAsFile");
}




void NP_LOADDS NPP_Print(NPP pInstance, NPPrint * printInfo)
{
  dbgOut2("NPP_Print, printInfo = %#08x", printInfo);

  CPlugin * pPlugin = (CPlugin *)pInstance->pdata;
  assert(pPlugin != NULL);

  pPlugin->print(printInfo);
}

void NP_LOADDS NPP_URLNotify(NPP pInstance, const char* url, NPReason reason, void* notifyData)
{
  dbgOut2("NPP_URLNotify, URL '%s'", url);

  CPlugin * pPlugin = (CPlugin *)pInstance->pdata;
  assert(pPlugin != NULL);

  pPlugin->URLNotify(url);
}

#ifdef OJI
jref NP_LOADDS NPP_GetJavaClass(void)
{
  return NULL;
}
#endif
