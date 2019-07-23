






































#include "pluginbase.h"




NPError NPP_New(NPMIMEType pluginType, NPP instance, uint16 mode, int16 argc, char* argn[], char* argv[], NPSavedData* saved)
{   
  if (!instance)
    return NPERR_INVALID_INSTANCE_ERROR;

  
  
  nsPluginCreateData ds;
  
  ds.instance = instance;
  ds.type     = pluginType; 
  ds.mode     = mode; 
  ds.argc     = argc; 
  ds.argn     = argn; 
  ds.argv     = argv; 
  ds.saved    = saved;

  nsPluginInstanceBase * plugin = NS_NewPluginInstance(&ds);
  if (!plugin)
    return NPERR_OUT_OF_MEMORY_ERROR;

  
  instance->pdata = (void *)plugin;
  return NPERR_NO_ERROR;
}


NPError NPP_Destroy (NPP instance, NPSavedData** save)
{
  if (!instance)
    return NPERR_INVALID_INSTANCE_ERROR;

  nsPluginInstanceBase * plugin = (nsPluginInstanceBase *)instance->pdata;
  if (plugin) {
    plugin->shut();
    NS_DestroyPluginInstance(plugin);
  }
  return NPERR_NO_ERROR;
}




NPError NPP_SetWindow (NPP instance, NPWindow* pNPWindow)
{    
  if (!instance)
    return NPERR_INVALID_INSTANCE_ERROR;

  if (!pNPWindow)
    return NPERR_GENERIC_ERROR;

  nsPluginInstanceBase * plugin = (nsPluginInstanceBase *)instance->pdata;

  if (!plugin) 
    return NPERR_GENERIC_ERROR;

  
  if (!plugin->isInitialized() && pNPWindow->window) { 
    if (!plugin->init(pNPWindow)) {
      NS_DestroyPluginInstance(plugin);
      return NPERR_MODULE_LOAD_FAILED_ERROR;
    }
  }

  
  if (!pNPWindow->window && plugin->isInitialized())
    return plugin->SetWindow(pNPWindow);

  
  if (plugin->isInitialized() && pNPWindow->window)
    return plugin->SetWindow(pNPWindow);

  
  if (!pNPWindow->window && !plugin->isInitialized())
    return plugin->SetWindow(pNPWindow);

  return NPERR_NO_ERROR;
}

NPError NPP_NewStream(NPP instance, NPMIMEType type, NPStream* stream, NPBool seekable, uint16* stype)
{
  if (!instance)
    return NPERR_INVALID_INSTANCE_ERROR;

  nsPluginInstanceBase * plugin = (nsPluginInstanceBase *)instance->pdata;
  if (!plugin) 
    return NPERR_GENERIC_ERROR;

  return plugin->NewStream(type, stream, seekable, stype);
}

int32 NPP_WriteReady (NPP instance, NPStream *stream)
{
  if (!instance)
    return 0x0fffffff;

  nsPluginInstanceBase * plugin = (nsPluginInstanceBase *)instance->pdata;
  if (!plugin) 
    return 0x0fffffff;

  return plugin->WriteReady(stream);
}

int32 NPP_Write (NPP instance, NPStream *stream, int32 offset, int32 len, void *buffer)
{   
  if (!instance)
    return len;

  nsPluginInstanceBase * plugin = (nsPluginInstanceBase *)instance->pdata;
  if (!plugin) 
    return len;

  return plugin->Write(stream, offset, len, buffer);
}

NPError NPP_DestroyStream (NPP instance, NPStream *stream, NPError reason)
{
  if (!instance)
    return NPERR_INVALID_INSTANCE_ERROR;

  nsPluginInstanceBase * plugin = (nsPluginInstanceBase *)instance->pdata;
  if (!plugin) 
    return NPERR_GENERIC_ERROR;

  return plugin->DestroyStream(stream, reason);
}

void NPP_StreamAsFile (NPP instance, NPStream* stream, const char* fname)
{
  if (!instance)
    return;

  nsPluginInstanceBase * plugin = (nsPluginInstanceBase *)instance->pdata;
  if (!plugin) 
    return;

  plugin->StreamAsFile(stream, fname);
}

void NPP_Print (NPP instance, NPPrint* printInfo)
{
  if (!instance)
    return;

  nsPluginInstanceBase * plugin = (nsPluginInstanceBase *)instance->pdata;
  if (!plugin) 
    return;

  plugin->Print(printInfo);
}

void NPP_URLNotify(NPP instance, const char* url, NPReason reason, void* notifyData)
{
  if (!instance)
    return;

  nsPluginInstanceBase * plugin = (nsPluginInstanceBase *)instance->pdata;
  if (!plugin) 
    return;

  plugin->URLNotify(url, reason, notifyData);
}

NPError	NPP_GetValue(NPP instance, NPPVariable variable, void *value)
{
  if (!instance)
    return NPERR_INVALID_INSTANCE_ERROR;

  nsPluginInstanceBase * plugin = (nsPluginInstanceBase *)instance->pdata;
  if (!plugin) 
    return NPERR_GENERIC_ERROR;

  return plugin->GetValue(variable, value);
}

NPError NPP_SetValue(NPP instance, NPNVariable variable, void *value)
{
  if (!instance)
    return NPERR_INVALID_INSTANCE_ERROR;

  nsPluginInstanceBase * plugin = (nsPluginInstanceBase *)instance->pdata;
  if (!plugin) 
    return NPERR_GENERIC_ERROR;

  return plugin->SetValue(variable, value);
}

int16	NPP_HandleEvent(NPP instance, void* event)
{
  if (!instance)
    return 0;

  nsPluginInstanceBase * plugin = (nsPluginInstanceBase *)instance->pdata;
  if (!plugin) 
    return 0;

  return plugin->HandleEvent(event);
}
