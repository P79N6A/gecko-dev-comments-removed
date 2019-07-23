





































#ifndef mozilla_dom_plugins_NPEventOSX_h
#define mozilla_dom_plugins_NPEventOSX_h 1


#include "npapi.h"
#include "IPC/IPCMessageUtils.h"

#warning This is only a stub implementation IMPLEMENT ME

namespace mozilla {
namespace plugins {
struct NPRemoteEvent {
    NPEvent event;
};
}
}

namespace IPC {

template <>
struct ParamTraits<mozilla::plugins::NPRemoteEvent>
{
    typedef mozilla::plugins::NPRemoteEvent paramType;

    static void Write(Message* aMsg, const paramType& aParam)
    {
    }

    static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
    {
        return true;
    }

    static void Log(const paramType& aParam, std::wstring* aLog)
    {
    }
};

} 

#endif 
