




















































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
    return "1.0.0";
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
        if (nsTable->size < sizeof(NPNetscapeFuncs))
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
        gNetscapeFuncs.getJavaEnv    = NULL;
        gNetscapeFuncs.getJavaPeer   = NULL;
        gNetscapeFuncs.getvalue      = nsTable->getvalue;
        gNetscapeFuncs.pushpopupsenabledstate = nsTable->pushpopupsenabledstate;
        gNetscapeFuncs.poppopupsenabledstate  = nsTable->poppopupsenabledstate;

        





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
