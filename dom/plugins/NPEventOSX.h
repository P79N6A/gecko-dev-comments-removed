





































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
                aMsg->WriteBytes(&paramCopy, sizeof(paramType));
                return;
            case NPCocoaEventDrawRect:
                paramCopy.event.data.draw.context = NULL;
                aMsg->WriteBytes(&paramCopy, sizeof(paramType));
                return;
            case NPCocoaEventFlagsChanged:
                paramCopy.event.data.key.characters = NULL;
                paramCopy.event.data.key.charactersIgnoringModifiers = NULL;
                aMsg->WriteBytes(&paramCopy, sizeof(paramType));
                return;
            case NPCocoaEventKeyDown:
            case NPCocoaEventKeyUp:
                paramCopy.event.data.key.characters = NULL;
                paramCopy.event.data.key.charactersIgnoringModifiers = NULL;
                aMsg->WriteBytes(&paramCopy, sizeof(paramType));
                WriteParam(aMsg, aParam.event.data.key.characters);
                WriteParam(aMsg, aParam.event.data.key.charactersIgnoringModifiers);
                return;
            case NPCocoaEventTextInput:
                paramCopy.event.data.text.text = NULL;
                aMsg->WriteBytes(&paramCopy, sizeof(paramType));
                WriteParam(aMsg, aParam.event.data.text.text);
                return;
            default:
                NS_NOTREACHED("Attempted to serialize unknown event type.");
                return;
        }
    }

    static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
    {
        const char* bytes = 0;

        if (!aMsg->ReadBytes(aIter, &bytes, sizeof(paramType))) {
            return false;
        }
        memcpy(aResult, bytes, sizeof(paramType));

        switch (aResult->event.type) {
            case NPCocoaEventMouseDown:
            case NPCocoaEventMouseUp:
            case NPCocoaEventMouseMoved:
            case NPCocoaEventMouseEntered:
            case NPCocoaEventMouseExited:
            case NPCocoaEventMouseDragged:
            case NPCocoaEventFocusChanged:
            case NPCocoaEventWindowFocusChanged:
            case NPCocoaEventScrollWheel:
            case NPCocoaEventDrawRect:
            case NPCocoaEventFlagsChanged:
                break;
            case NPCocoaEventKeyDown:
            case NPCocoaEventKeyUp:
                if (!ReadParam(aMsg, aIter, &aResult->event.data.key.characters) ||
                    !ReadParam(aMsg, aIter, &aResult->event.data.key.charactersIgnoringModifiers)) {
                  return false;
                }
                break;
            case NPCocoaEventTextInput:
                if (!ReadParam(aMsg, aIter, &aResult->event.data.text.text)) {
                    return false;
                }
                break;
            default:
                
                return false;
        }

        return true;
    }

    static void Log(const paramType& aParam, std::wstring* aLog)
    {
        aLog->append(L"(NPCocoaEvent)");
    }
};

} 

#endif 
