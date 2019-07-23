




















































#define XP_UNIX 1

#include <stdio.h>
#include "npapi.h"
#include "npfunctions.h"






#ifdef PLUGIN_TRACE
#include <stdio.h>
#define PLUGINDEBUGSTR(msg) fprintf(stderr, "%s\n", msg)
#else
#define PLUGINDEBUGSTR(msg)
#endif








static NPNetscapeFuncs   gNetscapeFuncs;    












void
NPN_Version(int* plugin_major, int* plugin_minor,
         int* netscape_major, int* netscape_minor)
{
    *plugin_major = NP_VERSION_MAJOR;
    *plugin_minor = NP_VERSION_MINOR;

    
    *netscape_major = gNetscapeFuncs.version >> 8;
    
    *netscape_minor = gNetscapeFuncs.version & 0xFF;
}

NPError
NPN_GetValue(NPP instance, NPNVariable variable, void *r_value)
{
    return (*gNetscapeFuncs.getvalue)(instance, variable, r_value);
}

NPError
NPN_SetValue(NPP instance, NPPVariable variable, void *value)
{
    return (*gNetscapeFuncs.setvalue)(instance, variable, value);
}

NPError
NPN_GetURL(NPP instance, const char* url, const char* window)
{
    return (*gNetscapeFuncs.geturl)(instance, url, window);
}

NPError
NPN_GetURLNotify(NPP instance, const char* url, const char* window, void* notifyData)
{
    return (*gNetscapeFuncs.geturlnotify)(instance, url, window, notifyData);
}

NPError
NPN_PostURL(NPP instance, const char* url, const char* window,
         uint32_t len, const char* buf, NPBool file)
{
    return (*gNetscapeFuncs.posturl)(instance, url, window, len, buf, file);
}

NPError
NPN_PostURLNotify(NPP instance, const char* url, const char* window, uint32_t len,
                  const char* buf, NPBool file, void* notifyData)
{
    return (*gNetscapeFuncs.posturlnotify)(instance, url, window, len, buf, file, notifyData);
}

NPError
NPN_RequestRead(NPStream* stream, NPByteRange* rangeList)
{
    return (*gNetscapeFuncs.requestread)(stream, rangeList);
}

NPError
NPN_NewStream(NPP instance, NPMIMEType type, const char *window,
          NPStream** stream_ptr)
{
    return (*gNetscapeFuncs.newstream)(instance, type, window, stream_ptr);
}

int32_t
NPN_Write(NPP instance, NPStream* stream, int32_t len, void* buffer)
{
    return (*gNetscapeFuncs.write)(instance, stream, len, buffer);
}

NPError
NPN_DestroyStream(NPP instance, NPStream* stream, NPError reason)
{
    return (*gNetscapeFuncs.destroystream)(instance, stream, reason);
}

void
NPN_Status(NPP instance, const char* message)
{
    (*gNetscapeFuncs.status)(instance, message);
}

const char*
NPN_UserAgent(NPP instance)
{
    return (*gNetscapeFuncs.uagent)(instance);
}

void*
NPN_MemAlloc(uint32_t size)
{
    return (*gNetscapeFuncs.memalloc)(size);
}

void NPN_MemFree(void* ptr)
{
    (*gNetscapeFuncs.memfree)(ptr);
}

uint32_t NPN_MemFlush(uint32_t size)
{
    return (*gNetscapeFuncs.memflush)(size);
}

void NPN_ReloadPlugins(NPBool reloadPages)
{
    (*gNetscapeFuncs.reloadplugins)(reloadPages);
}

void
NPN_InvalidateRect(NPP instance, NPRect *invalidRect)
{
    (*gNetscapeFuncs.invalidaterect)(instance, invalidRect);
}

void
NPN_InvalidateRegion(NPP instance, NPRegion invalidRegion)
{
    (*gNetscapeFuncs.invalidateregion)(instance, invalidRegion);
}

void
NPN_ForceRedraw(NPP instance)
{
    (*gNetscapeFuncs.forceredraw)(instance);
}

void NPN_PushPopupsEnabledState(NPP instance, NPBool enabled)
{
    (*gNetscapeFuncs.pushpopupsenabledstate)(instance, enabled);
}

void NPN_PopPopupsEnabledState(NPP instance)
{
    (*gNetscapeFuncs.poppopupsenabledstate)(instance);
}

NPIdentifier NPN_GetStringIdentifier(const NPUTF8 *name)
{
    return (*gNetscapeFuncs.getstringidentifier)(name);
}

void NPN_GetStringIdentifiers(const NPUTF8 **names, int32_t nameCount,
                              NPIdentifier *identifiers)
{
    (*gNetscapeFuncs.getstringidentifiers)(names, nameCount, identifiers);
}

NPIdentifier NPN_GetIntIdentifier(int32_t intid)
{
    return (*gNetscapeFuncs.getintidentifier)(intid);
}

bool NPN_IdentifierIsString(NPIdentifier identifier)
{
    return (*gNetscapeFuncs.identifierisstring)(identifier);
}

NPUTF8 *NPN_UTF8FromIdentifier(NPIdentifier identifier)
{
    return (*gNetscapeFuncs.utf8fromidentifier)(identifier);
}

int32_t NPN_IntFromIdentifier(NPIdentifier identifier)
{
    return (*gNetscapeFuncs.intfromidentifier)(identifier);
}

NPObject *NPN_CreateObject(NPP npp, NPClass *aClass)
{
    return (*gNetscapeFuncs.createobject)(npp, aClass);
}

NPObject *NPN_RetainObject(NPObject *obj)
{
    return (*gNetscapeFuncs.retainobject)(obj);
}

void NPN_ReleaseObject(NPObject *obj)
{
    (*gNetscapeFuncs.releaseobject)(obj);
}

bool NPN_Invoke(NPP npp, NPObject* obj, NPIdentifier methodName,
                const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    return (*gNetscapeFuncs.invoke)(npp, obj, methodName, args, argCount, result);
}

bool NPN_InvokeDefault(NPP npp, NPObject* obj, const NPVariant *args,
                       uint32_t argCount, NPVariant *result)
{
    return (*gNetscapeFuncs.invokeDefault)(npp, obj, args, argCount, result);
}

bool NPN_Evaluate(NPP npp, NPObject* obj, NPString *script,
                  NPVariant *result)
{
    return (*gNetscapeFuncs.evaluate)(npp, obj, script, result);
}

bool NPN_GetProperty(NPP npp, NPObject* obj, NPIdentifier propertyName,
                     NPVariant *result)
{
    return (*gNetscapeFuncs.getproperty)(npp, obj, propertyName, result);
}

bool NPN_SetProperty(NPP npp, NPObject* obj, NPIdentifier propertyName,
                     const NPVariant *value)
{
    return (*gNetscapeFuncs.setproperty)(npp, obj, propertyName, value);
}

bool NPN_RemoveProperty(NPP npp, NPObject* obj, NPIdentifier propertyName)
{
    return (*gNetscapeFuncs.removeproperty)(npp, obj, propertyName);
}

bool NPN_HasProperty(NPP npp, NPObject* obj, NPIdentifier propertyName)
{
    return (*gNetscapeFuncs.hasproperty)(npp, obj, propertyName);
}

bool NPN_HasMethod(NPP npp, NPObject* obj, NPIdentifier methodName)
{
    return (*gNetscapeFuncs.hasmethod)(npp, obj, methodName);
}

void NPN_ReleaseVariantValue(NPVariant *variant)
{
    (*gNetscapeFuncs.releasevariantvalue)(variant);
}

void NPN_SetException(NPObject* obj, const NPUTF8 *message)
{
    (*gNetscapeFuncs.setexception)(obj, message);
}













NPError
Private_New(NPMIMEType pluginType, NPP instance, uint16_t mode,
        int16_t argc, char* argn[], char* argv[], NPSavedData* saved)
{
    NPError ret;
    PLUGINDEBUGSTR("New");
    ret = NPP_New(pluginType, instance, mode, argc, argn, argv, saved);
    return ret; 
}

NPError
Private_Destroy(NPP instance, NPSavedData** save)
{
    PLUGINDEBUGSTR("Destroy");
    return NPP_Destroy(instance, save);
}

NPError
Private_SetWindow(NPP instance, NPWindow* window)
{
    NPError err;
    PLUGINDEBUGSTR("SetWindow");
    err = NPP_SetWindow(instance, window);
    return err;
}

NPError
Private_NewStream(NPP instance, NPMIMEType type, NPStream* stream,
            NPBool seekable, uint16_t* stype)
{
    NPError err;
    PLUGINDEBUGSTR("NewStream");
    err = NPP_NewStream(instance, type, stream, seekable, stype);
    return err;
}

int32_t
Private_WriteReady(NPP instance, NPStream* stream)
{
    unsigned int result;
    PLUGINDEBUGSTR("WriteReady");
    result = NPP_WriteReady(instance, stream);
    return result;
}

int32_t
Private_Write(NPP instance, NPStream* stream, int32_t offset, int32_t len,
        void* buffer)
{
    unsigned int result;
    PLUGINDEBUGSTR("Write");
    result = NPP_Write(instance, stream, offset, len, buffer);
    return result;
}

void
Private_StreamAsFile(NPP instance, NPStream* stream, const char* fname)
{
    PLUGINDEBUGSTR("StreamAsFile");
    NPP_StreamAsFile(instance, stream, fname);
}


NPError
Private_DestroyStream(NPP instance, NPStream* stream, NPError reason)
{
    NPError err;
    PLUGINDEBUGSTR("DestroyStream");
    err = NPP_DestroyStream(instance, stream, reason);
    return err;
}

void
Private_URLNotify(NPP instance, const char* url,
                NPReason reason, void* notifyData)
                
{
    PLUGINDEBUGSTR("URLNotify");
    NPP_URLNotify(instance, url, reason, notifyData);
}



void
Private_Print(NPP instance, NPPrint* platformPrint)
{
    PLUGINDEBUGSTR("Print");
    NPP_Print(instance, platformPrint);
}












char *
NP_GetPluginVersion(void)
{
    return "1.0.0.15";
}







char *
NP_GetMIMEDescription(void)
{
    return NPP_GetMIMEDescription();
}







NPError
NP_GetValue(void* future, NPPVariable variable, void *value)
{
    return NPP_GetValue(future, variable, value);
}


















NPError
NP_Initialize(NPNetscapeFuncs* nsTable, NPPluginFuncs* pluginFuncs)
{
    NPError err = NPERR_NO_ERROR;

    PLUGINDEBUGSTR("NP_Initialize");
    
    

    if ((nsTable == NULL) || (pluginFuncs == NULL))
        err = NPERR_INVALID_FUNCTABLE_ERROR;
    
    








    if (err == NPERR_NO_ERROR) {
        if ((nsTable->version >> 8) > NP_VERSION_MAJOR)
            err = NPERR_INCOMPATIBLE_VERSION_ERROR;
        if (nsTable->size < ((char *)&nsTable->posturlnotify - (char *)nsTable))
            err = NPERR_INVALID_FUNCTABLE_ERROR;
        if (pluginFuncs->size < sizeof(NPPluginFuncs))      
            err = NPERR_INVALID_FUNCTABLE_ERROR;
    }
        
    
    if (err == NPERR_NO_ERROR) {
        






        gNetscapeFuncs.version       = nsTable->version;
        gNetscapeFuncs.size          = nsTable->size;
        gNetscapeFuncs.posturl       = nsTable->posturl;
        gNetscapeFuncs.geturl        = nsTable->geturl;
        gNetscapeFuncs.geturlnotify  = nsTable->geturlnotify;
        gNetscapeFuncs.requestread   = nsTable->requestread;
        gNetscapeFuncs.newstream     = nsTable->newstream;
        gNetscapeFuncs.write         = nsTable->write;
        gNetscapeFuncs.destroystream = nsTable->destroystream;
        gNetscapeFuncs.status        = nsTable->status;
        gNetscapeFuncs.uagent        = nsTable->uagent;
        gNetscapeFuncs.memalloc      = nsTable->memalloc;
        gNetscapeFuncs.memfree       = nsTable->memfree;
        gNetscapeFuncs.memflush      = nsTable->memflush;
        gNetscapeFuncs.reloadplugins = nsTable->reloadplugins;
        gNetscapeFuncs.getvalue      = nsTable->getvalue;
        gNetscapeFuncs.setvalue      = nsTable->setvalue;
        gNetscapeFuncs.posturlnotify = nsTable->posturlnotify;

        if (nsTable->size >= ((char *)&nsTable->setexception - (char *)nsTable))
        {
          gNetscapeFuncs.invalidaterect = nsTable->invalidaterect;
          gNetscapeFuncs.invalidateregion = nsTable->invalidateregion;
          gNetscapeFuncs.forceredraw = nsTable->forceredraw;
          gNetscapeFuncs.getstringidentifier = nsTable->getstringidentifier;
          gNetscapeFuncs.getstringidentifiers = nsTable->getstringidentifiers;
          gNetscapeFuncs.getintidentifier = nsTable->getintidentifier;
          gNetscapeFuncs.identifierisstring = nsTable->identifierisstring;
          gNetscapeFuncs.utf8fromidentifier = nsTable->utf8fromidentifier;
          gNetscapeFuncs.intfromidentifier = nsTable->intfromidentifier;
          gNetscapeFuncs.createobject = nsTable->createobject;
          gNetscapeFuncs.retainobject = nsTable->retainobject;
          gNetscapeFuncs.releaseobject = nsTable->releaseobject;
          gNetscapeFuncs.invoke = nsTable->invoke;
          gNetscapeFuncs.invokeDefault = nsTable->invokeDefault;
          gNetscapeFuncs.evaluate = nsTable->evaluate;
          gNetscapeFuncs.getproperty = nsTable->getproperty;
          gNetscapeFuncs.setproperty = nsTable->setproperty;
          gNetscapeFuncs.removeproperty = nsTable->removeproperty;
          gNetscapeFuncs.hasproperty = nsTable->hasproperty;
          gNetscapeFuncs.hasmethod = nsTable->hasmethod;
          gNetscapeFuncs.releasevariantvalue = nsTable->releasevariantvalue;
          gNetscapeFuncs.setexception = nsTable->setexception;
        }
         else
        {
          gNetscapeFuncs.invalidaterect = NULL;
          gNetscapeFuncs.invalidateregion = NULL;
          gNetscapeFuncs.forceredraw = NULL;
          gNetscapeFuncs.getstringidentifier = NULL;
          gNetscapeFuncs.getstringidentifiers = NULL;
          gNetscapeFuncs.getintidentifier = NULL;
          gNetscapeFuncs.identifierisstring = NULL;
          gNetscapeFuncs.utf8fromidentifier = NULL;
          gNetscapeFuncs.intfromidentifier = NULL;
          gNetscapeFuncs.createobject = NULL;
          gNetscapeFuncs.retainobject = NULL;
          gNetscapeFuncs.releaseobject = NULL;
          gNetscapeFuncs.invoke = NULL;
          gNetscapeFuncs.invokeDefault = NULL;
          gNetscapeFuncs.evaluate = NULL;
          gNetscapeFuncs.getproperty = NULL;
          gNetscapeFuncs.setproperty = NULL;
          gNetscapeFuncs.removeproperty = NULL;
          gNetscapeFuncs.hasproperty = NULL;
          gNetscapeFuncs.releasevariantvalue = NULL;
          gNetscapeFuncs.setexception = NULL;
        }
        if (nsTable->size >=
            ((char *)&nsTable->poppopupsenabledstate - (char *)nsTable))
        {
          gNetscapeFuncs.pushpopupsenabledstate = nsTable->pushpopupsenabledstate;
          gNetscapeFuncs.poppopupsenabledstate  = nsTable->poppopupsenabledstate;
        }
         else
        {
          gNetscapeFuncs.pushpopupsenabledstate = NULL;
          gNetscapeFuncs.poppopupsenabledstate  = NULL;
        }

        





        pluginFuncs->version    = (NP_VERSION_MAJOR << 8) + NP_VERSION_MINOR;
        pluginFuncs->size       = sizeof(NPPluginFuncs);
        pluginFuncs->newp       = (NPP_NewProcPtr)(Private_New);
        pluginFuncs->destroy    = (NPP_DestroyProcPtr)(Private_Destroy);
        pluginFuncs->setwindow  = (NPP_SetWindowProcPtr)(Private_SetWindow);
        pluginFuncs->newstream  = (NPP_NewStreamProcPtr)(Private_NewStream);
        pluginFuncs->destroystream = (NPP_DestroyStreamProcPtr)(Private_DestroyStream);
        pluginFuncs->asfile     = (NPP_StreamAsFileProcPtr)(Private_StreamAsFile);
        pluginFuncs->writeready = (NPP_WriteReadyProcPtr)(Private_WriteReady);
        pluginFuncs->write      = (NPP_WriteProcPtr)(Private_Write);
        pluginFuncs->print      = (NPP_PrintProcPtr)(Private_Print);
        pluginFuncs->urlnotify  = (NPP_URLNotifyProcPtr)(Private_URLNotify);
        pluginFuncs->event      = NULL;
        pluginFuncs->javaClass  = NULL;

        
        
        pluginFuncs->getvalue   = (NPP_GetValueProcPtr)(NP_GetValue);

        err = NPP_Initialize();
    }
    
    return err;
}








NPError
NP_Shutdown(void)
{
    PLUGINDEBUGSTR("NP_Shutdown");
    NPP_Shutdown();
    return NPERR_NO_ERROR;
}
