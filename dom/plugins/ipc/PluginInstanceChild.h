





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

namespace layers {
struct RemoteImageData;
}

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
    static LRESULT CALLBACK PluginWindowProcInternal(HWND hWnd,
                                                     UINT message,
                                                     WPARAM wParam,
                                                     LPARAM lParam);
#endif

protected:
    virtual bool AnswerNPP_SetWindow(const NPRemoteWindow& window) MOZ_OVERRIDE;

    virtual bool
    AnswerNPP_GetValue_NPPVpluginWantsAllNetworkStreams(bool* wantsAllStreams, NPError* rv) MOZ_OVERRIDE;
    virtual bool
    AnswerNPP_GetValue_NPPVpluginNeedsXEmbed(bool* needs, NPError* rv) MOZ_OVERRIDE;
    virtual bool
    AnswerNPP_GetValue_NPPVpluginScriptableNPObject(PPluginScriptableObjectChild** value,
                                                    NPError* result) MOZ_OVERRIDE;
    virtual bool
    AnswerNPP_GetValue_NPPVpluginNativeAccessibleAtkPlugId(nsCString* aPlugId,
                                                           NPError* aResult) MOZ_OVERRIDE;
    virtual bool
    AnswerNPP_SetValue_NPNVprivateModeBool(const bool& value, NPError* result) MOZ_OVERRIDE;

    virtual bool
    AnswerNPP_HandleEvent(const NPRemoteEvent& event, int16_t* handled) MOZ_OVERRIDE;
    virtual bool
    AnswerNPP_HandleEvent_Shmem(const NPRemoteEvent& event,
                                Shmem& mem,
                                int16_t* handled,
                                Shmem* rtnmem) MOZ_OVERRIDE;
    virtual bool
    AnswerNPP_HandleEvent_IOSurface(const NPRemoteEvent& event,
                                    const uint32_t& surface,
                                    int16_t* handled) MOZ_OVERRIDE;

    
    virtual bool
    RecvAsyncSetWindow(const gfxSurfaceType& aSurfaceType,
                       const NPRemoteWindow& aWindow) MOZ_OVERRIDE;

    virtual void
    DoAsyncSetWindow(const gfxSurfaceType& aSurfaceType,
                     const NPRemoteWindow& aWindow,
                     bool aIsAsync);

    virtual PPluginSurfaceChild*
    AllocPPluginSurfaceChild(const WindowsSharedMemoryHandle&,
                             const gfxIntSize&, const bool&) MOZ_OVERRIDE {
        return new PPluginSurfaceChild();
    }

    virtual bool DeallocPPluginSurfaceChild(PPluginSurfaceChild* s) MOZ_OVERRIDE {
        delete s;
        return true;
    }

    virtual bool
    AnswerPaint(const NPRemoteEvent& event, int16_t* handled) MOZ_OVERRIDE
    {
        PaintTracker pt;
        return AnswerNPP_HandleEvent(event, handled);
    }

    virtual bool
    RecvWindowPosChanged(const NPRemoteEvent& event) MOZ_OVERRIDE;

    virtual bool
    RecvContentsScaleFactorChanged(const double& aContentsScaleFactor) MOZ_OVERRIDE;

    virtual bool
    AnswerNPP_Destroy(NPError* result) MOZ_OVERRIDE;

    virtual PPluginScriptableObjectChild*
    AllocPPluginScriptableObjectChild() MOZ_OVERRIDE;

    virtual bool
    DeallocPPluginScriptableObjectChild(PPluginScriptableObjectChild* aObject) MOZ_OVERRIDE;

    virtual bool
    RecvPPluginScriptableObjectConstructor(PPluginScriptableObjectChild* aActor) MOZ_OVERRIDE;

    virtual PBrowserStreamChild*
    AllocPBrowserStreamChild(const nsCString& url,
                             const uint32_t& length,
                             const uint32_t& lastmodified,
                             PStreamNotifyChild* notifyData,
                             const nsCString& headers,
                             const nsCString& mimeType,
                             const bool& seekable,
                             NPError* rv,
                             uint16_t *stype) MOZ_OVERRIDE;

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
            uint16_t* stype) MOZ_OVERRIDE;

    virtual bool
    DeallocPBrowserStreamChild(PBrowserStreamChild* stream) MOZ_OVERRIDE;

    virtual PPluginStreamChild*
    AllocPPluginStreamChild(const nsCString& mimeType,
                            const nsCString& target,
                            NPError* result) MOZ_OVERRIDE;

    virtual bool
    DeallocPPluginStreamChild(PPluginStreamChild* stream) MOZ_OVERRIDE;

    virtual PStreamNotifyChild*
    AllocPStreamNotifyChild(const nsCString& url, const nsCString& target,
                            const bool& post, const nsCString& buffer,
                            const bool& file,
                            NPError* result) MOZ_OVERRIDE;

    virtual bool
    DeallocPStreamNotifyChild(PStreamNotifyChild* notifyData) MOZ_OVERRIDE;

    virtual bool
    AnswerSetPluginFocus() MOZ_OVERRIDE;

    virtual bool
    AnswerUpdateWindow() MOZ_OVERRIDE;

    virtual bool
    RecvNPP_DidComposite() MOZ_OVERRIDE;

#if defined(MOZ_X11) && defined(XP_UNIX) && !defined(XP_MACOSX)
    bool CreateWindow(const NPRemoteWindow& aWindow);
    void DeleteWindow();
#endif

public:
    explicit PluginInstanceChild(const NPPluginFuncs* aPluginIface);

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

#ifdef MOZ_WIDGET_COCOA
    void Invalidate();
#endif 

    uint32_t ScheduleTimer(uint32_t interval, bool repeat, TimerFunc func);
    void UnscheduleTimer(uint32_t id);

    void AsyncCall(PluginThreadCallback aFunc, void* aUserData);

    int GetQuirks();

    void NPN_URLRedirectResponse(void* notifyData, NPBool allow);

    NPError NPN_InitAsyncSurface(NPSize *size, NPImageFormat format,
                                 void *initData, NPAsyncSurface *surface);
    NPError NPN_FinalizeAsyncSurface(NPAsyncSurface *surface);

    void NPN_SetCurrentAsyncSurface(NPAsyncSurface *surface, NPRect *changed);

    void DoAsyncRedraw();
private:
    friend class PluginModuleChild;

    NPError
    InternalGetNPObjectForValue(NPNVariable aValue,
                                NPObject** aObject);

    bool IsAsyncDrawing();

    NPError DeallocateAsyncBitmapSurface(NPAsyncSurface *aSurface);

    virtual bool RecvUpdateBackground(const SurfaceDescriptor& aBackground,
                                      const nsIntRect& aRect) MOZ_OVERRIDE;

    virtual PPluginBackgroundDestroyerChild*
    AllocPPluginBackgroundDestroyerChild() MOZ_OVERRIDE;

    virtual bool
    RecvPPluginBackgroundDestroyerConstructor(PPluginBackgroundDestroyerChild* aActor) MOZ_OVERRIDE;

    virtual bool
    DeallocPPluginBackgroundDestroyerChild(PPluginBackgroundDestroyerChild* aActor) MOZ_OVERRIDE;

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

        void Run() MOZ_OVERRIDE;

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
#if defined(XP_MACOSX)
    double mContentsScaleFactor;
#endif
    int16_t               mDrawingModel;
    NPAsyncSurface* mCurrentAsyncSurface;
    struct AsyncBitmapData {
      void *mRemotePtr;
      Shmem mShmem;
    };

    static PLDHashOperator DeleteSurface(NPAsyncSurface* surf, nsAutoPtr<AsyncBitmapData> &data, void* userArg);
    nsClassHashtable<nsPtrHashKey<NPAsyncSurface>, AsyncBitmapData> mAsyncBitmaps;
    Shmem mRemoteImageDataShmem;
    mozilla::layers::RemoteImageData *mRemoteImageData;
    nsAutoPtr<CrossProcessMutex> mRemoteImageDataMutex;
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
};

} 
} 

#endif
