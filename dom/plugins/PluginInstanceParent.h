





































#ifndef dom_plugins_PluginInstanceParent_h
#define dom_plugins_PluginInstanceParent_h 1

#include "mozilla/plugins/PPluginInstanceParent.h"
#include "mozilla/plugins/PluginScriptableObjectParent.h"

#include "npfunctions.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"

#undef _MOZ_LOG
#define _MOZ_LOG(s) printf("[PluginInstanceParent] %s\n", s)

namespace mozilla {
namespace plugins {

class PBrowserStreamParent;
class BrowserStreamParent;
class PluginModuleParent;

class PluginInstanceParent : public PPluginInstanceParent
{
    friend class PluginModuleParent;
    friend class BrowserStreamParent;
    friend class PluginStreamParent;

public:
    PluginInstanceParent(PluginModuleParent* parent,
                         NPP npp,
                         const NPNetscapeFuncs* npniface);

    virtual ~PluginInstanceParent();

    virtual PPluginScriptableObjectParent*
    AllocPPluginScriptableObject();

    virtual bool
    AnswerPPluginScriptableObjectConstructor(PPluginScriptableObjectParent* aActor);

    virtual bool
    DeallocPPluginScriptableObject(PPluginScriptableObjectParent* aObject);
    virtual PBrowserStreamParent*
    AllocPBrowserStream(const nsCString& url,
                        const uint32_t& length,
                        const uint32_t& lastmodified,
                        const PStreamNotifyParent* notifyData,
                        const nsCString& headers,
                        const nsCString& mimeType,
                        const bool& seekable,
                        NPError* rv,
                        uint16_t *stype);

    virtual bool
    AnswerPBrowserStreamDestructor(PBrowserStreamParent* stream,
                                   const NPError& reason,
                                   const bool& artificial);

    virtual bool
    DeallocPBrowserStream(PBrowserStreamParent* stream,
                          const NPError& reason,
                          const bool& artificial);

    virtual PPluginStreamParent*
    AllocPPluginStream(const nsCString& mimeType,
                       const nsCString& target,
                       NPError* result);

    virtual bool
    DeallocPPluginStream(PPluginStreamParent* stream,
                         const NPError& reason,
                         const bool& artificial);

    virtual bool
    AnswerNPN_GetValue_NPNVjavascriptEnabledBool(bool* value, NPError* result);
    virtual bool
    AnswerNPN_GetValue_NPNVisOfflineBool(bool* value, NPError* result);
    virtual bool
    AnswerNPN_GetValue_NPNVWindowNPObject(
                                       PPluginScriptableObjectParent** value,
                                       NPError* result);
    virtual bool
    AnswerNPN_GetValue_NPNVPluginElementNPObject(
                                       PPluginScriptableObjectParent** value,
                                       NPError* result);
    virtual bool
    AnswerNPN_GetValue_NPNVprivateModeBool(bool* value, NPError* result);

    virtual bool
    AnswerNPN_GetURL(const nsCString& url, const nsCString& target,
                     NPError *result);

    virtual bool
    AnswerNPN_PostURL(const nsCString& url, const nsCString& target,
                      const nsCString& buffer, const bool& file,
                      NPError* result);

    virtual PStreamNotifyParent*
    AllocPStreamNotify(const nsCString& url, const nsCString& target,
                       const bool& post, const nsCString& buffer,
                       const bool& file,
                       NPError* result);

    virtual bool
    DeallocPStreamNotify(PStreamNotifyParent* notifyData,
                         const NPReason& reason);

    NPError NPP_SetWindow(NPWindow* aWindow);
    NPError NPP_GetValue(NPPVariable variable, void *ret_value);

    NPError NPP_SetValue(NPNVariable variable, void *value)
    {
        _MOZ_LOG(__FUNCTION__);
        return 1;
    }


    NPError NPP_NewStream(NPMIMEType type, NPStream* stream,
                          NPBool seekable, uint16_t* stype);
    NPError NPP_DestroyStream(NPStream* stream, NPReason reason);

    void NPP_Print(NPPrint* platformPrint)
    {
        _MOZ_LOG(__FUNCTION__);
    }

    int16_t NPP_HandleEvent(void* event);

    void NPP_URLNotify(const char* url, NPReason reason, void* notifyData);

    PluginModuleParent* GetModule()
    {
        return mParent;
    }

private:
    PluginModuleParent* mParent;
    NPP mNPP;
    const NPNetscapeFuncs* mNPNIface;

    nsTArray<nsAutoPtr<PluginScriptableObjectParent> > mScriptableObjects;
};


} 
} 

#endif 
