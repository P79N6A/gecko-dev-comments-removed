





#ifndef mozilla_dom_TabChild_h
#define mozilla_dom_TabChild_h

#include "mozilla/dom/PBrowserChild.h"
#include "nsIWebNavigation.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsEventDispatcher.h"
#include "nsIWebBrowserChrome2.h"
#include "nsIEmbeddingSiteWindow.h"
#include "nsIWebBrowserChromeFocus.h"
#include "nsIDOMEventListener.h"
#include "nsIInterfaceRequestor.h"
#include "nsIWindowProvider.h"
#include "nsIDOMWindow.h"
#include "nsIDocShell.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsFrameMessageManager.h"
#include "nsIWebProgressListener.h"
#include "nsDOMEventTargetHelper.h"
#include "nsIPresShell.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsWeakReference.h"
#include "nsITabChild.h"
#include "nsITooltipListener.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/TabContext.h"
#include "mozilla/EventForwards.h"
#include "mozilla/layers/CompositorTypes.h"

class nsICachedFileDescriptorListener;
class nsIDOMWindowUtils;

namespace mozilla {
namespace layout {
class RenderFrameChild;
}

namespace dom {

class TabChild;
class ClonedMessageData;

class TabChildGlobal : public nsDOMEventTargetHelper,
                       public nsIContentFrameMessageManager,
                       public nsIScriptObjectPrincipal,
                       public nsIGlobalObject
{
public:
  TabChildGlobal(TabChild* aTabChild);
  void Init();
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(TabChildGlobal, nsDOMEventTargetHelper)
  NS_FORWARD_SAFE_NSIMESSAGELISTENERMANAGER(mMessageManager)
  NS_FORWARD_SAFE_NSIMESSAGESENDER(mMessageManager)
  NS_IMETHOD SendSyncMessage(const nsAString& aMessageName,
                             JS::Handle<JS::Value> aObject,
                             JS::Handle<JS::Value> aRemote,
                             nsIPrincipal* aPrincipal,
                             JSContext* aCx,
                             uint8_t aArgc,
                             JS::MutableHandle<JS::Value> aRetval)
  {
    return mMessageManager
      ? mMessageManager->SendSyncMessage(aMessageName, aObject, aRemote,
                                         aPrincipal, aCx, aArgc, aRetval)
      : NS_ERROR_NULL_POINTER;
  }
  NS_IMETHOD SendRpcMessage(const nsAString& aMessageName,
                            JS::Handle<JS::Value> aObject,
                            JS::Handle<JS::Value> aRemote,
                            nsIPrincipal* aPrincipal,
                            JSContext* aCx,
                            uint8_t aArgc,
                            JS::MutableHandle<JS::Value> aRetval)
  {
    return mMessageManager
      ? mMessageManager->SendRpcMessage(aMessageName, aObject, aRemote,
                                        aPrincipal, aCx, aArgc, aRetval)
      : NS_ERROR_NULL_POINTER;
  }
  NS_IMETHOD GetContent(nsIDOMWindow** aContent) MOZ_OVERRIDE;
  NS_IMETHOD GetDocShell(nsIDocShell** aDocShell) MOZ_OVERRIDE;
  NS_IMETHOD Dump(const nsAString& aStr) MOZ_OVERRIDE
  {
    return mMessageManager ? mMessageManager->Dump(aStr) : NS_OK;
  }
  NS_IMETHOD PrivateNoteIntentionalCrash() MOZ_OVERRIDE;
  NS_IMETHOD Btoa(const nsAString& aBinaryData,
                  nsAString& aAsciiBase64String) MOZ_OVERRIDE;
  NS_IMETHOD Atob(const nsAString& aAsciiString,
                  nsAString& aBinaryData) MOZ_OVERRIDE;

  NS_IMETHOD AddEventListener(const nsAString& aType,
                              nsIDOMEventListener* aListener,
                              bool aUseCapture)
  {
    
    return nsDOMEventTargetHelper::AddEventListener(aType, aListener,
                                                    aUseCapture, false, 2);
  }
  using nsDOMEventTargetHelper::AddEventListener;
  NS_IMETHOD AddEventListener(const nsAString& aType,
                              nsIDOMEventListener* aListener,
                              bool aUseCapture, bool aWantsUntrusted,
                              uint8_t optional_argc) MOZ_OVERRIDE
  {
    return nsDOMEventTargetHelper::AddEventListener(aType, aListener,
                                                    aUseCapture,
                                                    aWantsUntrusted,
                                                    optional_argc);
  }

  nsresult
  PreHandleEvent(nsEventChainPreVisitor& aVisitor)
  {
    aVisitor.mForceContentDispatch = true;
    return NS_OK;
  }

  virtual JSContext* GetJSContextForEventHandlers() MOZ_OVERRIDE;
  virtual nsIPrincipal* GetPrincipal() MOZ_OVERRIDE;
  virtual JSObject* GetGlobalJSObject() MOZ_OVERRIDE;

  nsCOMPtr<nsIContentFrameMessageManager> mMessageManager;
  TabChild* mTabChild;
};

class ContentListener MOZ_FINAL : public nsIDOMEventListener
{
public:
  ContentListener(TabChild* aTabChild) : mTabChild(aTabChild) {}
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMEVENTLISTENER
protected:
  TabChild* mTabChild;
};

class TabChild : public PBrowserChild,
                 public nsFrameScriptExecutor,
                 public nsIWebBrowserChrome2,
                 public nsIEmbeddingSiteWindow,
                 public nsIWebBrowserChromeFocus,
                 public nsIInterfaceRequestor,
                 public nsIWindowProvider,
                 public nsIDOMEventListener,
                 public nsIWebProgressListener,
                 public nsSupportsWeakReference,
                 public nsITabChild,
                 public nsIObserver,
                 public ipc::MessageManagerCallback,
                 public TabContext,
                 public nsITooltipListener
{
    typedef mozilla::dom::ClonedMessageData ClonedMessageData;
    typedef mozilla::layout::RenderFrameChild RenderFrameChild;
    typedef mozilla::layout::ScrollingBehavior ScrollingBehavior;

public:
    




    static void PreloadSlowThings();

    
    static already_AddRefed<TabChild>
    Create(ContentChild* aManager, const TabContext& aContext, uint32_t aChromeFlags);

    virtual ~TabChild();

    bool IsRootContentDocument();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBBROWSERCHROME
    NS_DECL_NSIWEBBROWSERCHROME2
    NS_DECL_NSIEMBEDDINGSITEWINDOW
    NS_DECL_NSIWEBBROWSERCHROMEFOCUS
    NS_DECL_NSIINTERFACEREQUESTOR
    NS_DECL_NSIWINDOWPROVIDER
    NS_DECL_NSIDOMEVENTLISTENER
    NS_DECL_NSIWEBPROGRESSLISTENER
    NS_DECL_NSITABCHILD
    NS_DECL_NSIOBSERVER
    NS_DECL_NSITOOLTIPLISTENER

    


    virtual bool DoSendBlockingMessage(JSContext* aCx,
                                       const nsAString& aMessage,
                                       const mozilla::dom::StructuredCloneData& aData,
                                       JS::Handle<JSObject *> aCpows,
                                       nsIPrincipal* aPrincipal,
                                       InfallibleTArray<nsString>* aJSONRetVal,
                                       bool aIsSync) MOZ_OVERRIDE;
    virtual bool DoSendAsyncMessage(JSContext* aCx,
                                    const nsAString& aMessage,
                                    const mozilla::dom::StructuredCloneData& aData,
                                    JS::Handle<JSObject *> aCpows,
                                    nsIPrincipal* aPrincipal) MOZ_OVERRIDE;

    virtual bool RecvLoadURL(const nsCString& uri) MOZ_OVERRIDE;
    virtual bool RecvCacheFileDescriptor(const nsString& aPath,
                                         const FileDescriptor& aFileDescriptor)
                                         MOZ_OVERRIDE;
    virtual bool RecvShow(const nsIntSize& size) MOZ_OVERRIDE;
    virtual bool RecvUpdateDimensions(const nsRect& rect,
                                      const nsIntSize& size,
                                      const ScreenOrientation& orientation) MOZ_OVERRIDE;
    virtual bool RecvUpdateFrame(const mozilla::layers::FrameMetrics& aFrameMetrics) MOZ_OVERRIDE;
    virtual bool RecvAcknowledgeScrollUpdate(const ViewID& aScrollId,
                                             const uint32_t& aScrollGeneration) MOZ_OVERRIDE;
    virtual bool RecvHandleDoubleTap(const CSSIntPoint& aPoint,
                                     const mozilla::layers::ScrollableLayerGuid& aGuid) MOZ_OVERRIDE;
    virtual bool RecvHandleSingleTap(const CSSIntPoint& aPoint,
                                     const mozilla::layers::ScrollableLayerGuid& aGuid) MOZ_OVERRIDE;
    virtual bool RecvHandleLongTap(const CSSIntPoint& aPoint,
                                   const mozilla::layers::ScrollableLayerGuid& aGuid) MOZ_OVERRIDE;
    virtual bool RecvHandleLongTapUp(const CSSIntPoint& aPoint,
                                     const mozilla::layers::ScrollableLayerGuid& aGuid) MOZ_OVERRIDE;
    virtual bool RecvNotifyTransformBegin(const ViewID& aViewId) MOZ_OVERRIDE;
    virtual bool RecvNotifyTransformEnd(const ViewID& aViewId) MOZ_OVERRIDE;
    virtual bool RecvActivate() MOZ_OVERRIDE;
    virtual bool RecvDeactivate() MOZ_OVERRIDE;
    virtual bool RecvMouseEvent(const nsString& aType,
                                const float&    aX,
                                const float&    aY,
                                const int32_t&  aButton,
                                const int32_t&  aClickCount,
                                const int32_t&  aModifiers,
                                const bool&     aIgnoreRootScrollFrame) MOZ_OVERRIDE;
    virtual bool RecvRealMouseEvent(const mozilla::WidgetMouseEvent& event) MOZ_OVERRIDE;
    virtual bool RecvRealKeyEvent(const mozilla::WidgetKeyboardEvent& event) MOZ_OVERRIDE;
    virtual bool RecvMouseWheelEvent(const mozilla::WidgetWheelEvent& event) MOZ_OVERRIDE;
    virtual bool RecvRealTouchEvent(const WidgetTouchEvent& aEvent,
                                    const ScrollableLayerGuid& aGuid) MOZ_OVERRIDE;
    virtual bool RecvRealTouchMoveEvent(const WidgetTouchEvent& aEvent,
                                        const ScrollableLayerGuid& aGuid) MOZ_OVERRIDE;
    virtual bool RecvKeyEvent(const nsString& aType,
                              const int32_t&  aKeyCode,
                              const int32_t&  aCharCode,
                              const int32_t&  aModifiers,
                              const bool&     aPreventDefault) MOZ_OVERRIDE;
    virtual bool RecvCompositionEvent(const mozilla::WidgetCompositionEvent& event) MOZ_OVERRIDE;
    virtual bool RecvTextEvent(const mozilla::WidgetTextEvent& event) MOZ_OVERRIDE;
    virtual bool RecvSelectionEvent(const mozilla::WidgetSelectionEvent& event) MOZ_OVERRIDE;
    virtual bool RecvActivateFrameEvent(const nsString& aType, const bool& capture) MOZ_OVERRIDE;
    virtual bool RecvLoadRemoteScript(const nsString& aURL,
                                      const bool& aRunInGlobalScope) MOZ_OVERRIDE;
    virtual bool RecvAsyncMessage(const nsString& aMessage,
                                  const ClonedMessageData& aData,
                                  const InfallibleTArray<CpowEntry>& aCpows,
                                  const IPC::Principal& aPrincipal) MOZ_OVERRIDE;

    virtual PDocumentRendererChild*
    AllocPDocumentRendererChild(const nsRect& documentRect, const gfx::Matrix& transform,
                                const nsString& bgcolor,
                                const uint32_t& renderFlags, const bool& flushLayout,
                                const nsIntSize& renderSize) MOZ_OVERRIDE;
    virtual bool DeallocPDocumentRendererChild(PDocumentRendererChild* actor) MOZ_OVERRIDE;
    virtual bool RecvPDocumentRendererConstructor(PDocumentRendererChild* actor,
                                                  const nsRect& documentRect,
                                                  const gfx::Matrix& transform,
                                                  const nsString& bgcolor,
                                                  const uint32_t& renderFlags,
                                                  const bool& flushLayout,
                                                  const nsIntSize& renderSize) MOZ_OVERRIDE;

    virtual PColorPickerChild*
    AllocPColorPickerChild(const nsString& title, const nsString& initialColor) MOZ_OVERRIDE;
    virtual bool DeallocPColorPickerChild(PColorPickerChild* actor) MOZ_OVERRIDE;

#ifdef DEBUG
    virtual PContentPermissionRequestChild*
    SendPContentPermissionRequestConstructor(PContentPermissionRequestChild* aActor,
                                             const InfallibleTArray<PermissionRequest>& aRequests,
                                             const IPC::Principal& aPrincipal);
#endif 

    virtual PContentPermissionRequestChild*
    AllocPContentPermissionRequestChild(const InfallibleTArray<PermissionRequest>& aRequests,
                                        const IPC::Principal& aPrincipal) MOZ_OVERRIDE;
    virtual bool
    DeallocPContentPermissionRequestChild(PContentPermissionRequestChild* actor) MOZ_OVERRIDE;

    virtual PFilePickerChild*
    AllocPFilePickerChild(const nsString& aTitle, const int16_t& aMode) MOZ_OVERRIDE;
    virtual bool
    DeallocPFilePickerChild(PFilePickerChild* actor) MOZ_OVERRIDE;

    virtual POfflineCacheUpdateChild* AllocPOfflineCacheUpdateChild(
            const URIParams& manifestURI,
            const URIParams& documentURI,
            const bool& stickDocument) MOZ_OVERRIDE;
    virtual bool
    DeallocPOfflineCacheUpdateChild(POfflineCacheUpdateChild* offlineCacheUpdate) MOZ_OVERRIDE;

    nsIWebNavigation* WebNavigation() { return mWebNav; }

    nsIPrincipal* GetPrincipal() { return mPrincipal; }

    
    void GetDPI(float* aDPI);
    void GetDefaultScale(double *aScale);

    ScreenOrientation GetOrientation() { return mOrientation; }

    void SetBackgroundColor(const nscolor& aColor);

    void NotifyPainted();

    bool IsAsyncPanZoomEnabled();

    


    bool DispatchMouseEvent(const nsString&       aType,
                            const CSSPoint&       aPoint,
                            const int32_t&        aButton,
                            const int32_t&        aClickCount,
                            const int32_t&        aModifiers,
                            const bool&           aIgnoreRootScrollFrame,
                            const unsigned short& aInputSourceArg);

    




    void MakeVisible();
    void MakeHidden();

    
    
    bool GetCachedFileDescriptor(const nsAString& aPath,
                                 nsICachedFileDescriptorListener* aCallback);

    void CancelCachedFileDescriptorCallback(
                                    const nsAString& aPath,
                                    nsICachedFileDescriptorListener* aCallback);

    ContentChild* Manager() { return mManager; }

    bool GetUpdateHitRegion() { return mUpdateHitRegion; }

    void UpdateHitRegion(const nsRegion& aRegion);

    static inline TabChild*
    GetFrom(nsIDocShell* aDocShell)
    {
      nsCOMPtr<nsITabChild> tc = do_GetInterface(aDocShell);
      return static_cast<TabChild*>(tc.get());
    }

    static TabChild* GetFrom(nsIPresShell* aPresShell);

    static inline TabChild*
    GetFrom(nsIDOMWindow* aWindow)
    {
      nsCOMPtr<nsIWebNavigation> webNav = do_GetInterface(aWindow);
      nsCOMPtr<nsIDocShell> docShell = do_QueryInterface(webNav);
      return GetFrom(docShell);
    }

protected:
    virtual PRenderFrameChild* AllocPRenderFrameChild() MOZ_OVERRIDE;
    virtual bool DeallocPRenderFrameChild(PRenderFrameChild* aFrame) MOZ_OVERRIDE;
    virtual bool RecvDestroy() MOZ_OVERRIDE;
    virtual bool RecvSetUpdateHitRegion(const bool& aEnabled) MOZ_OVERRIDE;
    virtual bool RecvSetIsDocShellActive(const bool& aIsActive) MOZ_OVERRIDE;

    nsEventStatus DispatchWidgetEvent(WidgetGUIEvent& event);

    virtual PIndexedDBChild* AllocPIndexedDBChild(const nsCString& aGroup,
                                                  const nsCString& aASCIIOrigin,
                                                  bool* ) MOZ_OVERRIDE;

    virtual bool DeallocPIndexedDBChild(PIndexedDBChild* aActor) MOZ_OVERRIDE;

private:
    







    TabChild(ContentChild* aManager, const TabContext& aContext, uint32_t aChromeFlags);

    nsresult Init();

    void InitializeRootMetrics();
    bool HasValidInnerSize();

    
    
    
    
    
    void NotifyTabContextUpdated();

    bool UseDirectCompositor();

    void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;

    enum FrameScriptLoading { DONT_LOAD_SCRIPTS, DEFAULT_LOAD_SCRIPTS };
    bool InitTabChildGlobal(FrameScriptLoading aScriptLoading = DEFAULT_LOAD_SCRIPTS);
    bool InitRenderingState();
    void DestroyWindow();
    void SetProcessNameToAppName();
    bool ProcessUpdateFrame(const mozilla::layers::FrameMetrics& aFrameMetrics);

    
    void DoFakeShow();

    
    
    void SetCSSViewport(const CSSSize& aSize);

    
    
    
    
    
    void HandlePossibleViewportChange();

    
    
    
    
    
    void DispatchMessageManagerMessage(const nsAString& aMessageName,
                                       const nsACString& aJSONData);

    void DispatchSynthesizedMouseEvent(uint32_t aMsg, uint64_t aTime,
                                       const LayoutDevicePoint& aRefPoint);

    
    
    
    
    
    
    void FireContextMenuEvent();
    void CancelTapTracking();
    void UpdateTapState(const WidgetTouchEvent& aEvent, nsEventStatus aStatus);

    nsresult
    BrowserFrameProvideWindow(nsIDOMWindow* aOpener,
                              nsIURI* aURI,
                              const nsAString& aName,
                              const nsACString& aFeatures,
                              bool* aWindowIsNew,
                              nsIDOMWindow** aReturn);

    
    already_AddRefed<nsIDOMWindowUtils> GetDOMWindowUtils();
    
    already_AddRefed<nsIDocument> GetDocument();

    class CachedFileDescriptorInfo;
    class CachedFileDescriptorCallbackRunnable;

    TextureFactoryIdentifier mTextureFactoryIdentifier;
    nsCOMPtr<nsIWebNavigation> mWebNav;
    nsCOMPtr<nsIWidget> mWidget;
    nsCOMPtr<nsIURI> mLastURI;
    FrameMetrics mLastRootMetrics;
    RenderFrameChild* mRemoteFrame;
    nsRefPtr<ContentChild> mManager;
    nsRefPtr<TabChildGlobal> mTabChildGlobal;
    uint32_t mChromeFlags;
    nsIntRect mOuterRect;
    ScreenIntSize mInnerSize;
    
    
    LayoutDevicePoint mGestureDownPoint;
    
    int32_t mActivePointerId;
    
    
    
    CancelableTask* mTapHoldTimer;
    
    bool mAppPackageFileDescriptorRecved;
    
    nsAutoTArray<nsAutoPtr<CachedFileDescriptorInfo>, 1>
        mCachedFileDescriptorInfos;
    float mOldViewportWidth;
    nscolor mLastBackgroundColor;
    ScrollingBehavior mScrolling;
    bool mDidFakeShow;
    bool mNotified;
    bool mContentDocumentIsDisplayed;
    bool mTriedBrowserInit;
    ScreenOrientation mOrientation;
    bool mUpdateHitRegion;
    bool mContextMenuHandled;
    bool mWaitingTouchListeners;
    void FireSingleTapEvent(LayoutDevicePoint aPoint);

    DISALLOW_EVIL_CONSTRUCTORS(TabChild);
};

}
}

#endif 
