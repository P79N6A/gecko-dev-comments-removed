





#ifndef mozilla_tabs_TabParent_h
#define mozilla_tabs_TabParent_h

#include "mozilla/dom/PBrowserParent.h"
#include "mozilla/dom/PContentDialogParent.h"
#include "mozilla/dom/TabContext.h"
#include "mozilla/TouchEvents.h"
#include "nsCOMPtr.h"
#include "nsIAuthPromptProvider.h"
#include "nsIBrowserDOMWindow.h"
#include "nsIDialogParamBlock.h"
#include "nsISecureBrowserUI.h"
#include "nsITabParent.h"
#include "Units.h"
#include "js/TypeDecls.h"

struct gfxMatrix;
class nsFrameLoader;
class nsIURI;
class CpowHolder;

namespace mozilla {

namespace layers {
struct FrameMetrics;
struct TextureFactoryIdentifier;
}

namespace layout {
class RenderFrameParent;
}

namespace dom {

class ClonedMessageData;
class ContentParent;
class Element;
struct StructuredCloneData;

class ContentDialogParent : public PContentDialogParent {};

class TabParent : public PBrowserParent 
                , public nsITabParent 
                , public nsIAuthPromptProvider
                , public nsISecureBrowserUI
                , public TabContext
{
    typedef mozilla::dom::ClonedMessageData ClonedMessageData;
    typedef mozilla::layout::ScrollingBehavior ScrollingBehavior;

public:
    TabParent(ContentParent* aManager, const TabContext& aContext);
    virtual ~TabParent();
    Element* GetOwnerElement() const { return mFrameElement; }
    void SetOwnerElement(Element* aElement);

    


    void GetAppType(nsAString& aOut);

    





    bool IsVisible();

    nsIBrowserDOMWindow *GetBrowserDOMWindow() { return mBrowserDOMWindow; }
    void SetBrowserDOMWindow(nsIBrowserDOMWindow* aBrowserDOMWindow) {
        mBrowserDOMWindow = aBrowserDOMWindow;
    }

    










    static TabParent* GetEventCapturer();
    









    bool TryCapture(const WidgetGUIEvent& aEvent);

    void Destroy();

    virtual bool RecvMoveFocus(const bool& aForward);
    virtual bool RecvEvent(const RemoteDOMEvent& aEvent);
    virtual bool RecvPRenderFrameConstructor(PRenderFrameParent* actor,
                                             ScrollingBehavior* scrolling,
                                             TextureFactoryIdentifier* identifier,
                                             uint64_t* layersId);
    virtual bool RecvBrowserFrameOpenWindow(PBrowserParent* aOpener,
                                            const nsString& aURL,
                                            const nsString& aName,
                                            const nsString& aFeatures,
                                            bool* aOutWindowOpened);
    virtual bool AnswerCreateWindow(PBrowserParent** retval);
    virtual bool RecvSyncMessage(const nsString& aMessage,
                                 const ClonedMessageData& aData,
                                 const InfallibleTArray<CpowEntry>& aCpows,
                                 InfallibleTArray<nsString>* aJSONRetVal);
    virtual bool AnswerRpcMessage(const nsString& aMessage,
                                  const ClonedMessageData& aData,
                                  const InfallibleTArray<CpowEntry>& aCpows,
                                  InfallibleTArray<nsString>* aJSONRetVal);
    virtual bool RecvAsyncMessage(const nsString& aMessage,
                                  const ClonedMessageData& aData,
                                  const InfallibleTArray<CpowEntry>& aCpows);
    virtual bool RecvNotifyIMEFocus(const bool& aFocus,
                                    nsIMEUpdatePreference* aPreference,
                                    uint32_t* aSeqno);
    virtual bool RecvNotifyIMETextChange(const uint32_t& aStart,
                                         const uint32_t& aEnd,
                                         const uint32_t& aNewEnd);
    virtual bool RecvNotifyIMESelection(const uint32_t& aSeqno,
                                        const uint32_t& aAnchor,
                                        const uint32_t& aFocus);
    virtual bool RecvNotifyIMETextHint(const nsString& aText);
    virtual bool RecvEndIMEComposition(const bool& aCancel,
                                       nsString* aComposition);
    virtual bool RecvGetInputContext(int32_t* aIMEEnabled,
                                     int32_t* aIMEOpen,
                                     intptr_t* aNativeIMEContext);
    virtual bool RecvSetInputContext(const int32_t& aIMEEnabled,
                                     const int32_t& aIMEOpen,
                                     const nsString& aType,
                                     const nsString& aInputmode,
                                     const nsString& aActionHint,
                                     const int32_t& aCause,
                                     const int32_t& aFocusChange);
    virtual bool RecvRequestFocus(const bool& aCanRaise);
    virtual bool RecvSetCursor(const uint32_t& aValue);
    virtual bool RecvSetBackgroundColor(const nscolor& aValue);
    virtual bool RecvSetStatus(const uint32_t& aType, const nsString& aStatus);
    virtual bool RecvGetDPI(float* aValue);
    virtual bool RecvGetDefaultScale(double* aValue);
    virtual bool RecvGetWidgetNativeData(WindowsHandle* aValue);
    virtual bool RecvZoomToRect(const CSSRect& aRect);
    virtual bool RecvUpdateZoomConstraints(const bool& aAllowZoom,
                                           const CSSToScreenScale& aMinZoom,
                                           const CSSToScreenScale& aMaxZoom);
    virtual bool RecvUpdateScrollOffset(const uint32_t& aPresShellId, const ViewID& aViewId, const CSSIntPoint& aScrollOffset);
    virtual bool RecvContentReceivedTouch(const bool& aPreventDefault);
    virtual PContentDialogParent* AllocPContentDialogParent(const uint32_t& aType,
                                                            const nsCString& aName,
                                                            const nsCString& aFeatures,
                                                            const InfallibleTArray<int>& aIntParams,
                                                            const InfallibleTArray<nsString>& aStringParams);
    virtual bool DeallocPContentDialogParent(PContentDialogParent* aDialog)
    {
      delete aDialog;
      return true;
    }


    void LoadURL(nsIURI* aURI);
    
    
    
    void Show(const nsIntSize& size);
    void UpdateDimensions(const nsRect& rect, const nsIntSize& size);
    void UpdateFrame(const layers::FrameMetrics& aFrameMetrics);
    void HandleDoubleTap(const CSSIntPoint& aPoint);
    void HandleSingleTap(const CSSIntPoint& aPoint);
    void HandleLongTap(const CSSIntPoint& aPoint);
    void Activate();
    void Deactivate();

    bool MapEventCoordinatesForChildProcess(nsEvent* aEvent);
    void MapEventCoordinatesForChildProcess(const LayoutDeviceIntPoint& aOffset,
                                                   nsEvent* aEvent);

    void SendMouseEvent(const nsAString& aType, float aX, float aY,
                        int32_t aButton, int32_t aClickCount,
                        int32_t aModifiers, bool aIgnoreRootScrollFrame);
    void SendKeyEvent(const nsAString& aType, int32_t aKeyCode,
                      int32_t aCharCode, int32_t aModifiers,
                      bool aPreventDefault);
    bool SendRealMouseEvent(nsMouseEvent& event);
    bool SendMouseWheelEvent(mozilla::WheelEvent& event);
    bool SendRealKeyEvent(mozilla::WidgetKeyboardEvent& event);
    bool SendRealTouchEvent(WidgetTouchEvent& event);

    virtual PDocumentRendererParent*
    AllocPDocumentRendererParent(const nsRect& documentRect, const gfxMatrix& transform,
                                 const nsString& bgcolor,
                                 const uint32_t& renderFlags, const bool& flushLayout,
                                 const nsIntSize& renderSize);
    virtual bool DeallocPDocumentRendererParent(PDocumentRendererParent* actor);

    virtual PContentPermissionRequestParent*
    AllocPContentPermissionRequestParent(const nsCString& aType, const nsCString& aAccess, const IPC::Principal& aPrincipal);
    virtual bool DeallocPContentPermissionRequestParent(PContentPermissionRequestParent* actor);

    virtual POfflineCacheUpdateParent* AllocPOfflineCacheUpdateParent(
            const URIParams& aManifestURI,
            const URIParams& aDocumentURI,
            const bool& stickDocument) MOZ_OVERRIDE;
    virtual bool DeallocPOfflineCacheUpdateParent(POfflineCacheUpdateParent* actor);

    bool GetGlobalJSObject(JSContext* cx, JSObject** globalp);

    NS_DECL_ISUPPORTS
    NS_DECL_NSIAUTHPROMPTPROVIDER
    NS_DECL_NSISECUREBROWSERUI

    void HandleDelayedDialogs();

    static TabParent *GetIMETabParent() { return mIMETabParent; }
    bool HandleQueryContentEvent(mozilla::WidgetQueryContentEvent& aEvent);
    bool SendCompositionEvent(mozilla::WidgetCompositionEvent& event);
    bool SendTextEvent(mozilla::WidgetTextEvent& event);
    bool SendSelectionEvent(mozilla::WidgetSelectionEvent& event);

    static TabParent* GetFrom(nsFrameLoader* aFrameLoader);
    static TabParent* GetFrom(nsIContent* aContent);

    ContentParent* Manager() { return mManager; }

protected:
    bool ReceiveMessage(const nsString& aMessage,
                        bool aSync,
                        const StructuredCloneData* aCloneData,
                        CpowHolder* aCpows,
                        InfallibleTArray<nsString>* aJSONRetVal = nullptr);

    virtual bool Recv__delete__() MOZ_OVERRIDE;

    virtual void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;

    virtual PIndexedDBParent* AllocPIndexedDBParent(
                                                  const nsCString& aGroup,
                                                  const nsCString& aASCIIOrigin,
                                                  bool* );

    virtual bool DeallocPIndexedDBParent(PIndexedDBParent* aActor);

    virtual bool
    RecvPIndexedDBConstructor(PIndexedDBParent* aActor,
                              const nsCString& aGroup,
                              const nsCString& aASCIIOrigin,
                              bool* aAllowed);

    Element* mFrameElement;
    nsCOMPtr<nsIBrowserDOMWindow> mBrowserDOMWindow;

    struct DelayedDialogData
    {
      DelayedDialogData(PContentDialogParent* aDialog, uint32_t aType,
                        const nsCString& aName,
                        const nsCString& aFeatures,
                        nsIDialogParamBlock* aParams)
      : mDialog(aDialog), mType(aType), mName(aName), mFeatures(aFeatures),
        mParams(aParams) {}

      PContentDialogParent* mDialog;
      uint32_t mType;
      nsCString mName;
      nsCString mFeatures;
      nsCOMPtr<nsIDialogParamBlock> mParams;
    };
    InfallibleTArray<DelayedDialogData*> mDelayedDialogs;

    bool ShouldDelayDialogs();
    bool AllowContentIME();

    virtual PRenderFrameParent* AllocPRenderFrameParent(ScrollingBehavior* aScrolling,
                                                        TextureFactoryIdentifier* aTextureFactoryIdentifier,
                                                        uint64_t* aLayersId) MOZ_OVERRIDE;
    virtual bool DeallocPRenderFrameParent(PRenderFrameParent* aFrame) MOZ_OVERRIDE;

    
    static TabParent *mIMETabParent;
    nsString mIMECacheText;
    uint32_t mIMESelectionAnchor;
    uint32_t mIMESelectionFocus;
    bool mIMEComposing;
    bool mIMECompositionEnding;
    
    
    nsAutoString mIMECompositionText;
    uint32_t mIMECompositionStart;
    uint32_t mIMESeqno;

    
    int32_t mEventCaptureDepth;

    nsRect mRect;
    nsIntSize mDimensions;
    ScreenOrientation mOrientation;
    float mDPI;
    CSSToLayoutDeviceScale mDefaultScale;
    bool mShown;
    bool mUpdatedDimensions;

private:
    already_AddRefed<nsFrameLoader> GetFrameLoader() const;
    already_AddRefed<nsIWidget> GetWidget() const;
    layout::RenderFrameParent* GetRenderFrame();
    nsRefPtr<ContentParent> mManager;
    void TryCacheDPIAndScale();

    
    
    bool UseAsyncPanZoom();
    
    
    
    
    void MaybeForwardEventToRenderFrame(const WidgetInputEvent& aEvent,
                                        WidgetInputEvent* aOutEvent);
    
    
    
    
    LayoutDeviceIntPoint mChildProcessOffsetAtTouchStart;
    
    
    bool mMarkedDestroying;
    
    
    bool mIsDestroyed;
    
    bool mAppPackageFileDescriptorSent;
};

} 
} 

#endif
