





































#ifndef dom_plugins_PluginInstanceChild_h
#define dom_plugins_PluginInstanceChild_h 1

#include "mozilla/plugins/PPluginInstanceChild.h"
#include "mozilla/plugins/PluginScriptableObjectChild.h"
#include "mozilla/plugins/StreamNotifyChild.h"
#if defined(OS_WIN)
#include "mozilla/gfx/SharedDIBWin.h"
#elif defined(OS_MACOSX)
#include "nsCoreAnimationSupport.h"
#include "base/timer.h"
#endif

#include "npfunctions.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "ChildAsyncCall.h"
#include "ChildTimer.h"
#include "nsRect.h"
#include "nsTHashtable.h"
#include "mozilla/PaintTracker.h"
#include "gfxASurface.h"

namespace mozilla {
namespace plugins {

class PBrowserStreamChild;
class BrowserStreamChild;
class StreamNotifyChild;

class PluginInstanceChild : public PPluginInstanceChild
{
    friend class BrowserStreamChild;
    friend class PluginStreamChild;
    friend class StreamNotifyChild; 

#ifdef OS_WIN
    friend LRESULT CALLBACK PluginWindowProc(HWND hWnd,
                                             UINT message,
                                             WPARAM wParam,
                                             LPARAM lParam);
#endif

protected:
    virtual bool AnswerNPP_SetWindow(const NPRemoteWindow& window);

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
    AnswerNPP_HandleEvent_Shmem(const NPRemoteEvent& event, Shmem& mem, int16_t* handled, Shmem* rtnmem);
    virtual bool
    AnswerNPP_HandleEvent_IOSurface(const NPRemoteEvent& event, const uint32_t& surface, int16_t* handled);

    
    virtual bool
    RecvAsyncSetWindow(const gfxSurfaceType& aSurfaceType,
                       const NPRemoteWindow& aWindow);
    virtual bool RecvPaintFinished(void);

    NS_OVERRIDE
    virtual bool
    AnswerPaint(const NPRemoteEvent& event, int16_t* handled)
    {
        PaintTracker pt;
        return AnswerNPP_HandleEvent(event, handled);
    }

    NS_OVERRIDE
    virtual bool
    RecvWindowPosChanged(const NPRemoteEvent& event);

    virtual bool
    AnswerNPP_Destroy(NPError* result);

    virtual PPluginScriptableObjectChild*
    AllocPPluginScriptableObject();

    virtual bool
    DeallocPPluginScriptableObject(PPluginScriptableObjectChild* aObject);

    NS_OVERRIDE virtual bool
    RecvPPluginScriptableObjectConstructor(PPluginScriptableObjectChild* aActor);

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
    PluginInstanceChild(const NPPluginFuncs* aPluginIface, const nsCString& aMimeType);

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

    uint32_t ScheduleTimer(uint32_t interval, bool repeat, TimerFunc func);
    void UnscheduleTimer(uint32_t id);

    void AsyncCall(PluginThreadCallback aFunc, void* aUserData);

private:
    friend class PluginModuleChild;

    
    enum PluginQuirks {
        
        
        
        QUIRK_SILVERLIGHT_WINLESS_INPUT_TRANSLATION     = 1 << 0,
        
        
        
        QUIRK_WINLESS_TRACKPOPUP_HOOK                   = 1 << 1,
        
        
        
        QUIRK_FLASH_THROTTLE_WMUSER_EVENTS              = 1 << 2,
    };

    void InitQuirksModes(const nsCString& aMimeType);

    NPError
    InternalGetNPObjectForValue(NPNVariable aValue,
                                NPObject** aObject);

#if defined(OS_WIN)
    static bool RegisterWindowClass();
    bool CreatePluginWindow();
    void DestroyPluginWindow();
    void ReparentPluginWindow(HWND hWndParent);
    void SizePluginWindow(int width, int height);
    int16_t WinlessHandleEvent(NPEvent& event);
    void CreateWinlessPopupSurrogate();
    void DestroyWinlessPopupSurrogate();
    void InitPopupMenuHook();
    void SetupFlashMsgThrottle();
    void UnhookWinlessFlashThrottle();
    void FlashThrottleMessage(HWND, UINT, WPARAM, LPARAM, bool);
    static LRESULT CALLBACK DummyWindowProc(HWND hWnd,
                                            UINT message,
                                            WPARAM wParam,
                                            LPARAM lParam);
    static LRESULT CALLBACK PluginWindowProc(HWND hWnd,
                                             UINT message,
                                             WPARAM wParam,
                                             LPARAM lParam);
    static BOOL WINAPI TrackPopupHookProc(HMENU hMenu,
                                          UINT uFlags,
                                          int x,
                                          int y,
                                          int nReserved,
                                          HWND hWnd,
                                          CONST RECT *prcRect);
    static BOOL CALLBACK EnumThreadWindowsCallback(HWND hWnd,
                                                   LPARAM aParam);
    static LRESULT CALLBACK WinlessHiddenFlashWndProc(HWND hWnd,
                                                      UINT message,
                                                      WPARAM wParam,
                                                      LPARAM lParam);

    class FlashThrottleAsyncMsg : public ChildAsyncCall
    {
      public:
        FlashThrottleAsyncMsg();
        FlashThrottleAsyncMsg(PluginInstanceChild* aInst, 
                              HWND aWnd, UINT aMsg,
                              WPARAM aWParam, LPARAM aLParam,
                              bool isWindowed)
          : ChildAsyncCall(aInst, nsnull, nsnull),
          mWnd(aWnd),
          mMsg(aMsg),
          mWParam(aWParam),
          mLParam(aLParam),
          mWindowed(isWindowed)
        {}

        NS_OVERRIDE void Run();

        WNDPROC GetProc();
        HWND GetWnd() { return mWnd; }
        UINT GetMsg() { return mMsg; }
        WPARAM GetWParam() { return mWParam; }
        LPARAM GetLParam() { return mLParam; }

      private:
        HWND                 mWnd;
        UINT                 mMsg;
        WPARAM               mWParam;
        LPARAM               mLParam;
        bool                 mWindowed;
    };

#endif

    const NPPluginFuncs* mPluginIface;
    NPP_t mData;
    NPWindow mWindow;
    int mQuirks;

    
    PluginScriptableObjectChild* mCachedWindowActor;
    PluginScriptableObjectChild* mCachedElementActor;

#if defined(MOZ_X11) && defined(XP_UNIX) && !defined(XP_MACOSX)
    NPSetWindowCallbackStruct mWsInfo;
#elif defined(OS_WIN)
    HWND mPluginWindowHWND;
    WNDPROC mPluginWndProc;
    HWND mPluginParentHWND;
    int mNestedEventLevelDepth;
    HWND mCachedWinlessPluginHWND;
    HWND mWinlessPopupSurrogateHWND;
    nsIntPoint mPluginSize;
    WNDPROC mWinlessThrottleOldWndProc;
    HWND mWinlessHiddenMsgHWND;
#endif

    friend class ChildAsyncCall;

    Mutex mAsyncCallMutex;
    nsTArray<ChildAsyncCall*> mPendingAsyncCalls;
    nsTArray<nsAutoPtr<ChildTimer> > mTimers;

    




    nsAutoPtr< nsTHashtable<DeletingObjectEntry> > mDeletingHash;

#if defined(OS_WIN)
private:
    
    bool SharedSurfaceSetWindow(const NPRemoteWindow& aWindow);
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
      PRUint16        doublePass;
      HDC             hdc;
      HBITMAP         bmp;
    } mAlphaExtract;
#endif
#if defined(OS_MACOSX)
private:
#if defined(__i386__)
    NPEventModel          mEventModel;
#endif
    CGColorSpaceRef       mShColorSpace;
    CGContextRef          mShContext;
    int16_t               mDrawingModel;
    nsCARenderer          mCARenderer;

public:
    const NPCocoaEvent* getCurrentEvent() {
        return mCurrentEvent;
    }

#if defined(__i386__)
    NPEventModel EventModel() { return mEventModel; }
#endif

private:
    const NPCocoaEvent   *mCurrentEvent;
#endif

    
    
    
    
    
    bool ShowPluginFrame(void);

    
    
    
    bool ReadbackDifferenceRect(const nsIntRect& rect);

    
    void AsyncShowPluginFrame(void);

    
    
    

    
    void PaintRectToSurface(const nsIntRect& aRect,
                            gfxASurface* aSurface,
                            const gfxRGBA& aColor);

    
    
    void PaintRectWithAlphaExtraction(const nsIntRect& aRect,
                                      gfxASurface* aSurface);

    
    
    
    void PaintRectToPlatformSurface(const nsIntRect& aRect,
                                    gfxASurface* aSurface);

    
    
    void UpdateWindowAttributes(bool aForceSetWindow = false);

    
    
    bool CreateOptSurface(void);

    
    
    bool MaybeCreatePlatformHelperSurface(void);

    
    bool EnsureCurrentBuffer(void);

    
    
    void InvalidateRectDelayed(void);

    
    
    bool mLayersRendering;

    
    nsRefPtr<gfxASurface> mCurrentSurface;

    
    
    nsRefPtr<gfxASurface> mBackSurface;

    
    
    nsIntRect mAccumulatedInvalidRect;

    
    
    
    bool mIsTransparent;

    
    gfxSurfaceType mSurfaceType;

    
    
    bool mPendingForcePaint;

    
    CancelableTask *mCurrentInvalidateTask;

    
    
    bool mPendingPluginCall;

    
    
    
    
    nsRefPtr<gfxASurface> mHelperSurface;

    
    
    
    bool mDoAlphaExtraction;

    
    
    
    nsIntRect mSurfaceDifferenceRect;

#ifdef MOZ_X11
    
    bool                  mFlash10Quirks;
#endif
#if (MOZ_PLATFORM_MAEMO == 5) || (MOZ_PLATFORM_MAEMO == 6)
    
    
    PRPackedBool          mMaemoImageRendering;
#endif
};

} 
} 

#endif
