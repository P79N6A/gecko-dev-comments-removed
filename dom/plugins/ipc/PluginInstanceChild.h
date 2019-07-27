





#ifndef dom_plugins_PluginInstanceChild_h
#define dom_plugins_PluginInstanceChild_h 1

#include "mozilla/plugins/PPluginInstanceChild.h"
#include "mozilla/plugins/PluginScriptableObjectChild.h"
#include "mozilla/plugins/StreamNotifyChild.h"
#include "mozilla/plugins/PPluginSurfaceChild.h"
#include "mozilla/ipc/CrossProcessMutex.h"
#include "nsClassHashtable.h"
#if defined(OS_WIN)
#include "mozilla/gfx/SharedDIBWin.h"
#elif defined(MOZ_WIDGET_COCOA)
#include "PluginUtilsOSX.h"
#include "mozilla/gfx/QuartzSupport.h"
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

#include <map>

#ifdef MOZ_WIDGET_GTK
#include "gtk2xtbin.h"
#endif

class gfxASurface;

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
    friend class PluginScriptableObjectChild;

#ifdef OS_WIN
    friend LRESULT CALLBACK PluginWindowProc(HWND hWnd,
                                             UINT message,
                                             WPARAM wParam,
                                             LPARAM lParam);
    static LRESULT CALLBACK PluginWindowProcInternal(HWND hWnd,
                                                     UINT message,
                                                     WPARAM wParam,
                                                     LPARAM lParam);
#endif

protected:
    virtual bool AnswerNPP_SetWindow(const NPRemoteWindow& window) override;

    virtual bool
    AnswerNPP_GetValue_NPPVpluginWantsAllNetworkStreams(bool* wantsAllStreams, NPError* rv) override;
    virtual bool
    AnswerNPP_GetValue_NPPVpluginNeedsXEmbed(bool* needs, NPError* rv) override;
    virtual bool
    AnswerNPP_GetValue_NPPVpluginScriptableNPObject(PPluginScriptableObjectChild** value,
                                                    NPError* result) override;
    virtual bool
    AnswerNPP_GetValue_NPPVpluginNativeAccessibleAtkPlugId(nsCString* aPlugId,
                                                           NPError* aResult) override;
    virtual bool
    AnswerNPP_SetValue_NPNVprivateModeBool(const bool& value, NPError* result) override;

    virtual bool
    AnswerNPP_HandleEvent(const NPRemoteEvent& event, int16_t* handled) override;
    virtual bool
    AnswerNPP_HandleEvent_Shmem(const NPRemoteEvent& event,
                                Shmem&& mem,
                                int16_t* handled,
                                Shmem* rtnmem) override;
    virtual bool
    AnswerNPP_HandleEvent_IOSurface(const NPRemoteEvent& event,
                                    const uint32_t& surface,
                                    int16_t* handled) override;

    
    virtual bool
    RecvAsyncSetWindow(const gfxSurfaceType& aSurfaceType,
                       const NPRemoteWindow& aWindow) override;

    virtual void
    DoAsyncSetWindow(const gfxSurfaceType& aSurfaceType,
                     const NPRemoteWindow& aWindow,
                     bool aIsAsync);

    virtual PPluginSurfaceChild*
    AllocPPluginSurfaceChild(const WindowsSharedMemoryHandle&,
                             const gfxIntSize&, const bool&) override {
        return new PPluginSurfaceChild();
    }

    virtual bool DeallocPPluginSurfaceChild(PPluginSurfaceChild* s) override {
        delete s;
        return true;
    }

    virtual bool
    AnswerPaint(const NPRemoteEvent& event, int16_t* handled) override
    {
        PaintTracker pt;
        return AnswerNPP_HandleEvent(event, handled);
    }

    virtual bool
    RecvWindowPosChanged(const NPRemoteEvent& event) override;

    virtual bool
    RecvContentsScaleFactorChanged(const double& aContentsScaleFactor) override;

    virtual bool
    AnswerNPP_Destroy(NPError* result) override;

    virtual PPluginScriptableObjectChild*
    AllocPPluginScriptableObjectChild() override;

    virtual bool
    DeallocPPluginScriptableObjectChild(PPluginScriptableObjectChild* aObject) override;

    virtual bool
    RecvPPluginScriptableObjectConstructor(PPluginScriptableObjectChild* aActor) override;

    virtual bool
    RecvPBrowserStreamConstructor(PBrowserStreamChild* aActor, const nsCString& aURL,
                                  const uint32_t& aLength, const uint32_t& aLastmodified,
                                  PStreamNotifyChild* aNotifyData, const nsCString& aHeaders) override;

    virtual bool
    AnswerNPP_NewStream(
            PBrowserStreamChild* actor,
            const nsCString& mimeType,
            const bool& seekable,
            NPError* rv,
            uint16_t* stype) override;

    virtual bool
    RecvAsyncNPP_NewStream(
            PBrowserStreamChild* actor,
            const nsCString& mimeType,
            const bool& seekable) override;

    virtual PBrowserStreamChild*
    AllocPBrowserStreamChild(const nsCString& url,
                             const uint32_t& length,
                             const uint32_t& lastmodified,
                             PStreamNotifyChild* notifyData,
                             const nsCString& headers) override;

    virtual bool
    DeallocPBrowserStreamChild(PBrowserStreamChild* stream) override;

    virtual PPluginStreamChild*
    AllocPPluginStreamChild(const nsCString& mimeType,
                            const nsCString& target,
                            NPError* result) override;

    virtual bool
    DeallocPPluginStreamChild(PPluginStreamChild* stream) override;

    virtual PStreamNotifyChild*
    AllocPStreamNotifyChild(const nsCString& url, const nsCString& target,
                            const bool& post, const nsCString& buffer,
                            const bool& file,
                            NPError* result) override;

    virtual bool
    DeallocPStreamNotifyChild(PStreamNotifyChild* notifyData) override;

    virtual bool
    AnswerSetPluginFocus() override;

    virtual bool
    AnswerUpdateWindow() override;

    virtual bool
    RecvNPP_DidComposite() override;

#if defined(MOZ_X11) && defined(XP_UNIX) && !defined(XP_MACOSX)
    bool CreateWindow(const NPRemoteWindow& aWindow);
    void DeleteWindow();
#endif

public:
    PluginInstanceChild(const NPPluginFuncs* aPluginIface,
                        const nsCString& aMimeType,
                        const uint16_t& aMode,
                        const InfallibleTArray<nsCString>& aNames,
                        const InfallibleTArray<nsCString>& aValues);

    virtual ~PluginInstanceChild();

    NPError DoNPP_New();

    
    NPError DoNPP_NewStream(BrowserStreamChild* actor,
                            const nsCString& mimeType,
                            const bool& seekable,
                            uint16_t* stype);

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

#ifdef MOZ_WIDGET_COCOA
    void Invalidate();
#endif 

    uint32_t ScheduleTimer(uint32_t interval, bool repeat, TimerFunc func);
    void UnscheduleTimer(uint32_t id);

    void AsyncCall(PluginThreadCallback aFunc, void* aUserData);
    
    void PostChildAsyncCall(ChildAsyncCall* aTask);

    int GetQuirks();

    void NPN_URLRedirectResponse(void* notifyData, NPBool allow);

    void DoAsyncRedraw();
private:
    friend class PluginModuleChild;

    NPError
    InternalGetNPObjectForValue(NPNVariable aValue,
                                NPObject** aObject);

    bool IsAsyncDrawing();

    virtual bool RecvUpdateBackground(const SurfaceDescriptor& aBackground,
                                      const nsIntRect& aRect) override;

    virtual PPluginBackgroundDestroyerChild*
    AllocPPluginBackgroundDestroyerChild() override;

    virtual bool
    RecvPPluginBackgroundDestroyerConstructor(PPluginBackgroundDestroyerChild* aActor) override;

    virtual bool
    DeallocPPluginBackgroundDestroyerChild(PPluginBackgroundDestroyerChild* aActor) override;

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
    void HookSetWindowLongPtr();
    static inline bool SetWindowLongHookCheck(HWND hWnd,
                                                int nIndex,
                                                LONG_PTR newLong);
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
#ifdef _WIN64
    static LONG_PTR WINAPI SetWindowLongPtrAHook(HWND hWnd,
                                                 int nIndex,
                                                 LONG_PTR newLong);
    static LONG_PTR WINAPI SetWindowLongPtrWHook(HWND hWnd,
                                                 int nIndex,
                                                 LONG_PTR newLong);
                      
#else
    static LONG WINAPI SetWindowLongAHook(HWND hWnd,
                                          int nIndex,
                                          LONG newLong);
    static LONG WINAPI SetWindowLongWHook(HWND hWnd,
                                          int nIndex,
                                          LONG newLong);
#endif

    class FlashThrottleAsyncMsg : public ChildAsyncCall
    {
      public:
        FlashThrottleAsyncMsg();
        FlashThrottleAsyncMsg(PluginInstanceChild* aInst, 
                              HWND aWnd, UINT aMsg,
                              WPARAM aWParam, LPARAM aLParam,
                              bool isWindowed)
          : ChildAsyncCall(aInst, nullptr, nullptr),
          mWnd(aWnd),
          mMsg(aMsg),
          mWParam(aWParam),
          mLParam(aLParam),
          mWindowed(isWindowed)
        {}

        void Run() override;

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
    nsCString                   mMimeType;
    uint16_t                    mMode;
    InfallibleTArray<nsCString> mNames;
    InfallibleTArray<nsCString> mValues;
    NPP_t mData;
    NPWindow mWindow;
#if defined(XP_MACOSX)
    double mContentsScaleFactor;
#endif
    int16_t               mDrawingModel;

    mozilla::Mutex mAsyncInvalidateMutex;
    CancelableTask *mAsyncInvalidateTask;

    
    PluginScriptableObjectChild* mCachedWindowActor;
    PluginScriptableObjectChild* mCachedElementActor;

#if defined(MOZ_X11) && defined(XP_UNIX) && !defined(XP_MACOSX)
    NPSetWindowCallbackStruct mWsInfo;
#ifdef MOZ_WIDGET_GTK
    bool mXEmbed;
    XtClient mXtClient;
#endif
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
      uint16_t        doublePass;
      HDC             hdc;
      HBITMAP         bmp;
    } mAlphaExtract;
#endif
#if defined(MOZ_WIDGET_COCOA)
private:
#if defined(__i386__)
    NPEventModel                  mEventModel;
#endif
    CGColorSpaceRef               mShColorSpace;
    CGContextRef                  mShContext;
    mozilla::RefPtr<nsCARenderer> mCARenderer;
    void                         *mCGLayer;

    
    uint32_t                      mCARefreshTimer;

public:
    const NPCocoaEvent* getCurrentEvent() {
        return mCurrentEvent;
    }
  
    bool CGDraw(CGContextRef ref, nsIntRect aUpdateRect);

#if defined(__i386__)
    NPEventModel EventModel() { return mEventModel; }
#endif

private:
    const NPCocoaEvent   *mCurrentEvent;
#endif

    bool CanPaintOnBackground();

    bool IsVisible() {
#ifdef XP_MACOSX
        return mWindow.clipRect.top != mWindow.clipRect.bottom &&
               mWindow.clipRect.left != mWindow.clipRect.right;
#else
        return mWindow.clipRect.top != 0 ||
            mWindow.clipRect.left != 0 ||
            mWindow.clipRect.bottom != 0 ||
            mWindow.clipRect.right != 0;
#endif
    }

    
    
    
    
    
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

    
    void ClearCurrentSurface();

    
    void SwapSurfaces();

    
    void ClearAllSurfaces();

    void Destroy();

    void ActorDestroy(ActorDestroyReason aWhy) override;

    
    
    bool mLayersRendering;

    
    nsRefPtr<gfxASurface> mCurrentSurface;

    
    
    nsRefPtr<gfxASurface> mBackSurface;

#ifdef XP_MACOSX
    
    
    PluginUtilsOSX::nsDoubleBufferCARenderer mDoubleBufferCARenderer; 
#endif

    
    
    
    
    
    nsRefPtr<gfxASurface> mBackground;

#ifdef XP_WIN
    
    PPluginSurfaceChild* mCurrentSurfaceActor;
    PPluginSurfaceChild* mBackSurfaceActor;
#endif

    
    
    nsIntRect mAccumulatedInvalidRect;

    
    
    
    bool mIsTransparent;

    
    gfxSurfaceType mSurfaceType;

    
    CancelableTask *mCurrentInvalidateTask;

    
    CancelableTask *mCurrentAsyncSetWindowTask;

    
    
    bool mPendingPluginCall;

    
    
    
    
    nsRefPtr<gfxASurface> mHelperSurface;

    
    
    
    
    bool mDoAlphaExtraction;

    
    
    
    bool mHasPainted;

    
    
    
    nsIntRect mSurfaceDifferenceRect;

    
    bool mDestroyed;
};

} 
} 

#endif
