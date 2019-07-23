





































#ifndef dom_plugins_PluginInstanceChild_h
#define dom_plugins_PluginInstanceChild_h 1

#include "mozilla/plugins/PPluginInstanceChild.h"
#include "mozilla/plugins/PluginScriptableObjectChild.h"
#include "mozilla/plugins/StreamNotifyChild.h"
#if defined(OS_WIN)
#include "mozilla/gfx/SharedDIBWin.h"
#endif

#include "npfunctions.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "ChildAsyncCall.h"
#include "ChildTimer.h"

namespace mozilla {
namespace plugins {

class PBrowserStreamChild;
class BrowserStreamChild;

class PluginInstanceChild : public PPluginInstanceChild
{
    friend class BrowserStreamChild;
    friend class PluginStreamChild;

#ifdef OS_WIN
    friend LRESULT CALLBACK PluginWindowProc(HWND hWnd,
                                             UINT message,
                                             WPARAM wParam,
                                             LPARAM lParam);
#endif

protected:
    virtual bool AnswerNPP_SetWindow(const NPRemoteWindow& window, NPError* rv);

    virtual bool
    AnswerNPP_GetValue_NPPVpluginWindow(bool* windowed, NPError* rv);
    virtual bool
    AnswerNPP_GetValue_NPPVpluginTransparent(bool* transparent, NPError* rv);
    virtual bool
    AnswerNPP_GetValue_NPPVpluginNeedsXEmbed(bool* needs, NPError* rv);
    virtual bool
    AnswerNPP_GetValue_NPPVpluginScriptableNPObject(PPluginScriptableObjectChild** value,
                                                    NPError* result);

    virtual bool
    AnswerNPP_SetValue_NPNVprivateModeBool(const bool& value, NPError* result);

    virtual bool
    AnswerNPP_HandleEvent(const NPRemoteEvent& event, int16_t* handled);

    virtual bool
    AnswerNPP_Destroy(NPError* result);

    virtual PPluginScriptableObjectChild*
    AllocPPluginScriptableObject();

    virtual bool
    DeallocPPluginScriptableObject(PPluginScriptableObjectChild* aObject);

    virtual bool
    AnswerPPluginScriptableObjectConstructor(PPluginScriptableObjectChild* aActor);

    virtual PBrowserStreamChild*
    AllocPBrowserStream(const nsCString& url,
                        const uint32_t& length,
                        const uint32_t& lastmodified,
                        PStreamNotifyChild* notifyData,
                        const nsCString& headers,
                        const nsCString& mimeType,
                        const bool& seekable,
                        NPError* rv,
                        uint16_t *stype);

    virtual bool
    AnswerPBrowserStreamConstructor(
            PBrowserStreamChild* aActor,
            const nsCString& url,
            const uint32_t& length,
            const uint32_t& lastmodified,
            PStreamNotifyChild* notifyData,
            const nsCString& headers,
            const nsCString& mimeType,
            const bool& seekable,
            NPError* rv,
            uint16_t* stype);
        
    virtual bool
    DeallocPBrowserStream(PBrowserStreamChild* stream);

    virtual PPluginStreamChild*
    AllocPPluginStream(const nsCString& mimeType,
                       const nsCString& target,
                       NPError* result);

    virtual bool
    DeallocPPluginStream(PPluginStreamChild* stream);

    virtual PStreamNotifyChild*
    AllocPStreamNotify(const nsCString& url, const nsCString& target,
                       const bool& post, const nsCString& buffer,
                       const bool& file,
                       NPError* result);

    NS_OVERRIDE virtual bool
    DeallocPStreamNotify(PStreamNotifyChild* notifyData);

    virtual bool
    AnswerSetPluginFocus();

    virtual bool
    AnswerUpdateWindow();

public:
    PluginInstanceChild(const NPPluginFuncs* aPluginIface);

    virtual ~PluginInstanceChild();

    bool Initialize();

    NPP GetNPP()
    {
        return &mData;
    }

    NPError
    NPN_GetValue(NPNVariable aVariable, void* aValue);

    NPError
    NPN_SetValue(NPPVariable aVariable, void* aValue);

    PluginScriptableObjectChild*
    GetActorForNPObject(NPObject* aObject);

    NPError
    NPN_NewStream(NPMIMEType aMIMEType, const char* aWindow,
                  NPStream** aStream);

    void InvalidateRect(NPRect* aInvalidRect);

    bool NotifyStream(StreamNotifyChild* notifyData, NPReason reason);

    uint32_t ScheduleTimer(uint32_t interval, bool repeat, TimerFunc func);
    void UnscheduleTimer(uint32_t id);

private:

#if defined(OS_WIN)
    static bool RegisterWindowClass();
    bool CreatePluginWindow();
    void DestroyPluginWindow();
    void ReparentPluginWindow(HWND hWndParent);
    void SizePluginWindow(int width, int height);
    int16_t WinlessHandleEvent(NPEvent& event);
    void SetNestedInputEventHook();
    void ResetNestedEventHook();
    void SetNestedInputPumpHook();
    void ResetPumpHooks();
    void InternalCallSetNestedEventState(bool aState);
    static LRESULT CALLBACK DummyWindowProc(HWND hWnd,
                                            UINT message,
                                            WPARAM wParam,
                                            LPARAM lParam);
    static LRESULT CALLBACK PluginWindowProc(HWND hWnd,
                                             UINT message,
                                             WPARAM wParam,
                                             LPARAM lParam);
    static VOID CALLBACK PumpTimerProc(HWND hwnd,
                                       UINT uMsg,
                                       UINT_PTR idEvent,
                                       DWORD dwTime);
    static LRESULT CALLBACK NestedInputEventHook(int code,
                                                 WPARAM wParam,
                                                 LPARAM lParam);
    static LRESULT CALLBACK NestedInputPumpHook(int code,
                                                WPARAM wParam,
                                                LPARAM lParam);
#endif

    const NPPluginFuncs* mPluginIface;
    NPP_t mData;
    NPWindow mWindow;

#if defined(MOZ_X11) && defined(XP_UNIX) && !defined(XP_MACOSX)
    NPSetWindowCallbackStruct mWsInfo;
#elif defined(OS_WIN)
    HWND mPluginWindowHWND;
    WNDPROC mPluginWndProc;
    HWND mPluginParentHWND;
    HHOOK mNestedEventHook;
    HHOOK mNestedPumpHook;
    int mNestedEventLevelDepth;
    bool mNestedEventState;
    HWND mCachedWinlessPluginHWND;
    UINT_PTR mEventPumpTimer;
#endif

    friend class ChildAsyncCall;
    nsTArray<ChildAsyncCall*> mPendingAsyncCalls;
    nsTArray<nsAutoPtr<ChildTimer> > mTimers;

#if defined(OS_WIN)
private:
    
    bool SharedSurfaceSetWindow(const NPRemoteWindow& aWindow, NPError* rv);
    int16_t SharedSurfacePaint(NPEvent& evcopy);
    void SharedSurfaceRelease();
    bool AlphaExtractCacheSetup();
    void AlphaExtractCacheRelease();
    void UpdatePaintClipRect(RECT* aRect);

private:
    enum {
      RENDER_NATIVE,
      RENDER_BACK_ONE,
      RENDER_BACK_TWO 
    };
    gfx::SharedDIBWin mSharedSurfaceDib;
    struct {
      PRUint32        doublePassEvent;
      PRUint16        doublePass;
      HDC             hdc;
      HBITMAP         bmp;
    } mAlphaExtract;
#endif
};

} 
} 

#endif
