





































#include "PluginInstanceParent.h"
#include "BrowserStreamParent.h"
#include "PluginStreamParent.h"
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

bool
PluginInstanceParent::AnswerPBrowserStreamDestructor(PBrowserStreamParent* stream,
                                                     const NPError& reason,
                                                     const bool& artificial)
{
    if (!artificial) {
        static_cast<BrowserStreamParent*>(stream)->NPN_DestroyStream(reason);
    }
    return true;
}

bool
PluginInstanceParent::PBrowserStreamDestructor(PBrowserStreamParent* stream,
                                               const NPError& reason,
                                               const bool& artificial)
{
    delete stream;
    return true;
}

PPluginStreamParent*
PluginInstanceParent::PPluginStreamConstructor(const nsCString& mimeType,
                                               const nsCString& target,
                                               NPError* result)
{
    return new PluginStreamParent(this, mimeType, target, result);
}

bool
PluginInstanceParent::PPluginStreamDestructor(PPluginStreamParent* stream,
                                              const NPError& reason,
                                              const bool& artificial)
{
    if (!artificial) {
        static_cast<PluginStreamParent*>(stream)->NPN_DestroyStream(reason);
    }
    delete stream;
    return true;
}

bool
PluginInstanceParent::AnswerNPN_GetValue_NPNVjavascriptEnabledBool(
                                                       bool* value,
                                                       NPError* result)
{
    NPBool v;
    *result = mNPNIface->getvalue(mNPP, NPNVjavascriptEnabledBool, &v);
    *value = v;
    return true;
}

bool
PluginInstanceParent::AnswerNPN_GetValue_NPNVisOfflineBool(bool* value,
                                                           NPError* result)
{
    NPBool v;
    *result = mNPNIface->getvalue(mNPP, NPNVisOfflineBool, &v);
    *value = v;
    return true;
}

bool
PluginInstanceParent::AnswerNPN_GetValue_NPNVWindowNPObject(
                                        PPluginScriptableObjectParent** value,
                                        NPError* result)
{
    
    *value = NULL;
    *result = NPERR_GENERIC_ERROR;
    return true;
}

bool
PluginInstanceParent::AnswerNPN_GetValue_NPNVPluginElementNPObject(
                                        PPluginScriptableObjectParent** value,
                                        NPError* result)
{
    
    *value = NULL;
    *result = NPERR_GENERIC_ERROR;
    return true;
}

bool
PluginInstanceParent::AnswerNPN_GetValue_NPNVprivateModeBool(bool* value,
                                                             NPError* result)
{
    NPBool v;
    *result = mNPNIface->getvalue(mNPP, NPNVprivateModeBool, &v);
    *value = v;
    return true;
}

bool
PluginInstanceParent::AnswerNPN_GetURL(const nsCString& url,
                                       const nsCString& target,
                                       NPError* result)
{
    *result = mNPNIface->geturl(mNPP,
                                NullableStringGet(url),
                                NullableStringGet(target));
    return true;
}

bool
PluginInstanceParent::AnswerNPN_PostURL(const nsCString& url,
                                        const nsCString& target,
                                        const nsCString& buffer,
                                        const bool& file,
                                        NPError* result)
{
    *result = mNPNIface->posturl(mNPP, url.get(), target.get(),
                                 buffer.Length(), buffer.get(), file);
    return true;
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
        *result = mNPNIface->geturlnotify(mNPP,
                                          NullableStringGet(url),
                                          NullableStringGet(target),
                                          notifyData);
    }
    else {
        *result = mNPNIface->posturlnotify(mNPP,
                                           NullableStringGet(url),
                                           NullableStringGet(target),
                                           buffer.Length(), buffer.get(),
                                           file, notifyData);
    }
    
    return notifyData;
}

bool
PluginInstanceParent::PStreamNotifyDestructor(PStreamNotifyParent* notifyData,
                                              const NPReason& reason)
{
    delete notifyData;
    return true;
}

NPError
PluginInstanceParent::NPP_SetWindow(NPWindow* aWindow)
{
    _MOZ_LOG(__FUNCTION__);
    NS_ENSURE_TRUE(aWindow, NPERR_GENERIC_ERROR);

    NPError prv;
    if (!CallNPP_SetWindow(*aWindow, &prv))
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

        case NPPVpluginScriptableNPObject: {
            PPluginScriptableObjectParent* actor;
            NPError rv;
            if (!CallNPP_GetValue_NPPVpluginScriptableNPObject(&actor, &rv)) {
                return NPERR_GENERIC_ERROR;
            }
            return rv;
        }
        
        default:
            return NPERR_GENERIC_ERROR;
    }
}

int16_t
PluginInstanceParent::NPP_HandleEvent(void* event)
{
    _MOZ_LOG(__FUNCTION__);

    int16_t handled;
    if (!CallNPP_HandleEvent(*reinterpret_cast<NPEvent*>(event),
                             &handled)) {
        return 0;               
    }

    return handled;
}

NPError
PluginInstanceParent::NPP_NewStream(NPMIMEType type, NPStream* stream,
                                    NPBool seekable, uint16_t* stype)
{
    _MOZ_LOG(__FUNCTION__);
        
    BrowserStreamParent* bs = new BrowserStreamParent(this, stream);

    NPError err;
    
    if (!CallPBrowserStreamConstructor(bs,
                                       nsCString(stream->url),
                                       stream->end,
                                       stream->lastmodified,
                                       static_cast<PStreamNotifyParent*>(stream->notifyData),
                                       nsCString(stream->headers),
                                       nsCString(type), seekable, &err, stype))
        return NPERR_GENERIC_ERROR;

    if (NPERR_NO_ERROR != err)
        CallPBrowserStreamDestructor(bs, NPERR_GENERIC_ERROR, true);

    return err;
}

NPError
PluginInstanceParent::NPP_DestroyStream(NPStream* stream, NPReason reason)
{
    AStream* s = static_cast<AStream*>(stream->pdata);
    if (s->IsBrowserStream()) {
        BrowserStreamParent* sp =
            static_cast<BrowserStreamParent*>(s);
        if (sp->mNPP != this)
            NS_RUNTIMEABORT("Mismatched plugin data");

        CallPBrowserStreamDestructor(sp, reason, false);
        return NPERR_NO_ERROR;
    }
    else {
        PluginStreamParent* sp =
            static_cast<PluginStreamParent*>(s);
        if (sp->mInstance != this)
            NS_RUNTIMEABORT("Mismatched plugin data");

        CallPPluginStreamDestructor(sp, reason, false);
        return NPERR_NO_ERROR;
    }
}

PPluginScriptableObjectParent*
PluginInstanceParent::PPluginScriptableObjectConstructor()
{
    return new PluginScriptableObjectParent();
}

bool
PluginInstanceParent::PPluginScriptableObjectDestructor(PPluginScriptableObjectParent* aObject)
{
    delete aObject;
    return true;
}

} 
} 
