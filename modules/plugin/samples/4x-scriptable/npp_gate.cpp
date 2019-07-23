









































#include "plugin.h"

NPError NPP_Initialize(void)
{
    return NPERR_NO_ERROR;
}

void NPP_Shutdown(void)
{
}




NPError NPP_New(NPMIMEType pluginType,
                NPP instance,
                uint16 mode,
                int16 argc,
                char* argn[],
                char* argv[],
                NPSavedData* saved)
{   
  if(instance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  NPError rv = NPERR_NO_ERROR;

  CPlugin * pPlugin = new CPlugin(instance);
  if(pPlugin == NULL)
    return NPERR_OUT_OF_MEMORY_ERROR;

  instance->pdata = (void *)pPlugin;
  return rv;
}


NPError NPP_Destroy (NPP instance, NPSavedData** save)
{
  if(instance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  NPError rv = NPERR_NO_ERROR;

  CPlugin * pPlugin = (CPlugin *)instance->pdata;
  if(pPlugin != NULL) {
    pPlugin->shut();
    delete pPlugin;
  }
  return rv;
}




NPError NPP_SetWindow (NPP instance, NPWindow* pNPWindow)
{    
  if(instance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  NPError rv = NPERR_NO_ERROR;

  if(pNPWindow == NULL)
    return NPERR_GENERIC_ERROR;

  CPlugin * pPlugin = (CPlugin *)instance->pdata;

  if(pPlugin == NULL) 
    return NPERR_GENERIC_ERROR;

  
  if(!pPlugin->isInitialized() && (pNPWindow->window != NULL)) { 
    if(!pPlugin->init(pNPWindow)) {
      delete pPlugin;
      pPlugin = NULL;
      return NPERR_MODULE_LOAD_FAILED_ERROR;
    }
  }

  
  if((pNPWindow->window == NULL) && pPlugin->isInitialized())
    return NPERR_NO_ERROR;

  
  if(pPlugin->isInitialized() && (pNPWindow->window != NULL))
    return NPERR_NO_ERROR;

  
  if((pNPWindow->window == NULL) && !pPlugin->isInitialized())
    return NPERR_NO_ERROR;

  return rv;
}










NPError	NPP_GetValue(NPP instance, NPPVariable variable, void *value)
{
  if(instance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  NPError rv = NPERR_NO_ERROR;

  if(instance == NULL)
    return NPERR_GENERIC_ERROR;

  CPlugin * pPlugin = (CPlugin *)instance->pdata;
  if(pPlugin == NULL)
    return NPERR_GENERIC_ERROR;

  if (variable == NPPVpluginScriptableInstance) {
    
    nsI4xScriptablePlugin * scriptablePeer = pPlugin->getScriptablePeer();
    if (scriptablePeer) {
      *(nsISupports **)value = scriptablePeer;
    } else {
      rv = NPERR_OUT_OF_MEMORY_ERROR;
    }
  }
  else if (variable == NPPVpluginScriptableIID) {
    static nsIID scriptableIID = NS_I4XSCRIPTABLEPLUGIN_IID;
    nsIID* ptr = (nsIID *)NPN_MemAlloc(sizeof(nsIID));
    if (ptr) {
        *ptr = scriptableIID;
        *(nsIID **)value = ptr;
    } else {
      rv = NPERR_OUT_OF_MEMORY_ERROR;
    }
  }

  return rv;
}

NPError NPP_NewStream(NPP instance,
                      NPMIMEType type,
                      NPStream* stream, 
                      NPBool seekable,
                      uint16* stype)
{
  if(instance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  NPError rv = NPERR_NO_ERROR;
  return rv;
}

int32 NPP_WriteReady (NPP instance, NPStream *stream)
{
  if(instance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  int32 rv = 0x0fffffff;
  return rv;
}

int32 NPP_Write (NPP instance, NPStream *stream, int32 offset, int32 len, void *buffer)
{   
  if(instance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  int32 rv = len;
  return rv;
}

NPError NPP_DestroyStream (NPP instance, NPStream *stream, NPError reason)
{
  if(instance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  NPError rv = NPERR_NO_ERROR;
  return rv;
}

void NPP_StreamAsFile (NPP instance, NPStream* stream, const char* fname)
{
  if(instance == NULL)
    return;
}

void NPP_Print (NPP instance, NPPrint* printInfo)
{
  if(instance == NULL)
    return;
}

void NPP_URLNotify(NPP instance, const char* url, NPReason reason, void* notifyData)
{
  if(instance == NULL)
    return;
}

NPError NPP_SetValue(NPP instance, NPNVariable variable, void *value)
{
  if(instance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  NPError rv = NPERR_NO_ERROR;
  return rv;
}

int16	NPP_HandleEvent(NPP instance, void* event)
{
  if(instance == NULL)
    return 0;

  int16 rv = 0;
  CPlugin * pPlugin = (CPlugin *)instance->pdata;
  if (pPlugin)
    rv = pPlugin->handleEvent(event);

  return rv;
}

jref NPP_GetJavaClass (void)
{
  return NULL;
}
