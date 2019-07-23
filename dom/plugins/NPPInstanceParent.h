





































#ifndef dom_plugins_NPPInstanceParent_h
#define dom_plugins_NPPInstanceParent_h 1

#include "mozilla/plugins/NPPProtocolParent.h"
#include "mozilla/plugins/NPObjectParent.h"

#include "npfunctions.h"

#undef _MOZ_LOG
#define _MOZ_LOG(s) printf("[NPPInstanceParent] %s\n", s)

namespace mozilla {
namespace plugins {

class NPPInstanceParent :
    public NPPProtocolParent
{
public:
    NPPInstanceParent(const NPNetscapeFuncs* mNPNIface) :
        mNPNIface(mNPNIface)
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

    NPError NPP_SetWindow(NPWindow* aWindow);
    NPError NPP_GetValue(NPPVariable variable, void *ret_value);

    NPError NPP_SetValue(NPNVariable variable, void *value)
    {
        _MOZ_LOG(__FUNCTION__);
        return 1;
    }


    NPError NPP_NewStream(NPMIMEType type, NPStream* stream,
                          NPBool seekable, uint16_t* stype)
    {
        _MOZ_LOG(__FUNCTION__);
        return 1;
    }

    NPError NPP_DestroyStream(NPStream* stream, NPReason reason)
    {
        _MOZ_LOG(__FUNCTION__);
        return 1;
    }

    int32_t NPP_WriteReady(NPStream* stream)
    {
        _MOZ_LOG(__FUNCTION__);
        return 0;
    }

    int32_t NPP_Write(NPStream* stream,
                      int32_t offset, int32_t len, void* buffer)
    {
        _MOZ_LOG(__FUNCTION__);
        return 0;
    }

    void NPP_StreamAsFile(NPStream* stream, const char* fname)
    {
        _MOZ_LOG(__FUNCTION__);
    }

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
    const NPNetscapeFuncs* mNPNIface;
};


} 
} 

#endif 
