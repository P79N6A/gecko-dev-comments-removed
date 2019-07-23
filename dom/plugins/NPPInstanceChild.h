





































#ifndef dom_plugins_NPPInstanceChild_h
#define dom_plugins_NPPInstanceChild_h 1

#include "mozilla/plugins/NPPProtocolChild.h"

#include "npfunctions.h"

#undef _MOZ_LOG
#define _MOZ_LOG(s) printf("[NPPInstanceChild] %s\n", s)

namespace mozilla {
namespace plugins {


class NPPInstanceChild : public NPPProtocolChild
{
#ifdef OS_WIN
    friend LRESULT CALLBACK PluginWindowProc(HWND hWnd,
                                             UINT message,
                                             WPARAM wParam,
                                             LPARAM lParam);
#endif

protected:
    virtual nsresult AnswerNPP_SetWindow(const NPWindow& window, NPError* rv);

    virtual nsresult AnswerNPP_GetValue(const nsString& key, nsString* value);

public:
    NPPInstanceChild(const NPPluginFuncs* aPluginIface) :
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

    virtual ~NPPInstanceChild();

    bool Initialize();

    NPP GetNPP()
    {
        return &mData;
    }

    NPError NPN_GetValue(NPNVariable aVariable, void* aValue);

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
