





































#ifndef dom_plugins_NPPInstanceChild_h
#define dom_plugins_NPPInstanceChild_h 1

#include "NPPProtocolChild.h"

#include "npfunctions.h"

#undef _MOZ_LOG
#define _MOZ_LOG(s) printf("[NPPInstanceChild] %s\n", s)

namespace mozilla {
namespace plugins {


class NPPInstanceChild : public NPPProtocol::Child
{
    friend class NPAPIPluginChild;

public:
    NPPInstanceChild(const NPPluginFuncs* aPluginIface) :
        mPluginIface(aPluginIface),
        mNpp(this)
    {
        mData.ndata = (void*) this;
    }

    virtual ~NPPInstanceChild()
    {

    }

    NPP GetNPP()
    {
        return &mData;
    }

    
    
    
    NPError NPN_GetValue(NPNVariable aVar, void* aValue);

    
    virtual NPError NPP_SetWindow(XID aWindow,
                                  int32_t aWidth,
                                  int32_t aHeight);

private:
    const NPPluginFuncs* mPluginIface;
    NPPProtocolChild mNpp;
    NPP_t mData;
    GtkWidget* mPlug;
    NPWindow mWindow;
    NPSetWindowCallbackStruct mWsInfo;
};


} 
} 

#endif 
