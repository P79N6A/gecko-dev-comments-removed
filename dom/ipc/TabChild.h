





#ifndef mozilla_dom_TabChild_h
#define mozilla_dom_TabChild_h

#include "mozilla/dom/PBrowserChild.h"
#include "nsIWebNavigation.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
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
#include "nsIPresShell.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsWeakReference.h"
#include "nsITabChild.h"
#include "nsITooltipListener.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/TabContext.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/EventDispatcher.h"
#include "mozilla/EventForwards.h"
#include "mozilla/layers/CompositorTypes.h"
#include "nsIWebBrowserChrome3.h"
#include "mozilla/dom/ipc/IdType.h"

class nsICachedFileDescriptorListener;
class nsIDOMWindowUtils;

namespace mozilla {
namespace layout {
class RenderFrameChild;
}

namespace layers {
class APZEventState;
struct SetTargetAPZCCallback;
struct SetAllowedTouchBehaviorCallback;
}

namespace widget {
struct AutoCacheNativeKeyCommands;
}

namespace plugins {
class PluginWidgetChild;
}

namespace dom {

class TabChild;
class ClonedMessageData;
class TabChildBase;

class TabChildGlobal : public DOMEventTargetHelper,
                       public nsIContentFrameMessageManager,
                       public nsIScriptObjectPrincipal,
                       public nsIGlobalObject,
                       public nsSupportsWeakReference
{
public:
  explicit TabChildGlobal(TabChildBase* aTabChild);
  void Init();
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(TabChildGlobal, DOMEventTargetHelper)
  NS_FORWARD_SAFE_NSIMESSAGELISTENERMANAGER(mMessageManager)
  NS_FORWARD_SAFE_NSIMESSAGESENDER(mMessageManager)
  NS_FORWARD_SAFE_NSIMESSAGEMANAGERGLOBAL(mMessageManager)
  NS_IMETHOD SendSyncMessage(const nsAString& aMessageName,
                             JS::Handle<JS::Value> aObject,
                             JS::Handle<JS::Value> aRemote,
                             nsIPrincipal* aPrincipal,
                             JSContext* aCx,
                             uint8_t aArgc,
                             JS::MutableHandle<JS::Value> aRetval) override
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
                            JS::MutableHandle<JS::Value> aRetval) override
  {
    return mMessageManager
      ? mMessageManager->SendRpcMessage(aMessageName, aObject, aRemote,
                                        aPrincipal, aCx, aArgc, aRetval)
      : NS_ERROR_NULL_POINTER;
  }
  NS_IMETHOD GetContent(nsIDOMWindow** aContent) override;
  NS_IMETHOD GetDocShell(nsIDocShell** aDocShell) override;

  nsresult AddEventListener(const nsAString& aType,
                            nsIDOMEventListener* aListener,
                            bool aUseCapture)
  {
    
    return DOMEventTargetHelper::AddEventListener(aType, aListener,
                                                  aUseCapture, false, 2);
  }
  using DOMEventTargetHelper::AddEventListener;
  NS_IMETHOD AddEventListener(const nsAString& aType,
                              nsIDOMEventListener* aListener,
                              bool aUseCapture, bool aWantsUntrusted,
                              uint8_t optional_argc) override
  {
    return DOMEventTargetHelper::AddEventListener(aType, aListener,
                                                  aUseCapture,
                                                  aWantsUntrusted,
                                                  optional_argc);
  }

  nsresult
  PreHandleEvent(EventChainPreVisitor& aVisitor) override
  {
    aVisitor.mForceContentDispatch = true;
    return NS_OK;
  }

  virtual JSContext* GetJSContextForEventHandlers() override;
  virtual nsIPrincipal* GetPrincipal() override;
  virtual JSObject* GetGlobalJSObject() override;

  virtual JSObject* WrapObject(JSContext* cx, JS::Handle<JSObject*> aGivenProto) override
  {
    MOZ_CRASH("TabChildGlobal doesn't use DOM bindings!");
  }

  nsCOMPtr<nsIContentFrameMessageManager> mMessageManager;
  nsRefPtr<TabChildBase> mTabChild;

protected:
  ~TabChildGlobal();
};

class ContentListener final : public nsIDOMEventListener
{
public:
  explicit ContentListener(TabChild* aTabChild) : mTabChild(aTabChild) {}
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMEVENTLISTENER
protected:
  ~ContentListener() {}
  TabChild* mTabChild;
};





class TabChildBase : public nsISupports,
                     public nsMessageManagerScriptExecutor,
                     public ipc::MessageManagerCallback
{
public:
    TabChildBase();

    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(TabChildBase)

    virtual nsIWebNavigation* WebNavigation() const = 0;
    virtual nsIWidget* WebWidget() = 0;
    nsIPrincipal* GetPrincipal() { return mPrincipal; }
    
    
    
    
    
    bool HandlePossibleViewportChange(const ScreenIntSize& aOldScreenSize);
    virtual bool DoUpdateZoomConstraints(const uint32_t& aPresShellId,
                                         const mozilla::layers::FrameMetrics::ViewID& aViewId,
                                         const bool& aIsRoot,
                                         const mozilla::layers::ZoomConstraints& aConstraints) = 0;

protected:
    virtual ~TabChildBase();
    CSSSize GetPageSize(nsCOMPtr<nsIDocument> aDocument, const CSSSize& aViewport);

    
    already_AddRefed<nsIDOMWindowUtils> GetDOMWindowUtils();
    
    already_AddRefed<nsIDocument> GetDocument() const;
    
    already_AddRefed<nsIPresShell> GetPresShell() const;

    
    
    void SetCSSViewport(const CSSSize& aSize);

    
    
    
    
    
    void DispatchMessageManagerMessage(const nsAString& aMessageName,
                                       const nsAString& aJSONData);

    void InitializeRootMetrics();

    mozilla::layers::FrameMetrics ProcessUpdateFrame(const mozilla::layers::FrameMetrics& aFrameMetrics);

    bool UpdateFrameHandler(const mozilla::layers::FrameMetrics& aFrameMetrics);

protected:
    CSSSize mOldViewportSize;
    bool mContentDocumentIsDisplayed;
    nsRefPtr<TabChildGlobal> mTabChildGlobal;
    ScreenIntSize mInnerSize;
    mozilla::layers::FrameMetrics mLastRootMetrics;
    nsCOMPtr<nsIWebBrowserChrome3> mWebBrowserChrome;
};

class TabChild final : public TabChildBase,
                       public PBrowserChild,
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
                       public TabContext,
                       public nsITooltipListener
{
    typedef mozilla::dom::ClonedMessageData ClonedMessageData;
    typedef mozilla::layout::RenderFrameChild RenderFrameChild;
    typedef mozilla::layers::APZEventState APZEventState;
    typedef mozilla::layers::SetTargetAPZCCallback SetTargetAPZCCallback;
    typedef mozilla::layers::SetAllowedTouchBehaviorCallback SetAllowedTouchBehaviorCallback;

public:
    



    static already_AddRefed<TabChild> FindTabChild(const TabId& aTabId);

public:
    




    static void PreloadSlowThings();
    static void PostForkPreload();

    
    static already_AddRefed<TabChild>
    Create(nsIContentChild* aManager, const TabId& aTabId, const TabContext& aContext, uint32_t aChromeFlags);

    bool IsRootContentDocument();
    
    bool IsDestroyed() { return mDestroyed; }
    const TabId GetTabId() const {
      MOZ_ASSERT(mUniqueId != 0);
      return mUniqueId;
    }

    NS_DECL_ISUPPORTS_INHERITED
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
                                       bool aIsSync) override;
    virtual bool DoSendAsyncMessage(JSContext* aCx,
                                    const nsAString& aMessage,
                                    const mozilla::dom::StructuredCloneData& aData,
                                    JS::Handle<JSObject *> aCpows,
                                    nsIPrincipal* aPrincipal) override;
    virtual bool DoUpdateZoomConstraints(const uint32_t& aPresShellId,
                                         const ViewID& aViewId,
                                         const bool& aIsRoot,
                                         const ZoomConstraints& aConstraints) override;
    virtual bool RecvLoadURL(const nsCString& aURI,
                             const BrowserConfiguration& aConfiguration) override;
    virtual bool RecvCacheFileDescriptor(const nsString& aPath,
                                         const FileDescriptor& aFileDescriptor)
                                         override;
    virtual bool RecvShow(const ScreenIntSize& aSize,
                          const ShowInfo& aInfo,
                          const TextureFactoryIdentifier& aTextureFactoryIdentifier,
                          const uint64_t& aLayersId,
                          PRenderFrameChild* aRenderFrame,
                          const bool& aParentIsActive) override;
    virtual bool RecvUpdateDimensions(const nsIntRect& rect,
                                      const ScreenIntSize& size,
                                      const ScreenOrientation& orientation,
                                      const LayoutDeviceIntPoint& chromeDisp) override;
    virtual bool RecvUpdateFrame(const layers::FrameMetrics& aFrameMetrics) override;
    virtual bool RecvRequestFlingSnap(const ViewID& aScrollId,
                                      const CSSPoint& aDestination) override;
    virtual bool RecvAcknowledgeScrollUpdate(const ViewID& aScrollId,
                                             const uint32_t& aScrollGeneration) override;
    virtual bool RecvHandleDoubleTap(const CSSPoint& aPoint,
                                     const Modifiers& aModifiers,
                                     const mozilla::layers::ScrollableLayerGuid& aGuid) override;
    virtual bool RecvHandleSingleTap(const CSSPoint& aPoint,
                                     const Modifiers& aModifiers,
                                     const mozilla::layers::ScrollableLayerGuid& aGuid) override;
    virtual bool RecvHandleLongTap(const CSSPoint& aPoint,
                                   const Modifiers& aModifiers,
                                   const mozilla::layers::ScrollableLayerGuid& aGuid,
                                   const uint64_t& aInputBlockId) override;
    virtual bool RecvNotifyAPZStateChange(const ViewID& aViewId,
                                          const APZStateChange& aChange,
                                          const int& aArg) override;
    virtual bool RecvActivate() override;
    virtual bool RecvDeactivate() override;
    virtual bool RecvMouseEvent(const nsString& aType,
                                const float&    aX,
                                const float&    aY,
                                const int32_t&  aButton,
                                const int32_t&  aClickCount,
                                const int32_t&  aModifiers,
                                const bool&     aIgnoreRootScrollFrame) override;
    virtual bool RecvRealMouseMoveEvent(const mozilla::WidgetMouseEvent& event) override;
    virtual bool RecvRealMouseButtonEvent(const mozilla::WidgetMouseEvent& event) override;
    virtual bool RecvRealDragEvent(const WidgetDragEvent& aEvent,
                                   const uint32_t& aDragAction,
                                   const uint32_t& aDropEffect) override;
    virtual bool RecvRealKeyEvent(const mozilla::WidgetKeyboardEvent& event,
                                  const MaybeNativeKeyBinding& aBindings) override;
    virtual bool RecvMouseWheelEvent(const mozilla::WidgetWheelEvent& event,
                                     const ScrollableLayerGuid& aGuid,
                                     const uint64_t& aInputBlockId) override;
    virtual bool RecvRealTouchEvent(const WidgetTouchEvent& aEvent,
                                    const ScrollableLayerGuid& aGuid,
                                    const uint64_t& aInputBlockId) override;
    virtual bool RecvRealTouchMoveEvent(const WidgetTouchEvent& aEvent,
                                        const ScrollableLayerGuid& aGuid,
                                        const uint64_t& aInputBlockId) override;
    virtual bool RecvKeyEvent(const nsString& aType,
                              const int32_t&  aKeyCode,
                              const int32_t&  aCharCode,
                              const int32_t&  aModifiers,
                              const bool&     aPreventDefault) override;
    virtual bool RecvMouseScrollTestEvent(const FrameMetrics::ViewID& aScrollId,
                                          const nsString& aEvent) override;
    virtual bool RecvNativeSynthesisResponse(const uint64_t& aObserverId,
                                             const nsCString& aResponse) override;
    virtual bool RecvCompositionEvent(const mozilla::WidgetCompositionEvent& event) override;
    virtual bool RecvSelectionEvent(const mozilla::WidgetSelectionEvent& event) override;
    virtual bool RecvActivateFrameEvent(const nsString& aType, const bool& capture) override;
    virtual bool RecvLoadRemoteScript(const nsString& aURL,
                                      const bool& aRunInGlobalScope) override;
    virtual bool RecvAsyncMessage(const nsString& aMessage,
                                  const ClonedMessageData& aData,
                                  InfallibleTArray<CpowEntry>&& aCpows,
                                  const IPC::Principal& aPrincipal) override;

    virtual bool RecvAppOfflineStatus(const uint32_t& aId, const bool& aOffline) override;

    virtual PDocumentRendererChild*
    AllocPDocumentRendererChild(const nsRect& documentRect, const gfx::Matrix& transform,
                                const nsString& bgcolor,
                                const uint32_t& renderFlags, const bool& flushLayout,
                                const nsIntSize& renderSize) override;
    virtual bool DeallocPDocumentRendererChild(PDocumentRendererChild* actor) override;
    virtual bool RecvPDocumentRendererConstructor(PDocumentRendererChild* actor,
                                                  const nsRect& documentRect,
                                                  const gfx::Matrix& transform,
                                                  const nsString& bgcolor,
                                                  const uint32_t& renderFlags,
                                                  const bool& flushLayout,
                                                  const nsIntSize& renderSize) override;

    virtual PColorPickerChild*
    AllocPColorPickerChild(const nsString& title, const nsString& initialColor) override;
    virtual bool DeallocPColorPickerChild(PColorPickerChild* actor) override;

    virtual PFilePickerChild*
    AllocPFilePickerChild(const nsString& aTitle, const int16_t& aMode) override;
    virtual bool
    DeallocPFilePickerChild(PFilePickerChild* actor) override;

    virtual PIndexedDBPermissionRequestChild*
    AllocPIndexedDBPermissionRequestChild(const Principal& aPrincipal)
                                          override;

    virtual bool
    DeallocPIndexedDBPermissionRequestChild(
                                       PIndexedDBPermissionRequestChild* aActor)
                                       override;

    virtual nsIWebNavigation* WebNavigation() const override { return mWebNav; }
    virtual nsIWidget* WebWidget() override { return mWidget; }

    
    void GetDPI(float* aDPI);
    void GetDefaultScale(double *aScale);

    ScreenOrientation GetOrientation() { return mOrientation; }

    void SetBackgroundColor(const nscolor& aColor);

    void NotifyPainted();

    void RequestNativeKeyBindings(mozilla::widget::AutoCacheNativeKeyCommands* aAutoCache,
                                  WidgetKeyboardEvent* aEvent);

    




    void MakeVisible();
    void MakeHidden();

    
    
    bool GetCachedFileDescriptor(const nsAString& aPath,
                                 nsICachedFileDescriptorListener* aCallback);

    void CancelCachedFileDescriptorCallback(
                                    const nsAString& aPath,
                                    nsICachedFileDescriptorListener* aCallback);

    nsIContentChild* Manager() { return mManager; }

    bool GetUpdateHitRegion() { return mUpdateHitRegion; }

    void UpdateHitRegion(const nsRegion& aRegion);

    static inline TabChild*
    GetFrom(nsIDocShell* aDocShell)
    {
      nsCOMPtr<nsITabChild> tc = do_GetInterface(aDocShell);
      return static_cast<TabChild*>(tc.get());
    }

    static TabChild* GetFrom(nsIPresShell* aPresShell);
    static TabChild* GetFrom(uint64_t aLayersId);

    void DidComposite(uint64_t aTransactionId);

    static inline TabChild*
    GetFrom(nsIDOMWindow* aWindow)
    {
      nsCOMPtr<nsIWebNavigation> webNav = do_GetInterface(aWindow);
      nsCOMPtr<nsIDocShell> docShell = do_QueryInterface(webNav);
      return GetFrom(docShell);
    }

    virtual bool RecvUIResolutionChanged() override;

    virtual bool RecvThemeChanged(nsTArray<LookAndFeelInt>&& aLookAndFeelIntCache) override;

    


    PPluginWidgetChild* AllocPPluginWidgetChild() override;
    bool DeallocPPluginWidgetChild(PPluginWidgetChild* aActor) override;
    nsresult CreatePluginWidget(nsIWidget* aParent, nsIWidget** aOut);

    LayoutDeviceIntPoint GetChromeDisplacement() { return mChromeDisp; };

    bool IPCOpen() { return mIPCOpen; }

    bool ParentIsActive()
    {
      return mParentIsActive;
    }

protected:
    virtual ~TabChild();

    virtual PRenderFrameChild* AllocPRenderFrameChild() override;
    virtual bool DeallocPRenderFrameChild(PRenderFrameChild* aFrame) override;
    virtual bool RecvDestroy() override;
    virtual bool RecvSetUpdateHitRegion(const bool& aEnabled) override;
    virtual bool RecvSetIsDocShellActive(const bool& aIsActive) override;

    virtual bool RecvRequestNotifyAfterRemotePaint() override;

    virtual bool RecvParentActivated(const bool& aActivated) override;

#ifdef MOZ_WIDGET_GONK
    void MaybeRequestPreinitCamera();
#endif

private:
    







    TabChild(nsIContentChild* aManager,
             const TabId& aTabId,
             const TabContext& aContext,
             uint32_t aChromeFlags);

    nsresult Init();

    class DelayedFireContextMenuEvent;

    
    
    
    
    
    void NotifyTabContextUpdated();

    void ActorDestroy(ActorDestroyReason why) override;

    enum FrameScriptLoading { DONT_LOAD_SCRIPTS, DEFAULT_LOAD_SCRIPTS };
    bool InitTabChildGlobal(FrameScriptLoading aScriptLoading = DEFAULT_LOAD_SCRIPTS);
    bool InitRenderingState(const TextureFactoryIdentifier& aTextureFactoryIdentifier,
                            const uint64_t& aLayersId,
                            PRenderFrameChild* aRenderFrame);
    void DestroyWindow();
    void SetProcessNameToAppName();

    
    void DoFakeShow(const TextureFactoryIdentifier& aTextureFactoryIdentifier,
                    const uint64_t& aLayersId,
                    PRenderFrameChild* aRenderFrame);

    void ApplyShowInfo(const ShowInfo& aInfo);

    
    
    
    
    
    
    void FireContextMenuEvent();
    void CancelTapTracking();
    void UpdateTapState(const WidgetTouchEvent& aEvent, nsEventStatus aStatus);

    nsresult
    ProvideWindowCommon(nsIDOMWindow* aOpener,
                        bool aIframeMoz,
                        uint32_t aChromeFlags,
                        bool aCalledFromJS,
                        bool aPositionSpecified,
                        bool aSizeSpecified,
                        nsIURI* aURI,
                        const nsAString& aName,
                        const nsACString& aFeatures,
                        bool* aWindowIsNew,
                        nsIDOMWindow** aReturn);

    bool HasValidInnerSize();

    
    float GetPresShellResolution() const;

    void SetTabId(const TabId& aTabId);

    class CachedFileDescriptorInfo;
    class CachedFileDescriptorCallbackRunnable;
    class DelayedDeleteRunnable;

    TextureFactoryIdentifier mTextureFactoryIdentifier;
    nsCOMPtr<nsIWebNavigation> mWebNav;
    nsCOMPtr<nsIWidget> mWidget;
    nsCOMPtr<nsIURI> mLastURI;
    RenderFrameChild* mRemoteFrame;
    nsRefPtr<nsIContentChild> mManager;
    uint32_t mChromeFlags;
    uint64_t mLayersId;
    nsIntRect mOuterRect;
    
    
    LayoutDevicePoint mGestureDownPoint;
    
    int32_t mActivePointerId;
    
    
    
    nsCOMPtr<nsITimer> mTapHoldTimer;
    
    bool mAppPackageFileDescriptorRecved;
    
    nsAutoTArray<nsAutoPtr<CachedFileDescriptorInfo>, 1>
        mCachedFileDescriptorInfos;
    nscolor mLastBackgroundColor;
    bool mDidFakeShow;
    bool mNotified;
    bool mTriedBrowserInit;
    ScreenOrientation mOrientation;
    bool mUpdateHitRegion;

    bool mIgnoreKeyPressEvent;
    nsRefPtr<APZEventState> mAPZEventState;
    nsRefPtr<SetAllowedTouchBehaviorCallback> mSetAllowedTouchBehaviorCallback;
    bool mHasValidInnerSize;
    bool mDestroyed;
    
    LayoutDeviceIntPoint mChromeDisp;
    TabId mUniqueId;
    float mDPI;
    double mDefaultScale;
    bool mIPCOpen;
    bool mParentIsActive;

    DISALLOW_EVIL_CONSTRUCTORS(TabChild);
};

}
}

#endif 
