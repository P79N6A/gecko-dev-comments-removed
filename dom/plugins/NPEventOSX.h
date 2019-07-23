





































#ifndef mozilla_dom_plugins_NPEventOSX_h
#define mozilla_dom_plugins_NPEventOSX_h 1


#include "npapi.h"

namespace mozilla {

namespace plugins {

struct NPRemoteEvent {
    NPCocoaEvent event;
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
        aLog->append(L"(NPCocoaEvent)");
    }
};

} 

#endif 
