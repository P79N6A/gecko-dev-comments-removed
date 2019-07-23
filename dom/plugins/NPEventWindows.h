





































#ifndef mozilla_dom_plugins_NPEventWindows_h
#define mozilla_dom_plugins_NPEventWindows_h 1


#include "npapi.h"
namespace mozilla {

namespace plugins {



struct NPRemoteEvent
{
    NPEvent event;
    union {
        NPRect rect;
        WINDOWPOS windowpos;
    } lParamData;
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

        
        
        
        
        switch (paramCopy.event.event) {
            case WM_WINDOWPOSCHANGED:
                
                
                
                paramCopy.lParamData.windowpos = *(reinterpret_cast<WINDOWPOS*>(paramCopy.event.lParam));
                break;
            case WM_PAINT:
                
                
                paramCopy.lParamData.rect = *(reinterpret_cast<NPRect*>(paramCopy.event.lParam));
                break;

            
            case WM_CHAR:
            case WM_SYSCHAR:

            case WM_KEYUP:
            case WM_SYSKEYUP:

            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:

            case WM_DEADCHAR:
            case WM_SYSDEADCHAR:
            case WM_CONTEXTMENU:

            case WM_CUT:
            case WM_COPY:
            case WM_PASTE:
            case WM_CLEAR:
            case WM_UNDO:

            case WM_MOUSEMOVE:
            case WM_LBUTTONDOWN:
            case WM_MBUTTONDOWN:
            case WM_RBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_MBUTTONUP:
            case WM_RBUTTONUP:
            case WM_LBUTTONDBLCLK:
            case WM_MBUTTONDBLCLK:
            case WM_RBUTTONDBLCLK:

            case WM_SETFOCUS:
            case WM_KILLFOCUS:
                break;

            
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

        if (aResult->event.event == WM_PAINT) {
            
            aResult->event.lParam = reinterpret_cast<LPARAM>(&aResult->lParamData.rect);
        } else if (aResult->event.event == WM_WINDOWPOSCHANGED) {
            
            aResult->event.lParam = reinterpret_cast<LPARAM>(&aResult->lParamData.windowpos);
        }

        return true;
    }

    static void Log(const paramType& aParam, std::wstring* aLog)
    {
        aLog->append(L"(WINEvent)");
    }

};

} 

#endif 
