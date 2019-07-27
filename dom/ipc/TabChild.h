





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
  NS_IMETHOD SendSyncMessage(const nsAString& aMessageName,
                             JS::Handle<JS::Value> aObject,
                             JS::Handle<JS::Value> aRemote,
                             nsIPrincipal* aPrincipal,
                             JSContext* aCx,
                             uint8_t aArgc,
                             JS::MutableHandle<JS::Value> aRetval) MOZ_OVERRIDE
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
                            JS::MutableHandle<JS::Value> aRetval) MOZ_OVERRIDE
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
                              uint8_t optional_argc) MOZ_OVERRIDE
  {
    return DOMEventTargetHelper::AddEventListener(aType, aListener,
                                                  aUseCapture,
                                                  aWantsUntrusted,
                                                  optional_argc);
  }

  nsresult
  PreHandleEvent(EventChainPreVisitor& aVisitor) MOZ_OVERRIDE
  {
    aVisitor.mForceContentDispatch = true;
    return NS_OK;
  }

  virtual JSContext* GetJSContextForEventHandlers() MOZ_OVERRIDE;
  virtual nsIPrincipal* GetPrincipal() MOZ_OVERRIDE;
  virtual JSObject* GetGlobalJSObject() MOZ_OVERRIDE;

  virtual JSObject* WrapObject(JSContext* cx) MOZ_OVERRIDE
  {
    MOZ_CRASH("TabChildGlobal doesn't use DOM bindings!");
  }

  nsCOMPtr<nsIContentFrameMessageManager> mMessageManager;
  nsRefPtr<TabChildBase> mTabChild;

protected:
  ~TabChildGlobal();
};

class ContentListener MOZ_FINAL : public nsIDOMEventListener
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
                     public nsFrameScriptExecutor,
                     public ipc::MessageManagerCallback
{
public:
    TabChildBase();

    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(TabChildBase)

    virtual nsIWebNavigation* WebNavigation() const = 0;
    virtual nsIWidget* WebWidget() = 0;
    nsIPrincipal* GetPrincipal() { return mPrincipal; }
    bool IsAsyncPanZoomEnabled();
    
    
    
    
    
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
    mozilla::layout::ScrollingBehavior mScrolling;
    nsCOMPtr<nsIWebBrowserChrome3> mWebBrowserChrome;
};

class TabChild MOZ_FINAL : public TabChildBase,
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
    typedef mozilla::layout::ScrollingBehavior ScrollingBehavior;
    typedef mozilla::layers::APZEventState APZEventState;
    typedef mozilla::layers::SetTargetAPZCCallback SetTargetAPZCCallback;

public:
    



    static already_AddRefed<TabChild> FindTabChild(const TabId& aTabId);

public:
    




    static void PreloadSlowThings();
    static void PostForkPreload();

    
    static already_AddRefed<TabChild>
    Create(nsIContentChild* aManager, const TabId& aTabId, const TabContext& aContext, uint32_t aChromeFlags);

    bool IsRootContentDocument();

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
                                       bool aIsSync) MOZ_OVERRIDE;
    virtual bool DoSendAsyncMessage(JSContext* aCx,
                                    const nsAString& aMessage,
                                    const mozilla::dom::StructuredCloneData& aData,
                                    JS::Handle<JSObject *> aCpows,
                                    nsIPrincipal* aPrincipal) MOZ_OVERRIDE;
    virtual bool DoUpdateZoomConstraints(const uint32_t& aPresShellId,
                                         const ViewID& aViewId,
                                         const bool& aIsRoot,
                                         const ZoomConstraints& aConstraints) MOZ_OVERRIDE;
    virtual bool RecvLoadURL(const nsCString& aURI,
                             const BrowserConfiguration& aConfiguration) MOZ_OVERRIDE;
    virtual bool RecvCacheFileDescriptor(const nsString& aPath,
                                         const FileDescriptor& aFileDescriptor)
                                         MOZ_OVERRIDE;
    virtual bool RecvShow(const nsIntSize& aSize,
                          const ShowInfo& aInfo,
                          const ScrollingBehavior& aScrolling,
                          const TextureFactoryIdentifier& aTextureFactoryIdentifier,
                          const uint64_t& aLayersId,
                          PRenderFrameChild* aRenderFrame,
                          const bool& aParentIsActive) MOZ_OVERRIDE;
    virtual bool RecvUpdateDimensions(const nsIntRect& rect,
                                      const nsIntSize& size,
                                      const ScreenOrientation& orientation,
                                      const nsIntPoint& chromeDisp) MOZ_OVERRIDE;
    virtual bool RecvUpdateFrame(const mozilla::layers::FrameMetrics& aFrameMetrics) MOZ_OVERRIDE;
    virtual bool RecvAcknowledgeScrollUpdate(const ViewID& aScrollId,
                                             const uint32_t& aScrollGeneration) MOZ_OVERRIDE;
    virtual bool RecvHandleDoubleTap(const CSSPoint& aPoint,
                                     const mozilla::layers::ScrollableLayerGuid& aGuid) MOZ_OVERRIDE;
    virtual bool RecvHandleSingleTap(const CSSPoint& aPoint,
                                     const mozilla::layers::ScrollableLayerGuid& aGuid) MOZ_OVERRIDE;
    virtual bool RecvHandleLongTap(const CSSPoint& aPoint,
                                   const mozilla::layers::ScrollableLayerGuid& aGuid,
                                   const uint64_t& aInputBlockId) MOZ_OVERRIDE;
    virtual bool RecvHandleLongTapUp(const CSSPoint& aPoint,
                                     const mozilla::layers::ScrollableLayerGuid& aGuid) MOZ_OVERRIDE;
    virtual bool RecvNotifyAPZStateChange(const ViewID& aViewId,
                                          const APZStateChange& aChange,
                                          const int& aArg) MOZ_OVERRIDE;
    virtual bool RecvActivate() MOZ_OVERRIDE;
    virtual bool RecvDeactivate() MOZ_OVERRIDE;
    virtual bool RecvMouseEvent(const nsString& aType,
                                const float&    aX,
                                const float&    aY,
                                const int32_t&  aButton,
                                const int32_t&  aClickCount,
                                const int32_t&  aModifiers,
                                const bool&     aIgnoreRootScrollFrame) MOZ_OVERRIDE;
    virtual bool RecvRealMouseMoveEvent(const mozilla::WidgetMouseEvent& event) MOZ_OVERRIDE;
    virtual bool RecvRealMouseButtonEvent(const mozilla::WidgetMouseEvent& event) MOZ_OVERRIDE;
    virtual bool RecvRealKeyEvent(const mozilla::WidgetKeyboardEvent& event,
                                  const MaybeNativeKeyBinding& aBindings) MOZ_OVERRIDE;
    virtual bool RecvMouseWheelEvent(const mozilla::WidgetWheelEvent& event,
                                     const ScrollableLayerGuid& aGuid,
                                     const uint64_t& aInputBlockId) MOZ_OVERRIDE;
    virtual bool RecvRealTouchEvent(const WidgetTouchEvent& aEvent,
                                    const ScrollableLayerGuid& aGuid,
                                    const uint64_t& aInputBlockId) MOZ_OVERRIDE;
    virtual bool RecvRealTouchMoveEvent(const WidgetTouchEvent& aEvent,
                                        const ScrollableLayerGuid& aGuid,
                                        const uint64_t& aInputBlockId) MOZ_OVERRIDE;
    virtual bool RecvKeyEvent(const nsString& aType,
                              const int32_t&  aKeyCode,
                              const int32_t&  aCharCode,
                              const int32_t&  aModifiers,
                              const bool&     aPreventDefault) MOZ_OVERRIDE;
    virtual bool RecvCompositionEvent(const mozilla::WidgetCompositionEvent& event) MOZ_OVERRIDE;
    virtual bool RecvSelectionEvent(const mozilla::WidgetSelectionEvent& event) MOZ_OVERRIDE;
    virtual bool RecvActivateFrameEvent(const nsString& aType, const bool& capture) MOZ_OVERRIDE;
    virtual bool RecvLoadRemoteScript(const nsString& aURL,
                                      const bool& aRunInGlobalScope) MOZ_OVERRIDE;
    virtual bool RecvAsyncMessage(const nsString& aMessage,
                                  const ClonedMessageData& aData,
                                  InfallibleTArray<CpowEntry>&& aCpows,
                                  const IPC::Principal& aPrincipal) MOZ_OVERRIDE;

    virtual bool RecvAppOfflineStatus(const uint32_t& aId, const bool& aOffline) MOZ_OVERRIDE;

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

    virtual PContentPermissionRequestChild*
    AllocPContentPermissionRequestChild(const InfallibleTArray<PermissionRequest>& aRequests,
                                        const IPC::Principal& aPrincipal) MOZ_OVERRIDE;
    virtual bool
    DeallocPContentPermissionRequestChild(PContentPermissionRequestChild* actor) MOZ_OVERRIDE;

    virtual PFilePickerChild*
    AllocPFilePickerChild(const nsString& aTitle, const int16_t& aMode) MOZ_OVERRIDE;
    virtual bool
    DeallocPFilePickerChild(PFilePickerChild* actor) MOZ_OVERRIDE;

    virtual PIndexedDBPermissionRequestChild*
    AllocPIndexedDBPermissionRequestChild(const Principal& aPrincipal)
                                          MOZ_OVERRIDE;

    virtual bool
    DeallocPIndexedDBPermissionRequestChild(
                                       PIndexedDBPermissionRequestChild* aActor)
                                       MOZ_OVERRIDE;

    virtual nsIWebNavigation* WebNavigation() const MOZ_OVERRIDE { return mWebNav; }
    virtual nsIWidget* WebWidget() MOZ_OVERRIDE { return mWidget; }

    
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

    virtual bool RecvUIResolutionChanged() MOZ_OVERRIDE;

    


    PPluginWidgetChild* AllocPPluginWidgetChild() MOZ_OVERRIDE;
    bool DeallocPPluginWidgetChild(PPluginWidgetChild* aActor) MOZ_OVERRIDE;
    nsresult CreatePluginWidget(nsIWidget* aParent, nsIWidget** aOut);

    nsIntPoint GetChromeDisplacement() { return mChromeDisp; };

    bool IPCOpen() { return mIPCOpen; }

    bool ParentIsActive()
    {
      return mParentIsActive;
    }

protected:
    virtual ~TabChild();

    virtual PRenderFrameChild* AllocPRenderFrameChild() MOZ_OVERRIDE;
    virtual bool DeallocPRenderFrameChild(PRenderFrameChild* aFrame) MOZ_OVERRIDE;
    virtual bool RecvDestroy() MOZ_OVERRIDE;
    virtual bool RecvSetUpdateHitRegion(const bool& aEnabled) MOZ_OVERRIDE;
    virtual bool RecvSetIsDocShellActive(const bool& aIsActive) MOZ_OVERRIDE;

    virtual bool RecvRequestNotifyAfterRemotePaint() MOZ_OVERRIDE;

    virtual bool RecvParentActivated(const bool& aActivated) MOZ_OVERRIDE;

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

    void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;

    enum FrameScriptLoading { DONT_LOAD_SCRIPTS, DEFAULT_LOAD_SCRIPTS };
    bool InitTabChildGlobal(FrameScriptLoading aScriptLoading = DEFAULT_LOAD_SCRIPTS);
    bool InitRenderingState(const ScrollingBehavior& aScrolling,
                            const TextureFactoryIdentifier& aTextureFactoryIdentifier,
                            const uint64_t& aLayersId,
                            PRenderFrameChild* aRenderFrame);
    void DestroyWindow();
    void SetProcessNameToAppName();

    
    void DoFakeShow(const ScrollingBehavior& aScrolling,
                    const TextureFactoryIdentifier& aTextureFactoryIdentifier,
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
    nsRefPtr<SetTargetAPZCCallback> mSetTargetAPZCCallback;
    bool mHasValidInnerSize;
    bool mDestroyed;
    
    nsIntPoint mChromeDisp;
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
