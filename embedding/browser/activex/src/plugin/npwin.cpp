






#include "npapi.h"
#include "npfunctions.h"


#ifdef WIN32
    #define NP_EXPORT
#else
    #define NP_EXPORT _export
#endif


NPNetscapeFuncs* g_pNavigatorFuncs = 0;












static NPPluginFuncs* g_pluginFuncs;








NPError WINAPI NP_EXPORT
NP_GetEntryPoints(NPPluginFuncs* pFuncs)
{
    
    if(pFuncs == NULL)
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
    pFuncs->event         = 0;       
#ifdef MOZ_ACTIVEX_PLUGIN_XPCONNECT
    pFuncs->getvalue      = NPP_GetValue;
    pFuncs->setvalue      = NPP_SetValue;
#endif

	g_pluginFuncs		  = pFuncs;

    return NPERR_NO_ERROR;
}







NPError WINAPI NP_EXPORT 
NP_Initialize(NPNetscapeFuncs* pFuncs)
{
    
    if(pFuncs == NULL)
        return NPERR_INVALID_FUNCTABLE_ERROR;

    g_pNavigatorFuncs = pFuncs; 

    
    
    if(HIBYTE(pFuncs->version) > NP_VERSION_MAJOR)
        return NPERR_INCOMPATIBLE_VERSION_ERROR;

	
    int navMinorVers = g_pNavigatorFuncs->version & 0xFF;

	if( navMinorVers >= NPVERS_HAS_NOTIFICATION ) {
		g_pluginFuncs->urlnotify = NPP_URLNotify;
	}
	
	
    return NPP_Initialize();
}









NPError WINAPI NP_EXPORT 
NP_Shutdown()
{
    NPP_Shutdown();
    g_pNavigatorFuncs = NULL;
    return NPERR_NO_ERROR;
}
















void NPN_Version(int* plugin_major, int* plugin_minor, int* netscape_major, int* netscape_minor)
{
    *plugin_major   = NP_VERSION_MAJOR;
    *plugin_minor   = NP_VERSION_MINOR;
    *netscape_major = HIBYTE(g_pNavigatorFuncs->version);
    *netscape_minor = LOBYTE(g_pNavigatorFuncs->version);
}



NPError NPN_GetURLNotify(NPP instance, const char *url, const char *target, void* notifyData)

{
	int navMinorVers = g_pNavigatorFuncs->version & 0xFF;
	NPError err;
	if( navMinorVers >= NPVERS_HAS_NOTIFICATION ) {
		err = g_pNavigatorFuncs->geturlnotify(instance, url, target, notifyData);
	}
	else {
		err = NPERR_INCOMPATIBLE_VERSION_ERROR;
	}
	return err;
}


NPError NPN_GetURL(NPP instance, const char *url, const char *target)
{
    return g_pNavigatorFuncs->geturl(instance, url, target);
}

NPError NPN_PostURLNotify(NPP instance, const char* url, const char* window, uint32_t len, const char* buf, NPBool file, void* notifyData)
{
	int navMinorVers = g_pNavigatorFuncs->version & 0xFF;
	NPError err;
	if( navMinorVers >= NPVERS_HAS_NOTIFICATION ) {
		err = g_pNavigatorFuncs->posturlnotify(instance, url, window, len, buf, file, notifyData);
	}
	else {
		err = NPERR_INCOMPATIBLE_VERSION_ERROR;
	}
	return err;
}


NPError NPN_PostURL(NPP instance, const char* url, const char* window, uint32_t len, const char* buf, NPBool file)
{
    return g_pNavigatorFuncs->posturl(instance, url, window, len, buf, file);
}





NPError NPN_RequestRead(NPStream* stream, NPByteRange* rangeList)
{
    return g_pNavigatorFuncs->requestread(stream, rangeList);
}




NPError NPN_NewStream(NPP instance, NPMIMEType type, 
								const char* target, NPStream** stream)
{
	int navMinorVersion = g_pNavigatorFuncs->version & 0xFF;
	NPError err;

	if( navMinorVersion >= NPVERS_HAS_STREAMOUTPUT ) {
		err = g_pNavigatorFuncs->newstream(instance, type, target, stream);
	}
	else {
		err = NPERR_INCOMPATIBLE_VERSION_ERROR;
	}
	return err;
}



int32_t NPN_Write(NPP instance, NPStream *stream,
                  int32_t len, void *buffer)
{
	int navMinorVersion = g_pNavigatorFuncs->version & 0xFF;
	int32_t result;

	if( navMinorVersion >= NPVERS_HAS_STREAMOUTPUT ) {
		result = g_pNavigatorFuncs->write(instance, stream, len, buffer);
	}
	else {
		result = -1;
	}
	return result;
}




NPError NPN_DestroyStream(NPP instance, NPStream* stream, NPError reason)
{
	int navMinorVersion = g_pNavigatorFuncs->version & 0xFF;
	NPError err;

	if( navMinorVersion >= NPVERS_HAS_STREAMOUTPUT ) {
		err = g_pNavigatorFuncs->destroystream(instance, stream, reason);
	}
	else {
		err = NPERR_INCOMPATIBLE_VERSION_ERROR;
	}
	return err;
}



void NPN_Status(NPP instance, const char *message)
{
    g_pNavigatorFuncs->status(instance, message);
}



const char* NPN_UserAgent(NPP instance)
{
    return g_pNavigatorFuncs->uagent(instance);
}






void* NPN_MemAlloc(uint32_t size)
{
    return g_pNavigatorFuncs->memalloc(size);
}



void NPN_MemFree(void* ptr)
{
    g_pNavigatorFuncs->memfree(ptr);
}



void NPN_ReloadPlugins(NPBool reloadPages)
{
    g_pNavigatorFuncs->reloadplugins(reloadPages);
}

NPError NPN_GetValue(NPP instance, NPNVariable variable, void *result)
{
    return g_pNavigatorFuncs->getvalue(instance, variable, result);
}
