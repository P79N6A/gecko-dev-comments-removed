





































#ifndef dom_plugins_PluginInstanceChild_h
#define dom_plugins_PluginInstanceChild_h 1

#include "mozilla/plugins/PPluginInstanceChild.h"
#include "mozilla/plugins/PluginScriptableObjectChild.h"

#include "npfunctions.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"

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

    virtual bool AnswerNPP_SetWindow(const NPWindow& window, NPError* rv);

    virtual bool
    AnswerNPP_GetValue_NPPVpluginScriptableNPObject(PPluginScriptableObjectChild** value,
                                                    NPError* result);

    virtual bool
    AnswerNPP_HandleEvent(const NPEvent& event, int16_t* handled);

    virtual PPluginScriptableObjectChild*
    PPluginScriptableObjectConstructor();

    virtual bool
    PPluginScriptableObjectDestructor(PPluginScriptableObjectChild* aObject);

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

    virtual bool
    PBrowserStreamDestructor(PBrowserStreamChild* stream,
                             const NPError& reason,
                             const bool& artificial);

    virtual PStreamNotifyChild*
    PStreamNotifyConstructor(const nsCString& url, const nsCString& target,
                             const bool& post, const nsCString& buffer,
                             const bool& file,
                             NPError* result);

    virtual bool
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

    PluginScriptableObjectChild*
    CreateActorForNPObject(NPObject* aObject);

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

    nsTArray<nsAutoPtr<PluginScriptableObjectChild> > mScriptableObjects;
};

} 
} 

#endif 
