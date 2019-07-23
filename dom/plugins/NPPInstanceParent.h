





































#ifndef dom_plugins_NPPInstanceParent_h
#define dom_plugins_NPPInstanceParent_h 1

#include "NPPProtocolParent.h"

#include "npfunctions.h"

#undef _MOZ_LOG
#define _MOZ_LOG(s) printf("[NPPInstanceParent] %s\n", s)

namespace mozilla {
namespace plugins {


class NPPInstanceParent : public NPPProtocol::Parent
{
    friend class NPAPIPluginParent;

public:
    NPPInstanceParent(const NPNetscapeFuncs* mNPNIface) :
        mNpp(this),
        mNPNIface(mNPNIface)
    {

    }

    virtual ~NPPInstanceParent()
    {

    }

    
    virtual void NPN_GetValue()
    {
        
    }

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
    NPPProtocolParent mNpp;
    const NPNetscapeFuncs* mNPNIface;
};


} 
} 

#endif 
