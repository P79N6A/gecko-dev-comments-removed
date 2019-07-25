






































#ifndef mozilla_dom_plugins_NPEventX11_h
#define mozilla_dom_plugins_NPEventX11_h 1

#include "npapi.h"

#include "mozilla/X11Util.h"

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
        aMsg->WriteBytes(&aParam, sizeof(paramType));
    }

    static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
    {
        const char* bytes = 0;

        if (!aMsg->ReadBytes(aIter, &bytes, sizeof(paramType))) {
            return false;
        }

        memcpy(aResult, bytes, sizeof(paramType));
        SetXDisplay(aResult->event);
        return true;
    }

    static void Log(const paramType& aParam, std::wstring* aLog)
    {
        
        aLog->append(L"(XEvent)");
    }

private:
    static void SetXDisplay(XEvent& ev)
    {
        Display* display = mozilla::DefaultXDisplay();
        if (ev.type >= KeyPress) {
            ev.xany.display = display;
        }
        else {
            
            
            ev.xerror.display = display;
        }
    }
};

} 


#endif 
