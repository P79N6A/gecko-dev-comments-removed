





#ifndef mozilla_dom_TabChild_h
#define mozilla_dom_TabChild_h

#include "mozilla/dom/PBrowserChild.h"
#ifdef DEBUG
#include "PCOMContentPermissionRequestChild.h"
#endif 
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
#include "nsIDialogCreator.h"
#include "nsIPresShell.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsWeakReference.h"
#include "nsITabChild.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/TabContext.h"
#include "mozilla/EventForwards.h"

struct gfxMatrix;
class nsICachedFileDescriptorListener;
class nsIDOMWindowUtils;

namespace mozilla {
namespace layout {
class RenderFrameChild;
}

namespace layers {
struct TextureFactoryIdentifier;
}

namespace dom {

class TabChild;
class PContentDialogChild;
class ClonedMessageData;

class TabChildGlobal : public nsDOMEventTargetHelper,
                       public nsIContentFrameMessageManager,
                       public nsIScriptObjectPrincipal
{
public:
  TabChildGlobal(TabChild* aTabChild);
  void Init();
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(TabChildGlobal, nsDOMEventTargetHelper)
  NS_FORWARD_SAFE_NSIMESSAGELISTENERMANAGER(mMessageManager)
  NS_FORWARD_SAFE_NSIMESSAGESENDER(mMessageManager)
  NS_IMETHOD SendSyncMessage(const nsAString& aMessageName,
                             const JS::Value& aObject,
                             const JS::Value& aRemote,
                             JSContext* aCx,
                             uint8_t aArgc,
                             JS::Value* aRetval)
  {
    return mMessageManager
      ? mMessageManager->SendSyncMessage(aMessageName, aObject, aRemote, aCx, aArgc, aRetval)
      : NS_ERROR_NULL_POINTER;
  }
  NS_IMETHOD SendRpcMessage(const nsAString& aMessageName,
                            const JS::Value& aObject,
                            const JS::Value& aRemote,
                            JSContext* aCx,
                            uint8_t aArgc,
                            JS::Value* aRetval)
  {
    return mMessageManager
      ? mMessageManager->SendRpcMessage(aMessageName, aObject, aRemote, aCx, aArgc, aRetval)
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
                 public nsIDialogCreator,
                 public nsITabChild,
                 public nsIObserver,
                 public ipc::MessageManagerCallback,
                 public TabContext
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
    NS_DECL_NSIDIALOGCREATOR
    NS_DECL_NSITABCHILD
    NS_DECL_NSIOBSERVER

    


    virtual bool DoSendBlockingMessage(JSContext* aCx,
                                       const nsAString& aMessage,
                                       const mozilla::dom::StructuredCloneData& aData,
                                       JS::Handle<JSObject *> aCpows,
                                       InfallibleTArray<nsString>* aJSONRetVal,
                                       bool aIsSync) MOZ_OVERRIDE;
    virtual bool DoSendAsyncMessage(JSContext* aCx,
                                    const nsAString& aMessage,
                                    const mozilla::dom::StructuredCloneData& aData,
                                    JS::Handle<JSObject *> aCpows) MOZ_OVERRIDE;

    virtual bool RecvLoadURL(const nsCString& uri);
    virtual bool RecvCacheFileDescriptor(const nsString& aPath,
                                         const FileDescriptor& aFileDescriptor)
                                         MOZ_OVERRIDE;
    virtual bool RecvShow(const nsIntSize& size);
    virtual bool RecvUpdateDimensions(const nsRect& rect, const nsIntSize& size, const ScreenOrientation& orientation);
    virtual bool RecvUpdateFrame(const mozilla::layers::FrameMetrics& aFrameMetrics);
    virtual bool RecvHandleDoubleTap(const CSSIntPoint& aPoint);
    virtual bool RecvHandleSingleTap(const CSSIntPoint& aPoint);
    virtual bool RecvHandleLongTap(const CSSIntPoint& aPoint);
    virtual bool RecvActivate();
    virtual bool RecvDeactivate();
    virtual bool RecvMouseEvent(const nsString& aType,
                                const float&    aX,
                                const float&    aY,
                                const int32_t&  aButton,
                                const int32_t&  aClickCount,
                                const int32_t&  aModifiers,
                                const bool&     aIgnoreRootScrollFrame);
    virtual bool RecvRealMouseEvent(const mozilla::WidgetMouseEvent& event);
    virtual bool RecvRealKeyEvent(const mozilla::WidgetKeyboardEvent& event);
    virtual bool RecvMouseWheelEvent(const mozilla::WidgetWheelEvent& event);
    virtual bool RecvRealTouchEvent(const WidgetTouchEvent& event);
    virtual bool RecvRealTouchMoveEvent(const WidgetTouchEvent& event);
    virtual bool RecvKeyEvent(const nsString& aType,
                              const int32_t&  aKeyCode,
                              const int32_t&  aCharCode,
                              const int32_t&  aModifiers,
                              const bool&     aPreventDefault);
    virtual bool RecvCompositionEvent(const mozilla::WidgetCompositionEvent& event);
    virtual bool RecvTextEvent(const mozilla::WidgetTextEvent& event);
    virtual bool RecvSelectionEvent(const mozilla::WidgetSelectionEvent& event);
    virtual bool RecvActivateFrameEvent(const nsString& aType, const bool& capture);
    virtual bool RecvLoadRemoteScript(const nsString& aURL);
    virtual bool RecvAsyncMessage(const nsString& aMessage,
                                  const ClonedMessageData& aData,
                                  const InfallibleTArray<CpowEntry>& aCpows);

    virtual PDocumentRendererChild*
    AllocPDocumentRendererChild(const nsRect& documentRect, const gfxMatrix& transform,
                                const nsString& bgcolor,
                                const uint32_t& renderFlags, const bool& flushLayout,
                                const nsIntSize& renderSize);
    virtual bool DeallocPDocumentRendererChild(PDocumentRendererChild* actor);
    virtual bool RecvPDocumentRendererConstructor(PDocumentRendererChild* actor,
                                                  const nsRect& documentRect,
                                                  const gfxMatrix& transform,
                                                  const nsString& bgcolor,
                                                  const uint32_t& renderFlags,
                                                  const bool& flushLayout,
                                                  const nsIntSize& renderSize);

    virtual PContentDialogChild* AllocPContentDialogChild(const uint32_t&,
                                                          const nsCString&,
                                                          const nsCString&,
                                                          const InfallibleTArray<int>&,
                                                          const InfallibleTArray<nsString>&);
    virtual bool DeallocPContentDialogChild(PContentDialogChild* aDialog);
    static void ParamsToArrays(nsIDialogParamBlock* aParams,
                               InfallibleTArray<int>& aIntParams,
                               InfallibleTArray<nsString>& aStringParams);
    static void ArraysToParams(const InfallibleTArray<int>& aIntParams,
                               const InfallibleTArray<nsString>& aStringParams,
                               nsIDialogParamBlock* aParams);

#ifdef DEBUG
    virtual PContentPermissionRequestChild* SendPContentPermissionRequestConstructor(PContentPermissionRequestChild* aActor,
                                                                                     const nsCString& aType,
                                                                                     const nsCString& aAccess,
                                                                                     const IPC::Principal& aPrincipal)
    {
      PCOMContentPermissionRequestChild* child = static_cast<PCOMContentPermissionRequestChild*>(aActor);
      PContentPermissionRequestChild* request = PBrowserChild::SendPContentPermissionRequestConstructor(aActor, aType, aAccess, aPrincipal);
      child->mIPCOpen = true;
      return request;
    }
#endif 

    virtual PContentPermissionRequestChild* AllocPContentPermissionRequestChild(const nsCString& aType,
                                                                                const nsCString& aAccess,
                                                                                const IPC::Principal& aPrincipal);
    virtual bool DeallocPContentPermissionRequestChild(PContentPermissionRequestChild* actor);

    virtual POfflineCacheUpdateChild* AllocPOfflineCacheUpdateChild(
            const URIParams& manifestURI,
            const URIParams& documentURI,
            const bool& stickDocument);
    virtual bool DeallocPOfflineCacheUpdateChild(POfflineCacheUpdateChild* offlineCacheUpdate);

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
    virtual PRenderFrameChild* AllocPRenderFrameChild(ScrollingBehavior* aScrolling,
                                                      TextureFactoryIdentifier* aTextureFactoryIdentifier,
                                                      uint64_t* aLayersId) MOZ_OVERRIDE;
    virtual bool DeallocPRenderFrameChild(PRenderFrameChild* aFrame) MOZ_OVERRIDE;
    virtual bool RecvDestroy() MOZ_OVERRIDE;
    virtual bool RecvSetUpdateHitRegion(const bool& aEnabled) MOZ_OVERRIDE;

    nsEventStatus DispatchWidgetEvent(WidgetGUIEvent& event);

    virtual PIndexedDBChild* AllocPIndexedDBChild(const nsCString& aGroup,
                                                  const nsCString& aASCIIOrigin,
                                                  bool* );

    virtual bool DeallocPIndexedDBChild(PIndexedDBChild* aActor);

private:
    







    TabChild(ContentChild* aManager, const TabContext& aContext, uint32_t aChromeFlags);

    nsresult Init();

    
    
    
    
    
    void NotifyTabContextUpdated();

    bool UseDirectCompositor();

    void ActorDestroy(ActorDestroyReason why);

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

    class CachedFileDescriptorInfo;
    class CachedFileDescriptorCallbackRunnable;

    TextureFactoryIdentifier mTextureFactoryIdentifier;
    nsCOMPtr<nsIWebNavigation> mWebNav;
    nsCOMPtr<nsIWidget> mWidget;
    nsCOMPtr<nsIURI> mLastURI;
    FrameMetrics mLastMetrics;
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

    DISALLOW_EVIL_CONSTRUCTORS(TabChild);
};

}
}

#endif 
