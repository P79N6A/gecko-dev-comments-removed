





#ifndef mozilla_tabs_TabParent_h
#define mozilla_tabs_TabParent_h

#include "mozilla/EventForwards.h"
#include "mozilla/dom/PBrowserParent.h"
#include "mozilla/dom/PFilePickerParent.h"
#include "mozilla/dom/TabContext.h"
#include "mozilla/dom/ipc/IdType.h"
#include "nsCOMPtr.h"
#include "nsIAuthPromptProvider.h"
#include "nsIBrowserDOMWindow.h"
#include "nsISecureBrowserUI.h"
#include "nsITabParent.h"
#include "nsIXULBrowserWindow.h"
#include "nsWeakReference.h"
#include "Units.h"
#include "WritingModes.h"
#include "js/TypeDecls.h"
#include "nsIDOMEventListener.h"

class nsFrameLoader;
class nsIFrameLoader;
class nsIContent;
class nsIPrincipal;
class nsIURI;
class nsIWidget;
class nsILoadContext;
class nsIDocShell;

namespace mozilla {

namespace jsipc {
class CpowHolder;
}

namespace layers {
struct FrameMetrics;
struct TextureFactoryIdentifier;
}

namespace layout {
class RenderFrameParent;
}

namespace widget {
struct IMENotification;
}

namespace dom {

class ClonedMessageData;
class nsIContentParent;
class Element;
struct StructuredCloneData;

class TabParent : public PBrowserParent
                , public nsIDOMEventListener
                , public nsITabParent 
                , public nsIAuthPromptProvider
                , public nsISecureBrowserUI
                , public nsSupportsWeakReference
                , public TabContext
{
    typedef mozilla::dom::ClonedMessageData ClonedMessageData;
    typedef mozilla::layout::ScrollingBehavior ScrollingBehavior;

    virtual ~TabParent();

public:
    
    NS_DECL_NSITABPARENT

    TabParent(nsIContentParent* aManager,
              const TabId& aTabId,
              const TabContext& aContext,
              uint32_t aChromeFlags);
    Element* GetOwnerElement() const { return mFrameElement; }
    void SetOwnerElement(Element* aElement);

    


    void GetAppType(nsAString& aOut);

    





    bool IsVisible();

    nsIBrowserDOMWindow *GetBrowserDOMWindow() { return mBrowserDOMWindow; }
    void SetBrowserDOMWindow(nsIBrowserDOMWindow* aBrowserDOMWindow) {
        mBrowserDOMWindow = aBrowserDOMWindow;
    }

    
    NS_DECL_NSIDOMEVENTLISTENER

    already_AddRefed<nsILoadContext> GetLoadContext();

    nsIXULBrowserWindow* GetXULBrowserWindow();

    










    static TabParent* GetEventCapturer();
    









    bool TryCapture(const WidgetGUIEvent& aEvent);

    void Destroy();

    virtual bool RecvMoveFocus(const bool& aForward) MOZ_OVERRIDE;
    virtual bool RecvEvent(const RemoteDOMEvent& aEvent) MOZ_OVERRIDE;
    virtual bool RecvReplyKeyEvent(const WidgetKeyboardEvent& aEvent) MOZ_OVERRIDE;
    virtual bool RecvDispatchAfterKeyboardEvent(const WidgetKeyboardEvent& aEvent) MOZ_OVERRIDE;
    virtual bool RecvBrowserFrameOpenWindow(PBrowserParent* aOpener,
                                            const nsString& aURL,
                                            const nsString& aName,
                                            const nsString& aFeatures,
                                            bool* aOutWindowOpened) MOZ_OVERRIDE;
    virtual bool RecvCreateWindow(PBrowserParent* aOpener,
                                  const uint32_t& aChromeFlags,
                                  const bool& aCalledFromJS,
                                  const bool& aPositionSpecified,
                                  const bool& aSizeSpecified,
                                  const nsString& aURI,
                                  const nsString& aName,
                                  const nsString& aFeatures,
                                  const nsString& aBaseURI,
                                  bool* aWindowIsNew,
                                  InfallibleTArray<FrameScriptInfo>* aFrameScripts,
                                  nsCString* aURLToLoad) MOZ_OVERRIDE;
    virtual bool RecvSyncMessage(const nsString& aMessage,
                                 const ClonedMessageData& aData,
                                 InfallibleTArray<CpowEntry>&& aCpows,
                                 const IPC::Principal& aPrincipal,
                                 InfallibleTArray<nsString>* aJSONRetVal) MOZ_OVERRIDE;
    virtual bool RecvRpcMessage(const nsString& aMessage,
                                const ClonedMessageData& aData,
                                InfallibleTArray<CpowEntry>&& aCpows,
                                const IPC::Principal& aPrincipal,
                                InfallibleTArray<nsString>* aJSONRetVal) MOZ_OVERRIDE;
    virtual bool RecvAsyncMessage(const nsString& aMessage,
                                  const ClonedMessageData& aData,
                                  InfallibleTArray<CpowEntry>&& aCpows,
                                  const IPC::Principal& aPrincipal) MOZ_OVERRIDE;
    virtual bool RecvNotifyIMEFocus(const bool& aFocus,
                                    nsIMEUpdatePreference* aPreference,
                                    uint32_t* aSeqno) MOZ_OVERRIDE;
    virtual bool RecvNotifyIMETextChange(const uint32_t& aStart,
                                         const uint32_t& aEnd,
                                         const uint32_t& aNewEnd,
                                         const bool& aCausedByComposition) MOZ_OVERRIDE;
    virtual bool RecvNotifyIMESelectedCompositionRect(
                   const uint32_t& aOffset,
                   InfallibleTArray<LayoutDeviceIntRect>&& aRects,
                   const uint32_t& aCaretOffset,
                   const LayoutDeviceIntRect& aCaretRect) MOZ_OVERRIDE;
    virtual bool RecvNotifyIMESelection(const uint32_t& aSeqno,
                                        const uint32_t& aAnchor,
                                        const uint32_t& aFocus,
                                        const mozilla::WritingMode& aWritingMode,
                                        const bool& aCausedByComposition) MOZ_OVERRIDE;
    virtual bool RecvNotifyIMETextHint(const nsString& aText) MOZ_OVERRIDE;
    virtual bool RecvNotifyIMEMouseButtonEvent(const widget::IMENotification& aEventMessage,
                                               bool* aConsumedByIME) MOZ_OVERRIDE;
    virtual bool RecvNotifyIMEEditorRect(const LayoutDeviceIntRect& aRect) MOZ_OVERRIDE;
    virtual bool RecvNotifyIMEPositionChange(
                   const LayoutDeviceIntRect& aEditorRect,
                   InfallibleTArray<LayoutDeviceIntRect>&& aCompositionRects,
                   const LayoutDeviceIntRect& aCaretRect) MOZ_OVERRIDE;
    virtual bool RecvEndIMEComposition(const bool& aCancel,
                                       nsString* aComposition) MOZ_OVERRIDE;
    virtual bool RecvGetInputContext(int32_t* aIMEEnabled,
                                     int32_t* aIMEOpen,
                                     intptr_t* aNativeIMEContext) MOZ_OVERRIDE;
    virtual bool RecvSetInputContext(const int32_t& aIMEEnabled,
                                     const int32_t& aIMEOpen,
                                     const nsString& aType,
                                     const nsString& aInputmode,
                                     const nsString& aActionHint,
                                     const int32_t& aCause,
                                     const int32_t& aFocusChange) MOZ_OVERRIDE;
    virtual bool RecvRequestFocus(const bool& aCanRaise) MOZ_OVERRIDE;
    virtual bool RecvEnableDisableCommands(const nsString& aAction,
                                           nsTArray<nsCString>&& aEnabledCommands,
                                           nsTArray<nsCString>&& aDisabledCommands) MOZ_OVERRIDE;
    virtual bool RecvSetCursor(const uint32_t& aValue, const bool& aForce) MOZ_OVERRIDE;
    virtual bool RecvSetBackgroundColor(const nscolor& aValue) MOZ_OVERRIDE;
    virtual bool RecvSetStatus(const uint32_t& aType, const nsString& aStatus) MOZ_OVERRIDE;
    virtual bool RecvIsParentWindowMainWidgetVisible(bool* aIsVisible) MOZ_OVERRIDE;
    virtual bool RecvShowTooltip(const uint32_t& aX, const uint32_t& aY, const nsString& aTooltip) MOZ_OVERRIDE;
    virtual bool RecvHideTooltip() MOZ_OVERRIDE;
    virtual bool RecvGetDPI(float* aValue) MOZ_OVERRIDE;
    virtual bool RecvGetDefaultScale(double* aValue) MOZ_OVERRIDE;
    virtual bool RecvGetWidgetNativeData(WindowsHandle* aValue) MOZ_OVERRIDE;
    virtual bool RecvZoomToRect(const uint32_t& aPresShellId,
                                const ViewID& aViewId,
                                const CSSRect& aRect) MOZ_OVERRIDE;
    virtual bool RecvUpdateZoomConstraints(const uint32_t& aPresShellId,
                                           const ViewID& aViewId,
                                           const bool& aIsRoot,
                                           const ZoomConstraints& aConstraints) MOZ_OVERRIDE;
    virtual bool RecvContentReceivedInputBlock(const ScrollableLayerGuid& aGuid,
                                               const uint64_t& aInputBlockId,
                                               const bool& aPreventDefault) MOZ_OVERRIDE;
    virtual bool RecvSetTargetAPZC(const uint64_t& aInputBlockId,
                                   nsTArray<ScrollableLayerGuid>&& aTargets) MOZ_OVERRIDE;

    virtual PColorPickerParent*
    AllocPColorPickerParent(const nsString& aTitle, const nsString& aInitialColor) MOZ_OVERRIDE;
    virtual bool DeallocPColorPickerParent(PColorPickerParent* aColorPicker) MOZ_OVERRIDE;

    void LoadURL(nsIURI* aURI);
    
    
    
    void Show(const nsIntSize& size, bool aParentIsActive);
    void UpdateDimensions(const nsIntRect& rect, const nsIntSize& size);
    void UpdateFrame(const layers::FrameMetrics& aFrameMetrics);
    void UIResolutionChanged();
    void AcknowledgeScrollUpdate(const ViewID& aScrollId, const uint32_t& aScrollGeneration);
    void HandleDoubleTap(const CSSPoint& aPoint,
                         int32_t aModifiers,
                         const ScrollableLayerGuid& aGuid);
    void HandleSingleTap(const CSSPoint& aPoint,
                         int32_t aModifiers,
                         const ScrollableLayerGuid& aGuid);
    void HandleLongTap(const CSSPoint& aPoint,
                       int32_t aModifiers,
                       const ScrollableLayerGuid& aGuid,
                       uint64_t aInputBlockId);
    void HandleLongTapUp(const CSSPoint& aPoint,
                         int32_t aModifiers,
                         const ScrollableLayerGuid& aGuid);
    void NotifyAPZStateChange(ViewID aViewId,
                              APZStateChange aChange,
                              int aArg);
    void Activate();
    void Deactivate();

    bool MapEventCoordinatesForChildProcess(mozilla::WidgetEvent* aEvent);
    void MapEventCoordinatesForChildProcess(const LayoutDeviceIntPoint& aOffset,
                                            mozilla::WidgetEvent* aEvent);
    LayoutDeviceToCSSScale GetLayoutDeviceToCSSScale();

    virtual bool RecvRequestNativeKeyBindings(const mozilla::WidgetKeyboardEvent& aEvent,
                                              MaybeNativeKeyBinding* aBindings) MOZ_OVERRIDE;

    void SendMouseEvent(const nsAString& aType, float aX, float aY,
                        int32_t aButton, int32_t aClickCount,
                        int32_t aModifiers, bool aIgnoreRootScrollFrame);
    void SendKeyEvent(const nsAString& aType, int32_t aKeyCode,
                      int32_t aCharCode, int32_t aModifiers,
                      bool aPreventDefault);
    bool SendRealMouseEvent(mozilla::WidgetMouseEvent& event);
    bool SendMouseWheelEvent(mozilla::WidgetWheelEvent& event);
    bool SendRealKeyEvent(mozilla::WidgetKeyboardEvent& event);
    bool SendRealTouchEvent(WidgetTouchEvent& event);
    bool SendHandleSingleTap(const CSSPoint& aPoint, const ScrollableLayerGuid& aGuid);
    bool SendHandleLongTap(const CSSPoint& aPoint, const ScrollableLayerGuid& aGuid, const uint64_t& aInputBlockId);
    bool SendHandleLongTapUp(const CSSPoint& aPoint, const ScrollableLayerGuid& aGuid);
    bool SendHandleDoubleTap(const CSSPoint& aPoint, const ScrollableLayerGuid& aGuid);

    virtual PDocumentRendererParent*
    AllocPDocumentRendererParent(const nsRect& documentRect,
                                 const gfx::Matrix& transform,
                                 const nsString& bgcolor,
                                 const uint32_t& renderFlags,
                                 const bool& flushLayout,
                                 const nsIntSize& renderSize) MOZ_OVERRIDE;
    virtual bool DeallocPDocumentRendererParent(PDocumentRendererParent* actor) MOZ_OVERRIDE;

    virtual PContentPermissionRequestParent*
    AllocPContentPermissionRequestParent(const InfallibleTArray<PermissionRequest>& aRequests,
                                         const IPC::Principal& aPrincipal) MOZ_OVERRIDE;
    virtual bool
    DeallocPContentPermissionRequestParent(PContentPermissionRequestParent* actor) MOZ_OVERRIDE;

    virtual PFilePickerParent*
    AllocPFilePickerParent(const nsString& aTitle,
                           const int16_t& aMode) MOZ_OVERRIDE;
    virtual bool DeallocPFilePickerParent(PFilePickerParent* actor) MOZ_OVERRIDE;

    virtual PIndexedDBPermissionRequestParent*
    AllocPIndexedDBPermissionRequestParent(const Principal& aPrincipal)
                                           MOZ_OVERRIDE;

    virtual bool
    RecvPIndexedDBPermissionRequestConstructor(
                                      PIndexedDBPermissionRequestParent* aActor,
                                      const Principal& aPrincipal)
                                      MOZ_OVERRIDE;

    virtual bool
    DeallocPIndexedDBPermissionRequestParent(
                                      PIndexedDBPermissionRequestParent* aActor)
                                      MOZ_OVERRIDE;

    bool GetGlobalJSObject(JSContext* cx, JSObject** globalp);

    NS_DECL_ISUPPORTS
    NS_DECL_NSIAUTHPROMPTPROVIDER
    NS_DECL_NSISECUREBROWSERUI

    static TabParent *GetIMETabParent() { return mIMETabParent; }
    bool HandleQueryContentEvent(mozilla::WidgetQueryContentEvent& aEvent);
    bool SendCompositionEvent(mozilla::WidgetCompositionEvent& event);
    bool SendSelectionEvent(mozilla::WidgetSelectionEvent& event);

    static TabParent* GetFrom(nsFrameLoader* aFrameLoader);
    static TabParent* GetFrom(nsIFrameLoader* aFrameLoader);
    static TabParent* GetFrom(nsITabParent* aTabParent);
    static TabParent* GetFrom(PBrowserParent* aTabParent);
    static TabParent* GetFrom(nsIContent* aContent);
    static TabId GetTabIdFrom(nsIDocShell* docshell);

    nsIContentParent* Manager() { return mManager; }

    



    bool IsDestroyed() const { return mIsDestroyed; }

    already_AddRefed<nsIWidget> GetWidget() const;

    const TabId GetTabId() const
    {
      return mTabId;
    }

    nsIntPoint GetChildProcessOffset();

    


    virtual PPluginWidgetParent* AllocPPluginWidgetParent() MOZ_OVERRIDE;
    virtual bool DeallocPPluginWidgetParent(PPluginWidgetParent* aActor) MOZ_OVERRIDE;

    void SetInitedByParent() { mInitedByParent = true; }
    bool IsInitedByParent() const { return mInitedByParent; }

    static TabParent* GetNextTabParent();

    bool SendLoadRemoteScript(const nsString& aURL,
                              const bool& aRunInGlobalScope);

protected:
    bool ReceiveMessage(const nsString& aMessage,
                        bool aSync,
                        const StructuredCloneData* aCloneData,
                        mozilla::jsipc::CpowHolder* aCpows,
                        nsIPrincipal* aPrincipal,
                        InfallibleTArray<nsString>* aJSONRetVal = nullptr);

    virtual bool RecvAsyncAuthPrompt(const nsCString& aUri,
                                     const nsString& aRealm,
                                     const uint64_t& aCallbackId) MOZ_OVERRIDE;

    virtual bool Recv__delete__() MOZ_OVERRIDE;

    virtual void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;

    Element* mFrameElement;
    nsCOMPtr<nsIBrowserDOMWindow> mBrowserDOMWindow;

    bool AllowContentIME();

    virtual PRenderFrameParent* AllocPRenderFrameParent() MOZ_OVERRIDE;
    virtual bool DeallocPRenderFrameParent(PRenderFrameParent* aFrame) MOZ_OVERRIDE;

    virtual bool RecvRemotePaintIsReady() MOZ_OVERRIDE;

    virtual bool RecvGetRenderFrameInfo(PRenderFrameParent* aRenderFrame,
                                        ScrollingBehavior* aScrolling,
                                        TextureFactoryIdentifier* aTextureFactoryIdentifier,
                                        uint64_t* aLayersId) MOZ_OVERRIDE;

    virtual bool RecvSetDimensions(const uint32_t& aFlags,
                                   const int32_t& aX, const int32_t& aY,
                                   const int32_t& aCx, const int32_t& aCy) MOZ_OVERRIDE;

    bool SendCompositionChangeEvent(mozilla::WidgetCompositionEvent& event);

    bool InitBrowserConfiguration(nsIURI* aURI,
                                  BrowserConfiguration& aConfiguration);

    
    static TabParent *mIMETabParent;
    nsString mIMECacheText;
    uint32_t mIMESelectionAnchor;
    uint32_t mIMESelectionFocus;
    mozilla::WritingMode mWritingMode;
    bool mIMEComposing;
    bool mIMECompositionEnding;
    
    
    nsAutoString mIMECompositionText;
    uint32_t mIMECompositionStart;
    uint32_t mIMESeqno;

    uint32_t mIMECompositionRectOffset;
    InfallibleTArray<LayoutDeviceIntRect> mIMECompositionRects;
    uint32_t mIMECaretOffset;
    LayoutDeviceIntRect mIMECaretRect;
    LayoutDeviceIntRect mIMEEditorRect;

    
    int32_t mEventCaptureDepth;

    nsIntRect mRect;
    nsIntSize mDimensions;
    ScreenOrientation mOrientation;
    float mDPI;
    CSSToLayoutDeviceScale mDefaultScale;
    bool mShown;
    bool mUpdatedDimensions;

private:
    already_AddRefed<nsFrameLoader> GetFrameLoader() const;
    layout::RenderFrameParent* GetRenderFrame();
    nsRefPtr<nsIContentParent> mManager;
    void TryCacheDPIAndScale();

    CSSPoint AdjustTapToChildWidget(const CSSPoint& aPoint);

    
    
    bool UseAsyncPanZoom();
    
    
    
    
    
    
    
    void ApzAwareEventRoutingToChild(ScrollableLayerGuid* aOutTargetGuid,
                                     uint64_t* aOutInputBlockId);
    
    
    
    
    LayoutDeviceIntPoint mChildProcessOffsetAtTouchStart;
    
    
    bool mMarkedDestroying;
    
    
    bool mIsDestroyed;
    
    bool mAppPackageFileDescriptorSent;

    
    
    bool mSendOfflineStatus;

    uint32_t mChromeFlags;

    
    
    bool mInitedByParent;

    nsCOMPtr<nsILoadContext> mLoadContext;

    TabId mTabId;

    
    struct AutoUseNewTab;

    
    
    
    
    
    static TabParent* sNextTabParent;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    bool mCreatingWindow;
    nsCString mDelayedURL;

    
    
    
    
    
    
    
    nsTArray<FrameScriptInfo> mDelayedFrameScripts;

private:
    
    
    typedef nsDataHashtable<nsUint64HashKey, TabParent*> LayerToTabParentTable;
    static LayerToTabParentTable* sLayerToTabParentTable;

    static void AddTabParentToTable(uint64_t aLayersId, TabParent* aTabParent);
    static void RemoveTabParentFromTable(uint64_t aLayersId);

public:
    static TabParent* GetTabParentFromLayersId(uint64_t aLayersId);
};

} 
} 

#endif
