





































#ifndef dom_plugins_NPPInstanceChild_h
#define dom_plugins_NPPInstanceChild_h 1

#include "NPPProtocolChild.h"

#include "npfunctions.h"

#undef _MOZ_LOG
#define _MOZ_LOG(s) printf("[NPPInstanceChild] %s\n", s)

namespace mozilla {
namespace plugins {


class NPPInstanceChild : public NPPProtocolChild
{
protected:
    virtual nsresult AnswerNPP_SetWindow(const NPWindow& window, NPError* rv);

    virtual nsresult AnswerNPP_GetValue(const String& key, String* value);

public:
    NPPInstanceChild(const NPPluginFuncs* aPluginIface) :
        mPluginIface(aPluginIface)
    {
        memset(&mWindow, 0, sizeof(mWindow));
        mData.ndata = (void*) this;
    }

    virtual ~NPPInstanceChild()
    {

    }

    NPP GetNPP()
    {
        return &mData;
    }

    NPError NPN_GetValue(NPNVariable aVariable, void* aValue);

private:
    const NPPluginFuncs* mPluginIface;
    NPP_t mData;
#ifdef OS_LINUX
    GtkWidget* mPlug;
#endif
    NPWindow mWindow;
#ifdef OS_LINUX
    NPSetWindowCallbackStruct mWsInfo;
#endif
};

} 
} 

#endif 
