





































#ifndef dom_plugins_PluginInstanceChild_h
#define dom_plugins_PluginInstanceChild_h 1

#include "mozilla/plugins/PPluginInstanceChild.h"
#include "mozilla/plugins/PluginScriptableObjectChild.h"

#include "npfunctions.h"

#undef _MOZ_LOG
#define _MOZ_LOG(s) printf("[PluginInstanceChild] %s\n", s)

namespace mozilla {
namespace plugins {

class PBrowserStreamChild;
class BrowserStreamChild;

class PluginInstanceChild : public PPluginInstanceChild
{
#ifdef OS_WIN
    friend LRESULT CALLBACK PluginWindowProc(HWND hWnd,
                                             UINT message,
                                             WPARAM wParam,
                                             LPARAM lParam);
#endif

protected:
    friend class BrowserStreamChild;



    
    virtual nsresult
    RecvTest(const Variant& v1, const Variant& v2)
    {
        printf("\n[PluginInstanceChild] v1: ");

        switch (v1.type()) {
        case Variant::Tint: {
            int i = v1;
            printf("variant-int %d", i);
            break;
        }            
        case Variant::Tdouble: {
            double d = v1;
            printf("variant-double %e", d);
            break;
        }
        case Variant::TPPluginInstanceChild: {
            const PPluginInstanceChild* p = v1;
            printf("plugin instance %p", p);
            break;
        }
        default:
            NS_RUNTIMEABORT("unexpected Variant value");
        }

        printf(", v2: ");

        switch (v2.type()) {
        case Variant::Tint: {
            int i = v2;
            printf("variant-int %d", i);
            break;
        }            
        case Variant::Tdouble: {
            double d = v2;
            printf("variant-double %e", d);
            break;
        }
        case Variant::TPPluginInstanceChild: {
            const PPluginInstanceChild* p = v2;
            printf("plugin instance %p", p);
            break;
        }
        default:
            NS_RUNTIMEABORT("unexpected Variant value");
        }

        puts("\n");
        return NS_OK;
    }



    virtual nsresult AnswerNPP_SetWindow(const NPWindow& window, NPError* rv);

    virtual nsresult AnswerNPP_GetValue(const nsString& key, nsString* value);

    virtual PPluginScriptableObjectChild*
    PPluginScriptableObjectConstructor(NPError* _retval);

    virtual nsresult
    PPluginScriptableObjectDestructor(PPluginScriptableObjectChild* aObject,
                                      NPError* _retval);


    virtual PBrowserStreamChild*
    PBrowserStreamConstructor(const nsCString& url,
                              const uint32_t& length,
                              const uint32_t& lastmodified,
                              const PStreamNotifyChild* notifyData,
                              const nsCString& headers,
                              const nsCString& mimeType,
                              const bool& seekable,
                              NPError* rv,
                              uint16_t *stype);

    virtual nsresult
    PBrowserStreamDestructor(PBrowserStreamChild* stream,
                             const NPError& reason,
                             const bool& artificial);

    virtual PStreamNotifyChild*
    PStreamNotifyConstructor(const nsCString& url, const nsCString& target,
                             const bool& post, const nsCString& buffer,
                             const bool& file,
                             NPError* result);

    virtual nsresult
    PStreamNotifyDestructor(PStreamNotifyChild* notifyData,
                            const NPReason& reason);

public:
    PluginInstanceChild(const NPPluginFuncs* aPluginIface) :
        mPluginIface(aPluginIface)
#if defined(OS_LINUX)
        , mPlug(0)
#elif defined(OS_WIN)
        , mPluginWindowHWND(0)
        , mPluginWndProc(0)
        , mPluginParentHWND(0)
#endif
    {
        memset(&mWindow, 0, sizeof(mWindow));
        mData.ndata = (void*) this;
#if defined(OS_LINUX)
        memset(&mWsInfo, 0, sizeof(mWsInfo));
#endif
    }

    virtual ~PluginInstanceChild();

    bool Initialize();

    NPP GetNPP()
    {
        return &mData;
    }

    NPError
    NPN_GetValue(NPNVariable aVariable,
                 void* aValue);

private:

#if defined(OS_WIN)
    static bool RegisterWindowClass();
    bool CreatePluginWindow();
    void DestroyPluginWindow();
    void ReparentPluginWindow(HWND hWndParent);
    void SizePluginWindow(int width, int height);
    static LRESULT CALLBACK DummyWindowProc(HWND hWnd,
                                            UINT message,
                                            WPARAM wParam,
                                            LPARAM lParam);
    static LRESULT CALLBACK PluginWindowProc(HWND hWnd,
                                             UINT message,
                                             WPARAM wParam,
                                             LPARAM lParam);
#endif

    const NPPluginFuncs* mPluginIface;
    NPP_t mData;
#ifdef OS_LINUX
    GtkWidget* mPlug;
#endif
    NPWindow mWindow;
#ifdef OS_LINUX
    NPSetWindowCallbackStruct mWsInfo;
#elif defined(OS_WIN)
    HWND mPluginWindowHWND;
    WNDPROC mPluginWndProc;
    HWND mPluginParentHWND;
#endif
};

} 
} 

#endif 
