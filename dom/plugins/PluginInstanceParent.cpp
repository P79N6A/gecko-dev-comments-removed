





































#include "PluginInstanceParent.h"
#include "PluginStreamParent.h"

namespace mozilla {
namespace plugins {

PPluginStreamProtocolParent*
PluginInstanceParent::PPluginStreamConstructor(const nsCString& url,
                                               const uint32_t& length,
                                               const uint32_t& lastmodified,
                                               const nsCString& headers,
                                               const nsCString& mimeType,
                                               const bool& seekable,
                                               NPError* rv,
                                               uint16_t *stype)
{
    NS_RUNTIMEABORT("Not reachable");
    return NULL;
}

nsresult
PluginInstanceParent::AnswerPPluginStreamDestructor(PPluginStreamProtocolParent* stream,
                                                    const NPError& reason,
                                                    const bool& artificial)
{
    if (!artificial) {
        static_cast<PluginStreamParent*>(stream)->NPN_DestroyStream(reason);
    }
    return NS_OK;
}

nsresult
PluginInstanceParent::PPluginStreamDestructor(PPluginStreamProtocolParent* stream,
                                              const NPError& reason,
                                              const bool& artificial)
{
    delete stream;
    return NS_OK;
}

NPError
PluginInstanceParent::NPP_SetWindow(NPWindow* aWindow)
{
    _MOZ_LOG(__FUNCTION__);
    NS_ENSURE_TRUE(aWindow, NPERR_GENERIC_ERROR);

    NPError prv;
    nsresult rv = CallNPP_SetWindow(*aWindow, &prv);
    if (NS_OK != rv)
        return NPERR_GENERIC_ERROR;
    return prv;
}

NPError
PluginInstanceParent::NPP_GetValue(NPPVariable variable, void *ret_value)
{
    _MOZ_LOG(__FUNCTION__);

    
    switch(variable) {
#ifdef OS_LINUX
    case NPPVpluginNeedsXEmbed:
        (*(PRBool*)ret_value) = PR_TRUE;
        return NPERR_NO_ERROR;
#endif
    default:
        return NPERR_GENERIC_ERROR;
    }

    NS_NOTREACHED("Don't get here!");
    return NPERR_GENERIC_ERROR;
}

NPError
PluginInstanceParent::NPP_NewStream(NPMIMEType type, NPStream* stream,
                                    NPBool seekable, uint16_t* stype)
{
    _MOZ_LOG(__FUNCTION__);
        
    NPError err;
    CallPPluginStreamConstructor(new PluginStreamParent(this, stream),
                                 nsCString(stream->url),
                                 stream->end,
                                 stream->lastmodified,
                                 nsCString(stream->headers),
                                 nsCString(type), seekable, &err, stype);
    return err;
}    

NPError
PluginInstanceParent::NPP_DestroyStream(NPStream* stream, NPReason reason)
{
    PluginStreamParent* sp =
        static_cast<PluginStreamParent*>(stream->pdata);
    if (sp->mNPP != this)
        NS_RUNTIMEABORT("Mismatched plugin data");

    return CallPPluginStreamDestructor(sp, reason, false);
}

PPluginScriptableObjectProtocolParent*
PluginInstanceParent::PPluginScriptableObjectConstructor(NPError* _retval)
{
    NS_NOTYETIMPLEMENTED("PluginInstanceParent::PPluginScriptableObjectConstructor");
    return nsnull;
}

nsresult
PluginInstanceParent::PPluginScriptableObjectDestructor(PPluginScriptableObjectProtocolParent* aObject,
                                                        NPError* _retval)
{
    NS_NOTYETIMPLEMENTED("PluginInstanceParent::PPluginScriptableObjectDestructor");
    return NS_ERROR_NOT_IMPLEMENTED;
}

} 
} 
