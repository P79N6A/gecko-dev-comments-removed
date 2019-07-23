





































#include "PluginInstanceParent.h"
#include "BrowserStreamParent.h"
#include "StreamNotifyParent.h"

namespace mozilla {
namespace plugins {

PBrowserStreamParent*
PluginInstanceParent::PBrowserStreamConstructor(const nsCString& url,
                                                const uint32_t& length,
                                                const uint32_t& lastmodified,
                                                const PStreamNotifyParent* notifyData,
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
PluginInstanceParent::AnswerPBrowserStreamDestructor(PBrowserStreamParent* stream,
                                                     const NPError& reason,
                                                     const bool& artificial)
{
    if (!artificial) {
        static_cast<BrowserStreamParent*>(stream)->NPN_DestroyStream(reason);
    }
    return NS_OK;
}

nsresult
PluginInstanceParent::PBrowserStreamDestructor(PBrowserStreamParent* stream,
                                               const NPError& reason,
                                               const bool& artificial)
{
    delete stream;
    return NS_OK;
}

nsresult
PluginInstanceParent::AnswerNPN_GetURL(const nsCString& url,
                                       const nsCString& target,
                                       NPError* result)
{
    *result = mNPNIface->geturl(mNPP, url.get(), target.get());
    
    return NS_OK;
}

nsresult
PluginInstanceParent::AnswerNPN_PostURL(const nsCString& url,
                                        const nsCString& target,
                                        const nsCString& buffer,
                                        const bool& file,
                                        NPError* result)
{
    *result = mNPNIface->posturl(mNPP, url.get(), target.get(),
                                 buffer.Length(), buffer.get(), file);
    
    return NS_OK;
}

PStreamNotifyParent*
PluginInstanceParent::PStreamNotifyConstructor(const nsCString& url,
                                               const nsCString& target,
                                               const bool& post,
                                               const nsCString& buffer,
                                               const bool& file,
                                               NPError* result)
{
    StreamNotifyParent* notifyData = new StreamNotifyParent();

    if (!post) {
        *result = mNPNIface->geturlnotify(mNPP, url.get(), target.get(),
                                          notifyData);
    }
    else {
        *result = mNPNIface->posturlnotify(mNPP, url.get(), target.get(),
                                           buffer.Length(), buffer.get(),
                                           file, notifyData);
    }
    
    return notifyData;
}

nsresult
PluginInstanceParent::PStreamNotifyDestructor(PStreamNotifyParent* notifyData,
                                              const NPReason& reason)
{
    delete notifyData;
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
    CallPBrowserStreamConstructor(new BrowserStreamParent(this, stream),
                                  nsCString(stream->url),
                                  stream->end,
                                  stream->lastmodified,
                                  static_cast<PStreamNotifyParent*>(stream->notifyData),
                                  nsCString(stream->headers),
                                  nsCString(type), seekable, &err, stype);
    return err;
}    

NPError
PluginInstanceParent::NPP_DestroyStream(NPStream* stream, NPReason reason)
{
    BrowserStreamParent* sp =
        static_cast<BrowserStreamParent*>(stream->pdata);
    if (sp->mNPP != this)
        NS_RUNTIMEABORT("Mismatched plugin data");

    return CallPBrowserStreamDestructor(sp, reason, false);
}

PPluginScriptableObjectParent*
PluginInstanceParent::PPluginScriptableObjectConstructor(NPError* _retval)
{
    NS_NOTYETIMPLEMENTED("PluginInstanceParent::PPluginScriptableObjectConstructor");
    return nsnull;
}

nsresult
PluginInstanceParent::PPluginScriptableObjectDestructor(PPluginScriptableObjectParent* aObject,
                                                        NPError* _retval)
{
    NS_NOTYETIMPLEMENTED("PluginInstanceParent::PPluginScriptableObjectDestructor");
    return NS_ERROR_NOT_IMPLEMENTED;
}

} 
} 
