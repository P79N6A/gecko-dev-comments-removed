








































#include "plugbase.h"
#include "logger.h"

extern CLogger * pLogger;

static char szINIFile[] = NPAPI_INI_FILE_NAME;
static char szTarget[] = LOGGER_DEFAULT_TARGET;




NPError NPP_New(NPMIMEType pluginType, NPP instance, uint16 mode, int16 argc, char* argn[], char* argv[], NPSavedData* saved)
{   
  DWORD dwTickEnter = XP_GetTickCount();
  NPError ret = NPERR_NO_ERROR;
  CPluginBase * pPlugin = NULL;

  if(!instance) {
    ret = NPERR_INVALID_INSTANCE_ERROR;
    goto Return;
  }

  pPlugin = (CPluginBase *)CreatePlugin(instance, mode);

  if(!pPlugin) {
    ret = NPERR_OUT_OF_MEMORY_ERROR;
    goto Return;
  }

  instance->pdata = (void *)pPlugin;

  
  if (!pLogger)
    pLogger = new CLogger(szTarget);
  else if (pLogger->isStale()) {
    delete pLogger;
    pLogger = new CLogger(szTarget);
  }

  char szFileName[_MAX_PATH];
  pPlugin->getModulePath(szFileName, sizeof(szFileName));
  strcat(szFileName, szINIFile);
  pLogger->restorePreferences(szFileName);
  
  pLogger->associate(pPlugin);

  if (pPlugin->isStandAlone())
    pPlugin->initStandAlone();

Return:
  DWORD dwTickReturn = XP_GetTickCount();
  pLogger->appendToLog(action_npp_new, dwTickEnter, dwTickReturn, (DWORD)ret, 
                       (DWORD)pluginType, (DWORD)instance, 
                       (DWORD)mode, (DWORD)argc, (DWORD)argn, (DWORD)argv, (DWORD)saved);

  pPlugin->autoStartScriptIfNeeded();

  return ret;
}


NPError NPP_Destroy (NPP instance, NPSavedData** save)
{
  DWORD dwTickEnter = XP_GetTickCount();
  NPError ret = NPERR_NO_ERROR;
  CPluginBase * pPlugin = NULL;

  if(!instance) {
    ret = NPERR_INVALID_INSTANCE_ERROR;
    goto Return;
  }

  pPlugin = (CPluginBase *)instance->pdata;
  if(pPlugin) {
    if (pPlugin->isStandAlone())
      pPlugin->shutStandAlone();

    pPlugin->shut();
    DestroyPlugin(pPlugin);
    goto Return;
  }

Return:
  pLogger->blockDumpToFrame(TRUE);
  DWORD dwTickReturn = XP_GetTickCount();
  pLogger->appendToLog(action_npp_destroy, dwTickEnter, dwTickReturn, (DWORD)ret, (DWORD)instance, (DWORD)save);
  pLogger->blockDumpToFrame(FALSE);

  
  
  pLogger->markStale();
  return ret;
}




NPError NPP_SetWindow (NPP instance, NPWindow* pNPWindow)
{    
  DWORD dwTickEnter = XP_GetTickCount();
  CPluginBase * pPlugin = NULL;
  NPError ret = NPERR_NO_ERROR;

  if(!instance ) {
    ret = NPERR_INVALID_INSTANCE_ERROR;
    goto Return;
  }

  if(!pNPWindow) {
    ret = NPERR_INVALID_INSTANCE_ERROR;
    goto Return;
  }

  pPlugin = (CPluginBase *)instance->pdata;

  if(!pPlugin) {
    ret = NPERR_GENERIC_ERROR;
    goto Return;
  }

  if (!pPlugin->isStandAlone())
  {
    
    if(!pPlugin->isInitialized() && pNPWindow->window) { 
      if(!pPlugin->init((DWORD)pNPWindow->window)) {
        delete pPlugin;
        pPlugin = NULL;
        ret = NPERR_MODULE_LOAD_FAILED_ERROR;
        goto Return;
      }

      if(pLogger->getShowImmediatelyFlag()) {
        pLogger->dumpLogToTarget();
        pLogger->clearLog();
      }
      goto Return;
    }

    
    if(!pNPWindow->window && pPlugin->isInitialized()) {
      pPlugin->shut();
      ret = NPERR_NO_ERROR;
      goto Return;
    }

    
    if(pPlugin->isInitialized() && pNPWindow->window) {
      ret = NPERR_NO_ERROR;
      goto Return;
    }

    
    if(!pNPWindow->window && !pPlugin->isInitialized()) {
      ret = NPERR_NO_ERROR;
      goto Return;
    }
  }

Return:
  DWORD dwTickReturn = XP_GetTickCount();
  pLogger->appendToLog(action_npp_set_window, dwTickEnter, dwTickReturn, (DWORD)ret, (DWORD)instance, (DWORD)pNPWindow);
  return ret;
}

NPError NPP_NewStream(NPP instance, NPMIMEType type, NPStream* stream, NPBool seekable, uint16* stype)
{
  DWORD dwTickEnter = XP_GetTickCount();
  CPluginBase * pPlugin = NULL;
  NPError ret = NPERR_NO_ERROR;

  if(!instance) {
    ret = NPERR_INVALID_INSTANCE_ERROR;
    goto Return;
  }

  pPlugin = (CPluginBase *)instance->pdata;
  pPlugin->onNPP_NewStream(instance, (LPSTR)type, stream, seekable, stype);

Return:
  DWORD dwTickReturn = XP_GetTickCount();
  pLogger->appendToLog(action_npp_new_stream, dwTickEnter, dwTickReturn, (DWORD)ret, (DWORD)instance, 
                       (DWORD)type, (DWORD)stream, (DWORD)seekable, (DWORD)stype);
  if (pPlugin->m_firstAction == action_npn_request_read && seekable) {
    *stype = NP_SEEK;
  }
  return ret;
}

int32 NPP_WriteReady (NPP instance, NPStream *stream)
{
  DWORD dwTickEnter = XP_GetTickCount();
  CPluginBase * pPlugin = NULL;
  int32 ret = 0x0FFFFFFF;

  if(!instance) {
    ret = 0L;
    goto Return;
  }

  pPlugin = (CPluginBase *)instance->pdata;

Return:
  DWORD dwTickReturn = XP_GetTickCount();
  pLogger->appendToLog(action_npp_write_ready, dwTickEnter, dwTickReturn, (DWORD)ret, 
                       (DWORD)instance, (DWORD)stream);
  return ret;
}

int32 NPP_Write (NPP instance, NPStream *stream, int32 offset, int32 len, void *buffer)
{   
  DWORD dwTickEnter = XP_GetTickCount();
  CPluginBase * pPlugin = NULL;
  int32 ret = len;

  if(!instance)
    goto Return;
  
  pPlugin = (CPluginBase *)instance->pdata;

Return:
  DWORD dwTickReturn = XP_GetTickCount();
  pLogger->appendToLog(action_npp_write, dwTickEnter, dwTickReturn, (DWORD)ret, 
                       (DWORD)instance, (DWORD)stream, 
                       (DWORD)offset, (DWORD)len, (DWORD)buffer);
  if (pPlugin->m_firstAction == action_npn_request_read) {
    if (stream->notifyData) {
      NPByteRange* rangeList = 	(NPByteRange*) stream->notifyData;
      NPN_RequestRead(stream, rangeList);
      stream->notifyData = 0;
    }
  }
  return ret;
}

NPError NPP_DestroyStream (NPP instance, NPStream *stream, NPError reason)
{
  DWORD dwTickEnter = XP_GetTickCount();
  CPluginBase * pPlugin = NULL;
  NPError ret = NPERR_NO_ERROR;

  if(!instance) {
    ret = NPERR_INVALID_INSTANCE_ERROR;
    goto Return;
  }

  pPlugin = (CPluginBase *)instance->pdata;

  pPlugin->onNPP_DestroyStream(stream);
  if(pLogger->onNPP_DestroyStream(stream))
    return ret;

Return:
  DWORD dwTickReturn = XP_GetTickCount();
  pLogger->appendToLog(action_npp_destroy_stream, dwTickEnter, dwTickReturn, (DWORD)ret, 
                       (DWORD)instance, (DWORD)stream, (DWORD)reason);
  return ret;
}

void NPP_StreamAsFile (NPP instance, NPStream* stream, const char* fname)
{
  DWORD dwTickEnter = XP_GetTickCount();
  CPluginBase * pPlugin = NULL;

  if(!instance)
    goto Return;

  pPlugin = (CPluginBase *)instance->pdata;

  pPlugin->onNPP_StreamAsFile(instance, stream, fname);

Return:
  DWORD dwTickReturn = XP_GetTickCount();
  pLogger->appendToLog(action_npp_stream_as_file, dwTickEnter, dwTickReturn, 0L, 
                       (DWORD)instance, (DWORD)stream, (DWORD)fname);
}

void NPP_Print (NPP instance, NPPrint* printInfo)
{
  DWORD dwTickEnter = XP_GetTickCount();
  CPluginBase * pPlugin = NULL;

  if(!instance)
    goto Return;

  pPlugin = (CPluginBase *)instance->pdata;

Return:
  DWORD dwTickReturn = XP_GetTickCount();
  pLogger->appendToLog(action_npp_print, dwTickEnter, dwTickReturn, 0L, 
                       (DWORD)instance, (DWORD)printInfo);
}

void NPP_URLNotify(NPP instance, const char* url, NPReason reason, void* notifyData)
{
  DWORD dwTickEnter = XP_GetTickCount();
  CPluginBase * pPlugin = NULL;

  if(!instance)
    goto Return;

  pPlugin = (CPluginBase *)instance->pdata;

Return:
  DWORD dwTickReturn = XP_GetTickCount();
  pLogger->appendToLog(action_npp_url_notify, dwTickEnter, dwTickReturn, 0L, 
                       (DWORD)instance, (DWORD)url, (DWORD)reason, (DWORD)notifyData);
}

NPError	NPP_GetValue(NPP instance, NPPVariable variable, void *value)
{
  DWORD dwTickEnter = XP_GetTickCount();
  CPluginBase * pPlugin = NULL;
  NPError ret = NPERR_NO_ERROR;

  if(!instance) {
    ret = NPERR_INVALID_INSTANCE_ERROR;
    goto Return;
  }

  pPlugin = (CPluginBase *)instance->pdata;

#ifdef XP_UNIX
  switch (variable) {
    case NPPVpluginNameString:   
      *((char **)value) = "API Tester plugin";
      break;
    case NPPVpluginDescriptionString:
      *((char **)value) = "This plugins reads and executes test scripts.";
      break;
    default:
      ret = NPERR_GENERIC_ERROR;
  }
#endif 

Return:
  DWORD dwTickReturn = XP_GetTickCount();
  pLogger->appendToLog(action_npp_get_value, dwTickEnter, dwTickReturn, (DWORD)ret, 
                       (DWORD)instance, (DWORD)variable, (DWORD)value);
  return ret;
}

NPError NPP_SetValue(NPP instance, NPNVariable variable, void *value)
{
  DWORD dwTickEnter = XP_GetTickCount();
  CPluginBase * pPlugin = NULL;
  NPError ret = NPERR_NO_ERROR;

  if(!instance) {
    ret = NPERR_INVALID_INSTANCE_ERROR;
    goto Return;
  }

  pPlugin = (CPluginBase *)instance->pdata;

Return:
  DWORD dwTickReturn = XP_GetTickCount();
  pLogger->appendToLog(action_npp_set_value, dwTickEnter, dwTickReturn, (DWORD)ret, 
                       (DWORD)instance, (DWORD)variable, (DWORD)value);
  return ret;
}

int16	NPP_HandleEvent(NPP instance, void* event)
{
  DWORD dwTickEnter = XP_GetTickCount();
  CPluginBase * pPlugin = NULL;
  int16 ret = (int16)TRUE;

  if(!instance)
    goto Return;

  pPlugin = (CPluginBase *)instance->pdata;

Return:
  DWORD dwTickReturn = XP_GetTickCount();
  pLogger->appendToLog(action_npp_handle_event, dwTickEnter, dwTickReturn, (DWORD)ret, 
                       (DWORD)instance, (DWORD)event);
  return ret;
}

jref NPP_GetJavaClass (void)
{
  DWORD dwTickEnter = XP_GetTickCount();
  DWORD dwTickReturn = XP_GetTickCount();
  if(pLogger)
    pLogger->appendToLog(action_npp_get_java_class, dwTickEnter, dwTickReturn, 0L);
  return NULL;
}









#ifdef XP_MAC

NPError	Private_New(NPMIMEType pluginType, NPP instance, uint16 mode, int16 argc, char* argn[], char* argv[], NPSavedData* saved)
{
  EnterCodeResource();
  NPError rv = NPP_New(pluginType, instance, mode, argc, argn, argv, saved);
  ExitCodeResource();
  return rv;	
}

NPError Private_Destroy(NPP instance, NPSavedData** save)
{
  EnterCodeResource();
  NPError rv = NPP_Destroy(instance, save);
  ExitCodeResource();
  return rv;
}

NPError Private_SetWindow(NPP instance, NPWindow* window)
{
  EnterCodeResource();
  NPError rv = NPP_SetWindow(instance, window);
  ExitCodeResource();
  return rv;
}

NPError Private_NewStream(NPP instance, NPMIMEType type, NPStream* stream, NPBool seekable, uint16* stype)
{
  EnterCodeResource();
  NPError rv = NPP_NewStream(instance, type, stream, seekable, stype);
  ExitCodeResource();
  return rv;
}

int32 Private_WriteReady(NPP instance, NPStream* stream)
{
  EnterCodeResource();
  int32 rv = NPP_WriteReady(instance, stream);
  ExitCodeResource();
  return rv;
}

int32 Private_Write(NPP instance, NPStream* stream, int32 offset, int32 len, void* buffer)
{
  EnterCodeResource();
  int32 rv = NPP_Write(instance, stream, offset, len, buffer);
  ExitCodeResource();
  return rv;
}

void Private_StreamAsFile(NPP instance, NPStream* stream, const char* fname)
{
  EnterCodeResource();
  NPP_StreamAsFile(instance, stream, fname);
  ExitCodeResource();
}


NPError Private_DestroyStream(NPP instance, NPStream* stream, NPError reason)
{
  EnterCodeResource();
  NPError rv = NPP_DestroyStream(instance, stream, reason);
  ExitCodeResource();
  return rv;
}

int16 Private_HandleEvent(NPP instance, void* event)
{
  EnterCodeResource();
  int16 rv = NPP_HandleEvent(instance, event);
  ExitCodeResource();
  return rv;
}

void Private_Print(NPP instance, NPPrint* platformPrint)
{
  EnterCodeResource();
  NPP_Print(instance, platformPrint);
  ExitCodeResource();
}

void Private_URLNotify(NPP instance, const char* url, NPReason reason, void* notifyData)
{
  EnterCodeResource();
  NPP_URLNotify(instance, url, reason, notifyData);
  ExitCodeResource();
}

jref Private_GetJavaClass(void)
{
  return NULL;
}

NPError Private_GetValue(NPP instance, NPPVariable variable, void *result)
{
  EnterCodeResource();
  NPError rv = NPP_GetValue(instance, variable, result);
  ExitCodeResource();
  return rv;
}

NPError Private_SetValue(NPP instance, NPNVariable variable, void *value)
{
  EnterCodeResource();
  NPError rv = NPP_SetValue(instance, variable, value);
  ExitCodeResource();
  return rv;
}

#endif 
