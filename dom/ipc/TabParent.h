





#ifndef mozilla_tabs_TabParent_h
#define mozilla_tabs_TabParent_h

#include "js/TypeDecls.h"
#include "mozilla/dom/ipc/IdType.h"
#include "mozilla/dom/PBrowserParent.h"
#include "mozilla/dom/PFilePickerParent.h"
#include "mozilla/dom/TabContext.h"
#include "mozilla/EventForwards.h"
#include "mozilla/dom/File.h"
#include "mozilla/WritingModes.h"
#include "mozilla/RefPtr.h"
#include "nsCOMPtr.h"
#include "nsIAuthPromptProvider.h"
#include "nsIBrowserDOMWindow.h"
#include "nsIDOMEventListener.h"
#include "nsISecureBrowserUI.h"
#include "nsITabParent.h"
#include "nsIXULBrowserWindow.h"
#include "nsRefreshDriver.h"
#include "nsWeakReference.h"
#include "Units.h"
#include "nsIWidget.h"

class nsFrameLoader;
class nsIFrameLoader;
class nsIContent;
class nsIPrincipal;
class nsIURI;
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

namespace gfx {
class SourceSurface;
class DataSourceSurface;
}

namespace dom {

class ClonedMessageData;
class nsIContentParent;
class Element;
class DataTransfer;
struct StructuredCloneData;

class TabParent final : public PBrowserParent
                      , public nsIDOMEventListener
                      , public nsITabParent
                      , public nsIAuthPromptProvider
                      , public nsISecureBrowserUI
                      , public nsSupportsWeakReference
                      , public TabContext
                      , public nsAPostRefreshObserver
{
    typedef mozilla::dom::ClonedMessageData ClonedMessageData;

    virtual ~TabParent();

public:
    
    NS_DECL_NSITABPARENT
    
    NS_DECL_NSIDOMEVENTLISTENER

    TabParent(nsIContentParent* aManager,
              const TabId& aTabId,
              const TabContext& aContext,
              uint32_t aChromeFlags);
    Element* GetOwnerElement() const { return mFrameElement; }
    void SetOwnerElement(Element* aElement);

    void CacheFrameLoader(nsFrameLoader* aFrameLoader);

    


    void GetAppType(nsAString& aOut);

    





    bool IsVisible();

    nsIBrowserDOMWindow *GetBrowserDOMWindow() { return mBrowserDOMWindow; }
    void SetBrowserDOMWindow(nsIBrowserDOMWindow* aBrowserDOMWindow) {
        mBrowserDOMWindow = aBrowserDOMWindow;
    }

    already_AddRefed<nsILoadContext> GetLoadContext();

    nsIXULBrowserWindow* GetXULBrowserWindow();

    void Destroy();

    void RemoveWindowListeners();
    void AddWindowListeners();
    void DidRefresh() override;

    virtual bool RecvMoveFocus(const bool& aForward) override;
    virtual bool RecvEvent(const RemoteDOMEvent& aEvent) override;
    virtual bool RecvReplyKeyEvent(const WidgetKeyboardEvent& aEvent) override;
    virtual bool RecvDispatchAfterKeyboardEvent(const WidgetKeyboardEvent& aEvent) override;
    virtual bool RecvBrowserFrameOpenWindow(PBrowserParent* aOpener,
                                            const nsString& aURL,
                                            const nsString& aName,
                                            const nsString& aFeatures,
                                            bool* aOutWindowOpened) override;
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
                                  nsCString* aURLToLoad) override;
    virtual bool RecvSyncMessage(const nsString& aMessage,
                                 const ClonedMessageData& aData,
                                 InfallibleTArray<CpowEntry>&& aCpows,
                                 const IPC::Principal& aPrincipal,
                                 InfallibleTArray<nsString>* aJSONRetVal) override;
    virtual bool RecvRpcMessage(const nsString& aMessage,
                                const ClonedMessageData& aData,
                                InfallibleTArray<CpowEntry>&& aCpows,
                                const IPC::Principal& aPrincipal,
                                InfallibleTArray<nsString>* aJSONRetVal) override;
    virtual bool RecvAsyncMessage(const nsString& aMessage,
                                  const ClonedMessageData& aData,
                                  InfallibleTArray<CpowEntry>&& aCpows,
                                  const IPC::Principal& aPrincipal) override;
    virtual bool RecvNotifyIMEFocus(const bool& aFocus,
                                    nsIMEUpdatePreference* aPreference,
                                    uint32_t* aSeqno) override;
    virtual bool RecvNotifyIMETextChange(const uint32_t& aStart,
                                         const uint32_t& aEnd,
                                         const uint32_t& aNewEnd,
                                         const bool& aCausedByComposition) override;
    virtual bool RecvNotifyIMESelectedCompositionRect(
                   const uint32_t& aOffset,
                   InfallibleTArray<LayoutDeviceIntRect>&& aRects,
                   const uint32_t& aCaretOffset,
                   const LayoutDeviceIntRect& aCaretRect) override;
    virtual bool RecvNotifyIMESelection(const uint32_t& aSeqno,
                                        const uint32_t& aAnchor,
                                        const uint32_t& aFocus,
                                        const mozilla::WritingMode& aWritingMode,
                                        const bool& aCausedByComposition) override;
    virtual bool RecvNotifyIMETextHint(const nsString& aText) override;
    virtual bool RecvNotifyIMEMouseButtonEvent(const widget::IMENotification& aEventMessage,
                                               bool* aConsumedByIME) override;
    virtual bool RecvNotifyIMEEditorRect(const LayoutDeviceIntRect& aRect) override;
    virtual bool RecvNotifyIMEPositionChange(
                   const LayoutDeviceIntRect& aEditorRect,
                   InfallibleTArray<LayoutDeviceIntRect>&& aCompositionRects,
                   const LayoutDeviceIntRect& aCaretRect) override;
    virtual bool RecvEndIMEComposition(const bool& aCancel,
                                       bool* aNoCompositionEvent,
                                       nsString* aComposition) override;
    virtual bool RecvStartPluginIME(const WidgetKeyboardEvent& aKeyboardEvent,
                                    const int32_t& aPanelX,
                                    const int32_t& aPanelY,
                                    nsString* aCommitted) override;
    virtual bool RecvSetPluginFocused(const bool& aFocused) override;
    virtual bool RecvGetInputContext(int32_t* aIMEEnabled,
                                     int32_t* aIMEOpen,
                                     intptr_t* aNativeIMEContext) override;
    virtual bool RecvSetInputContext(const int32_t& aIMEEnabled,
                                     const int32_t& aIMEOpen,
                                     const nsString& aType,
                                     const nsString& aInputmode,
                                     const nsString& aActionHint,
                                     const int32_t& aCause,
                                     const int32_t& aFocusChange) override;
    virtual bool RecvRequestFocus(const bool& aCanRaise) override;
    virtual bool RecvEnableDisableCommands(const nsString& aAction,
                                           nsTArray<nsCString>&& aEnabledCommands,
                                           nsTArray<nsCString>&& aDisabledCommands) override;
    virtual bool RecvSetCursor(const uint32_t& aValue, const bool& aForce) override;
    virtual bool RecvSetBackgroundColor(const nscolor& aValue) override;
    virtual bool RecvSetStatus(const uint32_t& aType, const nsString& aStatus) override;
    virtual bool RecvIsParentWindowMainWidgetVisible(bool* aIsVisible) override;
    virtual bool RecvShowTooltip(const uint32_t& aX, const uint32_t& aY, const nsString& aTooltip) override;
    virtual bool RecvHideTooltip() override;
    virtual bool RecvGetTabOffset(LayoutDeviceIntPoint* aPoint) override;
    virtual bool RecvGetDPI(float* aValue) override;
    virtual bool RecvGetDefaultScale(double* aValue) override;
    virtual bool RecvGetWidgetNativeData(WindowsHandle* aValue) override;
    virtual bool RecvZoomToRect(const uint32_t& aPresShellId,
                                const ViewID& aViewId,
                                const CSSRect& aRect) override;
    virtual bool RecvUpdateZoomConstraints(const uint32_t& aPresShellId,
                                           const ViewID& aViewId,
                                           const bool& aIsRoot,
                                           const ZoomConstraints& aConstraints) override;
    virtual bool RecvContentReceivedInputBlock(const ScrollableLayerGuid& aGuid,
                                               const uint64_t& aInputBlockId,
                                               const bool& aPreventDefault) override;
    virtual bool RecvSetTargetAPZC(const uint64_t& aInputBlockId,
                                   nsTArray<ScrollableLayerGuid>&& aTargets) override;
    virtual bool RecvSetAllowedTouchBehavior(const uint64_t& aInputBlockId,
                                             nsTArray<TouchBehaviorFlags>&& aTargets) override;
    virtual bool RecvDispatchWheelEvent(const mozilla::WidgetWheelEvent& aEvent) override;
    virtual bool RecvDispatchMouseEvent(const mozilla::WidgetMouseEvent& aEvent) override;
    virtual bool RecvDispatchKeyboardEvent(const mozilla::WidgetKeyboardEvent& aEvent) override;

    virtual PColorPickerParent*
    AllocPColorPickerParent(const nsString& aTitle, const nsString& aInitialColor) override;
    virtual bool DeallocPColorPickerParent(PColorPickerParent* aColorPicker) override;

    void LoadURL(nsIURI* aURI);
    
    
    
    void Show(const ScreenIntSize& size, bool aParentIsActive);
    void UpdateDimensions(const nsIntRect& rect, const ScreenIntSize& size);
    void UpdateFrame(const layers::FrameMetrics& aFrameMetrics);
    void UIResolutionChanged();
    void ThemeChanged();
    void RequestFlingSnap(const FrameMetrics::ViewID& aScrollId,
                          const mozilla::CSSPoint& aDestination);
    void AcknowledgeScrollUpdate(const ViewID& aScrollId, const uint32_t& aScrollGeneration);
    void HandleDoubleTap(const CSSPoint& aPoint,
                         Modifiers aModifiers,
                         const ScrollableLayerGuid& aGuid);
    void HandleSingleTap(const CSSPoint& aPoint,
                         Modifiers aModifiers,
                         const ScrollableLayerGuid& aGuid);
    void HandleLongTap(const CSSPoint& aPoint,
                       Modifiers aModifiers,
                       const ScrollableLayerGuid& aGuid,
                       uint64_t aInputBlockId);
    void NotifyAPZStateChange(ViewID aViewId,
                              APZStateChange aChange,
                              int aArg);
    void NotifyMouseScrollTestEvent(const ViewID& aScrollId, const nsString& aEvent);
    void Activate();
    void Deactivate();

    bool MapEventCoordinatesForChildProcess(mozilla::WidgetEvent* aEvent);
    void MapEventCoordinatesForChildProcess(const LayoutDeviceIntPoint& aOffset,
                                            mozilla::WidgetEvent* aEvent);
    LayoutDeviceToCSSScale GetLayoutDeviceToCSSScale();

    virtual bool RecvRequestNativeKeyBindings(const mozilla::WidgetKeyboardEvent& aEvent,
                                              MaybeNativeKeyBinding* aBindings) override;

    virtual bool RecvSynthesizeNativeKeyEvent(const int32_t& aNativeKeyboardLayout,
                                              const int32_t& aNativeKeyCode,
                                              const uint32_t& aModifierFlags,
                                              const nsString& aCharacters,
                                              const nsString& aUnmodifiedCharacters,
                                              const uint64_t& aObserverId) override;
    virtual bool RecvSynthesizeNativeMouseEvent(const LayoutDeviceIntPoint& aPoint,
                                                const uint32_t& aNativeMessage,
                                                const uint32_t& aModifierFlags,
                                                const uint64_t& aObserverId) override;
    virtual bool RecvSynthesizeNativeMouseMove(const LayoutDeviceIntPoint& aPoint,
                                               const uint64_t& aObserverId) override;
    virtual bool RecvSynthesizeNativeMouseScrollEvent(const LayoutDeviceIntPoint& aPoint,
                                                      const uint32_t& aNativeMessage,
                                                      const double& aDeltaX,
                                                      const double& aDeltaY,
                                                      const double& aDeltaZ,
                                                      const uint32_t& aModifierFlags,
                                                      const uint32_t& aAdditionalFlags,
                                                      const uint64_t& aObserverId) override;
    virtual bool RecvSynthesizeNativeTouchPoint(const uint32_t& aPointerId,
                                                const TouchPointerState& aPointerState,
                                                const nsIntPoint& aPointerScreenPoint,
                                                const double& aPointerPressure,
                                                const uint32_t& aPointerOrientation,
                                                const uint64_t& aObserverId) override;
    virtual bool RecvSynthesizeNativeTouchTap(const nsIntPoint& aPointerScreenPoint,
                                              const bool& aLongTap,
                                              const uint64_t& aObserverId) override;
    virtual bool RecvClearNativeTouchSequence(const uint64_t& aObserverId) override;

    void SendMouseEvent(const nsAString& aType, float aX, float aY,
                        int32_t aButton, int32_t aClickCount,
                        int32_t aModifiers, bool aIgnoreRootScrollFrame);
    void SendKeyEvent(const nsAString& aType, int32_t aKeyCode,
                      int32_t aCharCode, int32_t aModifiers,
                      bool aPreventDefault);
    bool SendRealMouseEvent(mozilla::WidgetMouseEvent& event);
    bool SendRealDragEvent(mozilla::WidgetDragEvent& aEvent, uint32_t aDragAction,
                           uint32_t aDropEffect);
    bool SendMouseWheelEvent(mozilla::WidgetWheelEvent& event);
    bool SendRealKeyEvent(mozilla::WidgetKeyboardEvent& event);
    bool SendRealTouchEvent(WidgetTouchEvent& event);
    bool SendHandleSingleTap(const CSSPoint& aPoint, const Modifiers& aModifiers, const ScrollableLayerGuid& aGuid);
    bool SendHandleLongTap(const CSSPoint& aPoint, const Modifiers& aModifiers, const ScrollableLayerGuid& aGuid, const uint64_t& aInputBlockId);
    bool SendHandleDoubleTap(const CSSPoint& aPoint, const Modifiers& aModifiers, const ScrollableLayerGuid& aGuid);

    virtual PDocumentRendererParent*
    AllocPDocumentRendererParent(const nsRect& documentRect,
                                 const gfx::Matrix& transform,
                                 const nsString& bgcolor,
                                 const uint32_t& renderFlags,
                                 const bool& flushLayout,
                                 const nsIntSize& renderSize) override;
    virtual bool DeallocPDocumentRendererParent(PDocumentRendererParent* actor) override;

    virtual PFilePickerParent*
    AllocPFilePickerParent(const nsString& aTitle,
                           const int16_t& aMode) override;
    virtual bool DeallocPFilePickerParent(PFilePickerParent* actor) override;

    virtual PIndexedDBPermissionRequestParent*
    AllocPIndexedDBPermissionRequestParent(const Principal& aPrincipal)
                                           override;

    virtual bool
    RecvPIndexedDBPermissionRequestConstructor(
                                      PIndexedDBPermissionRequestParent* aActor,
                                      const Principal& aPrincipal)
                                      override;

    virtual bool
    DeallocPIndexedDBPermissionRequestParent(
                                      PIndexedDBPermissionRequestParent* aActor)
                                      override;

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

    LayoutDeviceIntPoint GetChildProcessOffset();

    


    virtual PPluginWidgetParent* AllocPPluginWidgetParent() override;
    virtual bool DeallocPPluginWidgetParent(PPluginWidgetParent* aActor) override;

    void SetInitedByParent() { mInitedByParent = true; }
    bool IsInitedByParent() const { return mInitedByParent; }

    static TabParent* GetNextTabParent();

    bool SendLoadRemoteScript(const nsString& aURL,
                              const bool& aRunInGlobalScope);

    
    bool RequestNotifyLayerTreeReady();
    bool RequestNotifyLayerTreeCleared();
    bool LayerTreeUpdate(bool aActive);
    void SwapLayerTreeObservers(TabParent* aOther);

    virtual bool
    RecvInvokeDragSession(nsTArray<IPCDataTransfer>&& aTransfers,
                          const uint32_t& aAction,
                          const nsCString& aVisualDnDData,
                          const uint32_t& aWidth, const uint32_t& aHeight,
                          const uint32_t& aStride, const uint8_t& aFormat,
                          const int32_t& aDragAreaX, const int32_t& aDragAreaY) override;

    void AddInitialDnDDataTo(DataTransfer* aDataTransfer);

    void TakeDragVisualization(RefPtr<mozilla::gfx::SourceSurface>& aSurface,
                               int32_t& aDragAreaX, int32_t& aDragAreaY);
protected:
    bool ReceiveMessage(const nsString& aMessage,
                        bool aSync,
                        const StructuredCloneData* aCloneData,
                        mozilla::jsipc::CpowHolder* aCpows,
                        nsIPrincipal* aPrincipal,
                        InfallibleTArray<nsString>* aJSONRetVal = nullptr);

    virtual bool RecvAsyncAuthPrompt(const nsCString& aUri,
                                     const nsString& aRealm,
                                     const uint64_t& aCallbackId) override;

    virtual bool Recv__delete__() override;

    virtual void ActorDestroy(ActorDestroyReason why) override;

    Element* mFrameElement;
    nsCOMPtr<nsIBrowserDOMWindow> mBrowserDOMWindow;

    bool AllowContentIME();

    virtual PRenderFrameParent* AllocPRenderFrameParent() override;
    virtual bool DeallocPRenderFrameParent(PRenderFrameParent* aFrame) override;

    virtual bool RecvRemotePaintIsReady() override;

    virtual bool RecvGetRenderFrameInfo(PRenderFrameParent* aRenderFrame,
                                        TextureFactoryIdentifier* aTextureFactoryIdentifier,
                                        uint64_t* aLayersId) override;

    virtual bool RecvSetDimensions(const uint32_t& aFlags,
                                   const int32_t& aX, const int32_t& aY,
                                   const int32_t& aCx, const int32_t& aCy) override;

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
    uint32_t mIMEEventCountAfterEnding;
    
    
    nsAutoString mIMECompositionText;
    uint32_t mIMECompositionStart;
    uint32_t mIMESeqno;

    uint32_t mIMECompositionRectOffset;
    InfallibleTArray<LayoutDeviceIntRect> mIMECompositionRects;
    uint32_t mIMECaretOffset;
    LayoutDeviceIntRect mIMECaretRect;
    LayoutDeviceIntRect mIMEEditorRect;

    nsIntRect mRect;
    ScreenIntSize mDimensions;
    ScreenOrientation mOrientation;
    float mDPI;
    CSSToLayoutDeviceScale mDefaultScale;
    bool mUpdatedDimensions;
    LayoutDeviceIntPoint mChromeOffset;

private:
    already_AddRefed<nsFrameLoader> GetFrameLoader(bool aUseCachedFrameLoaderAfterDestroy = false) const;
    layout::RenderFrameParent* GetRenderFrame();
    nsRefPtr<nsIContentParent> mManager;
    void TryCacheDPIAndScale();

    nsresult UpdatePosition();

    CSSPoint AdjustTapToChildWidget(const CSSPoint& aPoint);

    
    
    
    
    
    
    
    void ApzAwareEventRoutingToChild(ScrollableLayerGuid* aOutTargetGuid,
                                     uint64_t* aOutInputBlockId);
    
    
    bool mMarkedDestroying;
    
    
    bool mIsDestroyed;
    
    bool mAppPackageFileDescriptorSent;

    
    
    bool mSendOfflineStatus;

    uint32_t mChromeFlags;

    struct DataTransferItem
    {
      nsCString mFlavor;
      nsString mStringData;
      nsRefPtr<mozilla::dom::FileImpl> mBlobData;
      enum DataType
      {
        eString,
        eBlob
      };
      DataType mType;
    };
    nsTArray<nsTArray<DataTransferItem>> mInitialDataTransferItems;

    mozilla::RefPtr<gfx::DataSourceSurface> mDnDVisualization;
    int32_t mDragAreaX;
    int32_t mDragAreaY;

    
    
    bool mInitedByParent;

    nsCOMPtr<nsILoadContext> mLoadContext;

    
    
    
    nsRefPtr<nsFrameLoader> mFrameLoader;

    TabId mTabId;

    
    struct AutoUseNewTab;

    
    
    
    
    
    static TabParent* sNextTabParent;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    bool mCreatingWindow;
    nsCString mDelayedURL;

    
    
    
    
    
    
    
    nsTArray<FrameScriptInfo> mDelayedFrameScripts;

    
    
    
    bool mNeedLayerTreeReadyNotification;

    
    
    nsCursor mCursor;

    
    
    bool mTabSetsCursor;

    nsRefPtr<nsIPresShell> mPresShellWithRefreshListener;

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
