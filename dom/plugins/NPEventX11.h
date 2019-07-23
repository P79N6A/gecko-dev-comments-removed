






































#ifndef mozilla_dom_plugins_NPEventX11_h
#define mozilla_dom_plugins_NPEventX11_h 1

#if defined(MOZ_WIDGET_GTK2)
#  include <gdk/gdkx.h>
#else
#  error Implement me for your toolkit
#endif

#include "npapi.h"


















namespace IPC {

template <>
struct ParamTraits<NPEvent>     
{
    typedef NPEvent paramType;

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
        SetXDisplay(*aResult);
        return true;
    }

    static void Log(const paramType& aParam, std::wstring* aLog)
    {
        
        aLog->append(L"(XEvent)");
    }

private:
    static Display* GetXDisplay(const XAnyEvent& ev)
    {
        

        
        
        return GDK_DISPLAY();
    }

    static Display* GetXDisplay(const XErrorEvent& ev)
    {
        

        
        
        return GDK_DISPLAY();
    }

    static void SetXDisplay(XEvent& ev)
    {
        if (ev.type >= KeyPress) {
            ev.xany.display = GetXDisplay(ev.xany);
        }
        else {
            
            
            ev.xerror.display = GetXDisplay(ev.xerror);
        }
    }
};

} 


#endif 
