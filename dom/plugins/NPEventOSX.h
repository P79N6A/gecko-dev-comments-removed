





































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
        
        
        paramType paramCopy;

        paramCopy.event = aParam.event;

        switch (paramCopy.event.type) {
            case NPCocoaEventMouseDown:
            case NPCocoaEventMouseUp:
            case NPCocoaEventMouseMoved:
            case NPCocoaEventMouseEntered:
            case NPCocoaEventMouseExited:
            case NPCocoaEventMouseDragged:
            case NPCocoaEventFocusChanged:
            case NPCocoaEventWindowFocusChanged:
            case NPCocoaEventScrollWheel:
                
                break;
            case NPCocoaEventDrawRect:
                
                paramCopy.event.data.draw.context = NULL;
                break;
            case NPCocoaEventKeyDown:
            case NPCocoaEventKeyUp:
            case NPCocoaEventFlagsChanged:
            case NPCocoaEventTextInput:
            default:
                
                return; 
        }

        aMsg->WriteBytes(&paramCopy, sizeof(paramType));
    }

    static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
    {
        const char* bytes = 0;

        if (!aMsg->ReadBytes(aIter, &bytes, sizeof(paramType))) {
            return false;
        }
        memcpy(aResult, bytes, sizeof(paramType));

        return true;
    }

    static void Log(const paramType& aParam, std::wstring* aLog)
    {
        aLog->append(L"(NPCocoaEvent)");
    }
};

} 

#endif 
