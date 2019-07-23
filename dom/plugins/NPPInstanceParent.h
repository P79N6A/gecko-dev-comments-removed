





































#ifndef dom_plugins_NPPInstanceParent_h
#define dom_plugins_NPPInstanceParent_h 1

#include "mozilla/plugins/NPPProtocolParent.h"
#include "mozilla/plugins/NPObjectParent.h"

#include "npfunctions.h"

#undef _MOZ_LOG
#define _MOZ_LOG(s) printf("[NPPInstanceParent] %s\n", s)

namespace mozilla {
namespace plugins {

class NPBrowserStreamParent;

class NPPInstanceParent :
    public NPPProtocolParent
{
    friend class NPAPIPluginParent;
    friend class mozilla::plugins::NPBrowserStreamParent;

public:
    NPPInstanceParent(NPP npp, const NPNetscapeFuncs* npniface)
        : mNPP(npp)
        , mNPNIface(npniface)
    {
    }

    virtual ~NPPInstanceParent()
    {
    }

    virtual nsresult AnswerNPN_GetValue(const nsString& in, nsString* out)
    {
        return NS_OK;
    }

    virtual NPObjectProtocolParent*
    NPObjectConstructor(NPError* _retval);

    virtual nsresult
    NPObjectDestructor(NPObjectProtocolParent* aObject,
                       NPError* _retval);

    virtual NPBrowserStreamProtocolParent*
    NPBrowserStreamConstructor(const nsCString& url, const uint32_t& length,
                               const uint32_t& lastmodified,
                               const nsCString& headers,
                               const nsCString& mimeType, const bool& seekable,
                               NPError* rv, uint16_t *stype);

    virtual nsresult
    AnswerNPBrowserStreamDestructor(NPBrowserStreamProtocolParent* stream,
                                    const NPError& reason, const bool& artificial);

    virtual nsresult
    NPBrowserStreamDestructor(NPBrowserStreamProtocolParent* stream,
                              const NPError& reason, const bool& artificial);

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

    int16_t NPP_HandleEvent(void* event)
    {
        _MOZ_LOG(__FUNCTION__);
        return 0;
    }

    void NPP_URLNotify(const char* url, NPReason reason, void* notifyData)
    {
        _MOZ_LOG(__FUNCTION__);
    }

private:
    NPP mNPP;
    const NPNetscapeFuncs* mNPNIface;
};


} 
} 

#endif 
