





#ifndef mozilla_tabs_TabParent_h
#define mozilla_tabs_TabParent_h

#include "base/basictypes.h"

#include "jsapi.h"
#include "mozilla/dom/PBrowserParent.h"
#include "mozilla/dom/PContentDialogParent.h"
#include "mozilla/ipc/GeckoChildProcessHost.h"
#include "nsCOMPtr.h"
#include "nsIAuthPromptProvider.h"
#include "nsIBrowserDOMWindow.h"
#include "nsIDialogParamBlock.h"
#include "nsISecureBrowserUI.h"
#include "nsITabParent.h"
#include "nsWeakReference.h"

struct gfxMatrix;
struct JSContext;
struct JSObject;
class mozIApplication;
class nsFrameLoader;
class nsIDOMElement;
class nsIURI;

namespace mozilla {

namespace layers {
struct FrameMetrics;
}

namespace layout {
class RenderFrameParent;
}

namespace dom {

class ClonedMessageData;
struct StructuredCloneData;

class ContentDialogParent : public PContentDialogParent {};

class TabParent : public PBrowserParent 
                , public nsITabParent 
                , public nsIAuthPromptProvider
                , public nsISecureBrowserUI
{
    typedef mozilla::dom::ClonedMessageData ClonedMessageData;

public:
    TabParent(mozIApplication* aApp, bool aIsBrowserElement);
    virtual ~TabParent();
    nsIDOMElement* GetOwnerElement() { return mFrameElement; }
    void SetOwnerElement(nsIDOMElement* aElement);
    nsIBrowserDOMWindow *GetBrowserDOMWindow() { return mBrowserDOMWindow; }
    void SetBrowserDOMWindow(nsIBrowserDOMWindow* aBrowserDOMWindow) {
        mBrowserDOMWindow = aBrowserDOMWindow;
    }
 
    mozIApplication* GetApp() { return mApp; }
    bool IsBrowserElement() { return mIsBrowserElement; }

    void Destroy();

    virtual bool RecvMoveFocus(const bool& aForward);
    virtual bool RecvEvent(const RemoteDOMEvent& aEvent);
    virtual bool RecvBrowserFrameOpenWindow(PBrowserParent* aOpener,
                                            const nsString& aURL,
                                            const nsString& aName,
                                            const nsString& aFeatures,
                                            bool* aOutWindowOpened);
    virtual bool AnswerCreateWindow(PBrowserParent** retval);
    virtual bool RecvSyncMessage(const nsString& aMessage,
                                 const ClonedMessageData& aData,
                                 InfallibleTArray<nsString>* aJSONRetVal);
    virtual bool RecvAsyncMessage(const nsString& aMessage,
                                  const ClonedMessageData& aData);
    virtual bool RecvNotifyIMEFocus(const bool& aFocus,
                                    nsIMEUpdatePreference* aPreference,
                                    PRUint32* aSeqno);
    virtual bool RecvNotifyIMETextChange(const PRUint32& aStart,
                                         const PRUint32& aEnd,
                                         const PRUint32& aNewEnd);
    virtual bool RecvNotifyIMESelection(const PRUint32& aSeqno,
                                        const PRUint32& aAnchor,
                                        const PRUint32& aFocus);
    virtual bool RecvNotifyIMETextHint(const nsString& aText);
    virtual bool RecvEndIMEComposition(const bool& aCancel,
                                       nsString* aComposition);
    virtual bool RecvGetInputContext(PRInt32* aIMEEnabled,
                                     PRInt32* aIMEOpen);
    virtual bool RecvSetInputContext(const PRInt32& aIMEEnabled,
                                     const PRInt32& aIMEOpen,
                                     const nsString& aType,
                                     const nsString& aActionHint,
                                     const PRInt32& aCause,
                                     const PRInt32& aFocusChange);
    virtual bool RecvSetCursor(const PRUint32& aValue);
    virtual bool RecvSetBackgroundColor(const nscolor& aValue);
    virtual bool RecvGetDPI(float* aValue);
    virtual bool RecvGetWidgetNativeData(WindowsHandle* aValue);
    virtual bool RecvNotifyDOMTouchListenerAdded();
    virtual bool RecvZoomToRect(const gfxRect& aRect);
    virtual PContentDialogParent* AllocPContentDialog(const PRUint32& aType,
                                                      const nsCString& aName,
                                                      const nsCString& aFeatures,
                                                      const InfallibleTArray<int>& aIntParams,
                                                      const InfallibleTArray<nsString>& aStringParams);
    virtual bool DeallocPContentDialog(PContentDialogParent* aDialog)
    {
      delete aDialog;
      return true;
    }


    void LoadURL(nsIURI* aURI);
    
    
    
    void Show(const nsIntSize& size);
    void UpdateDimensions(const nsRect& rect, const nsIntSize& size);
    void UpdateFrame(const layers::FrameMetrics& aFrameMetrics);
    void HandleDoubleTap(const nsIntPoint& aPoint);
    void Activate();
    void Deactivate();

    



    bool Active();

    void SendMouseEvent(const nsAString& aType, float aX, float aY,
                        PRInt32 aButton, PRInt32 aClickCount,
                        PRInt32 aModifiers, bool aIgnoreRootScrollFrame);
    void SendKeyEvent(const nsAString& aType, PRInt32 aKeyCode,
                      PRInt32 aCharCode, PRInt32 aModifiers,
                      bool aPreventDefault);
    bool SendRealMouseEvent(nsMouseEvent& event);
    bool SendMouseScrollEvent(nsMouseScrollEvent& event);
    bool SendMouseWheelEvent(mozilla::widget::WheelEvent& event);
    bool SendRealKeyEvent(nsKeyEvent& event);
    bool SendRealTouchEvent(nsTouchEvent& event);

    virtual PDocumentRendererParent*
    AllocPDocumentRenderer(const nsRect& documentRect, const gfxMatrix& transform,
                           const nsString& bgcolor,
                           const PRUint32& renderFlags, const bool& flushLayout,
                           const nsIntSize& renderSize);
    virtual bool DeallocPDocumentRenderer(PDocumentRendererParent* actor);

    virtual PContentPermissionRequestParent*
    AllocPContentPermissionRequest(const nsCString& aType, const IPC::Principal& aPrincipal);
    virtual bool DeallocPContentPermissionRequest(PContentPermissionRequestParent* actor);

    virtual POfflineCacheUpdateParent* AllocPOfflineCacheUpdate(
            const URI& aManifestURI,
            const URI& aDocumentURI,
            const nsCString& aClientID,
            const bool& stickDocument);
    virtual bool DeallocPOfflineCacheUpdate(POfflineCacheUpdateParent* actor);

    JSBool GetGlobalJSObject(JSContext* cx, JSObject** globalp);

    NS_DECL_ISUPPORTS
    NS_DECL_NSIAUTHPROMPTPROVIDER
    NS_DECL_NSISECUREBROWSERUI

    void HandleDelayedDialogs();

    static TabParent *GetIMETabParent() { return mIMETabParent; }
    bool HandleQueryContentEvent(nsQueryContentEvent& aEvent);
    bool SendCompositionEvent(nsCompositionEvent& event);
    bool SendTextEvent(nsTextEvent& event);
    bool SendSelectionEvent(nsSelectionEvent& event);

    static TabParent* GetFrom(nsFrameLoader* aFrameLoader);
    static TabParent* GetFrom(nsIContent* aContent);

protected:
    bool ReceiveMessage(const nsString& aMessage,
                        bool aSync,
                        const StructuredCloneData* aCloneData,
                        InfallibleTArray<nsString>* aJSONRetVal = nullptr);

    virtual bool Recv__delete__() MOZ_OVERRIDE;

    virtual void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;

    virtual PIndexedDBParent* AllocPIndexedDB(const nsCString& aASCIIOrigin,
                                              bool* );

    virtual bool DeallocPIndexedDB(PIndexedDBParent* aActor);

    virtual bool
    RecvPIndexedDBConstructor(PIndexedDBParent* aActor,
                              const nsCString& aASCIIOrigin,
                              bool* aAllowed);

    nsIDOMElement* mFrameElement;
    nsCOMPtr<nsIBrowserDOMWindow> mBrowserDOMWindow;

    struct DelayedDialogData
    {
      DelayedDialogData(PContentDialogParent* aDialog, PRUint32 aType,
                        const nsCString& aName,
                        const nsCString& aFeatures,
                        nsIDialogParamBlock* aParams)
      : mDialog(aDialog), mType(aType), mName(aName), mFeatures(aFeatures),
        mParams(aParams) {}

      PContentDialogParent* mDialog;
      PRUint32 mType;
      nsCString mName;
      nsCString mFeatures;
      nsCOMPtr<nsIDialogParamBlock> mParams;
    };
    InfallibleTArray<DelayedDialogData*> mDelayedDialogs;

    bool ShouldDelayDialogs();
    bool AllowContentIME();

    virtual PRenderFrameParent* AllocPRenderFrame(ScrollingBehavior* aScrolling,
                                                  LayersBackend* aBackend,
                                                  int32_t* aMaxTextureSize,
                                                  uint64_t* aLayersId) MOZ_OVERRIDE;
    virtual bool DeallocPRenderFrame(PRenderFrameParent* aFrame) MOZ_OVERRIDE;

    nsCOMPtr<mozIApplication> mApp;
    
    static TabParent *mIMETabParent;
    nsString mIMECacheText;
    PRUint32 mIMESelectionAnchor;
    PRUint32 mIMESelectionFocus;
    bool mIMEComposing;
    bool mIMECompositionEnding;
    
    
    nsAutoString mIMECompositionText;
    PRUint32 mIMECompositionStart;
    PRUint32 mIMESeqno;

    float mDPI;
    bool mActive;
    bool mIsBrowserElement;
    bool mShown;

private:
    already_AddRefed<nsFrameLoader> GetFrameLoader() const;
    already_AddRefed<nsIWidget> GetWidget() const;
    layout::RenderFrameParent* GetRenderFrame();
    void TryCacheDPI();
    
    
    bool IsForMozBrowser();
    
    
    bool UseAsyncPanZoom();
    
    
    
    
    void MaybeForwardEventToRenderFrame(const nsInputEvent& aEvent,
                                        nsInputEvent* aOutEvent);
};

} 
} 

#endif
