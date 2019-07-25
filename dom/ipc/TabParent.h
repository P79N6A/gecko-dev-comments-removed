





































#ifndef mozilla_tabs_TabParent_h
#define mozilla_tabs_TabParent_h

#include "mozilla/dom/PBrowserParent.h"
#include "mozilla/dom/PContentDialogParent.h"
#include "mozilla/ipc/GeckoChildProcessHost.h"

#include "jsapi.h"
#include "nsCOMPtr.h"
#include "nsITabParent.h"
#include "nsIBrowserDOMWindow.h"
#include "nsWeakReference.h"
#include "nsIDialogParamBlock.h"
#include "nsIAuthPromptProvider.h"
#include "nsISSLStatusProvider.h"
#include "nsISecureBrowserUI.h"

class nsFrameLoader;
class nsIURI;
class nsIDOMElement;
struct gfxMatrix;

struct JSContext;
struct JSObject;

namespace mozilla {
namespace dom {

class ContentDialogParent : public PContentDialogParent {};

class TabParent : public PBrowserParent 
                , public nsITabParent 
                , public nsIAuthPromptProvider
                , public nsISecureBrowserUI
                , public nsISSLStatusProvider
{
public:
    TabParent();
    virtual ~TabParent();
    nsIDOMElement* GetOwnerElement() { return mFrameElement; }
    void SetOwnerElement(nsIDOMElement* aElement) { mFrameElement = aElement; }
    nsIBrowserDOMWindow *GetBrowserDOMWindow() { return mBrowserDOMWindow; }
    void SetBrowserDOMWindow(nsIBrowserDOMWindow* aBrowserDOMWindow) {
        mBrowserDOMWindow = aBrowserDOMWindow;
    }
 
    virtual bool RecvMoveFocus(const bool& aForward);
    virtual bool RecvEvent(const RemoteDOMEvent& aEvent);

    virtual bool AnswerCreateWindow(PBrowserParent** retval);
    virtual bool RecvSyncMessage(const nsString& aMessage,
                                 const nsString& aJSON,
                                 nsTArray<nsString>* aJSONRetVal);
    virtual bool RecvAsyncMessage(const nsString& aMessage,
                                  const nsString& aJSON);
    virtual bool RecvNotifyIMEFocus(const PRBool& aFocus,
                                    nsIMEUpdatePreference* aPreference,
                                    PRUint32* aSeqno);
    virtual bool RecvNotifyIMETextChange(const PRUint32& aStart,
                                         const PRUint32& aEnd,
                                         const PRUint32& aNewEnd);
    virtual bool RecvNotifyIMESelection(const PRUint32& aSeqno,
                                        const PRUint32& aAnchor,
                                        const PRUint32& aFocus);
    virtual bool RecvNotifyIMETextHint(const nsString& aText);
    virtual bool RecvEndIMEComposition(const PRBool& aCancel,
                                       nsString* aComposition);
    virtual bool RecvGetIMEEnabled(PRUint32* aValue);
    virtual bool RecvSetIMEEnabled(const PRUint32& aValue);
    virtual bool RecvGetIMEOpenState(PRBool* aValue);
    virtual bool RecvSetIMEOpenState(const PRBool& aValue);
    virtual PContentDialogParent* AllocPContentDialog(const PRUint32& aType,
                                                      const nsCString& aName,
                                                      const nsCString& aFeatures,
                                                      const nsTArray<int>& aIntParams,
                                                      const nsTArray<nsString>& aStringParams);
    virtual bool DeallocPContentDialog(PContentDialogParent* aDialog)
    {
      delete aDialog;
      return true;
    }

    void LoadURL(nsIURI* aURI);
    
    
    
    void Show(const nsIntSize& size);
    void Move(const nsIntSize& size);
    void Activate();
    void SendMouseEvent(const nsAString& aType, float aX, float aY,
                        PRInt32 aButton, PRInt32 aClickCount,
                        PRInt32 aModifiers, PRBool aIgnoreRootScrollFrame);
    void SendKeyEvent(const nsAString& aType, PRInt32 aKeyCode,
                      PRInt32 aCharCode, PRInt32 aModifiers,
                      PRBool aPreventDefault);

    virtual mozilla::ipc::PDocumentRendererParent* AllocPDocumentRenderer(
            const PRInt32& x,
            const PRInt32& y,
            const PRInt32& w,
            const PRInt32& h,
            const nsString& bgcolor,
            const PRUint32& flags,
            const bool& flush);
    virtual bool DeallocPDocumentRenderer(PDocumentRendererParent* actor);

    virtual mozilla::ipc::PDocumentRendererShmemParent* AllocPDocumentRendererShmem(
            const PRInt32& x,
            const PRInt32& y,
            const PRInt32& w,
            const PRInt32& h,
            const nsString& bgcolor,
            const PRUint32& flags,
            const bool& flush,
            const gfxMatrix& aMatrix,
            Shmem& buf);
    virtual bool DeallocPDocumentRendererShmem(PDocumentRendererShmemParent* actor);

    virtual mozilla::ipc::PDocumentRendererNativeIDParent* AllocPDocumentRendererNativeID(
            const PRInt32& x,
            const PRInt32& y,
            const PRInt32& w,
            const PRInt32& h,
            const nsString& bgcolor,
            const PRUint32& flags,
            const bool& flush,
            const gfxMatrix& aMatrix,
            const PRUint32& nativeID);
    virtual bool DeallocPDocumentRendererNativeID(PDocumentRendererNativeIDParent* actor);


    virtual PContentPermissionRequestParent* AllocPContentPermissionRequest(const nsCString& aType, const IPC::URI& uri);
    virtual bool DeallocPContentPermissionRequest(PContentPermissionRequestParent* actor);

    JSBool GetGlobalJSObject(JSContext* cx, JSObject** globalp);

    NS_DECL_ISUPPORTS
    NS_DECL_NSIAUTHPROMPTPROVIDER
    NS_DECL_NSISECUREBROWSERUI
    NS_DECL_NSISSLSTATUSPROVIDER

    void HandleDelayedDialogs();

    static TabParent *GetIMETabParent() { return mIMETabParent; }
    bool HandleQueryContentEvent(nsQueryContentEvent& aEvent);
    bool SendCompositionEvent(nsCompositionEvent& event);
    bool SendTextEvent(nsTextEvent& event);
    bool SendSelectionEvent(nsSelectionEvent& event);
protected:
    bool ReceiveMessage(const nsString& aMessage,
                        PRBool aSync,
                        const nsString& aJSON,
                        nsTArray<nsString>* aJSONRetVal = nsnull);

    void ActorDestroy(ActorDestroyReason why);

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
    nsTArray<DelayedDialogData*> mDelayedDialogs;

    PRBool ShouldDelayDialogs();

    NS_OVERRIDE
    virtual PRenderFrameParent* AllocPRenderFrame();
    NS_OVERRIDE
    virtual bool DeallocPRenderFrame(PRenderFrameParent* aFrame);

    PRUint32 mSecurityState;
    nsString mSecurityTooltipText;
    nsCOMPtr<nsISupports> mSecurityStatusObject;

    
    static TabParent *mIMETabParent;
    nsString mIMECacheText;
    PRUint32 mIMESelectionAnchor;
    PRUint32 mIMESelectionFocus;
    PRPackedBool mIMEComposing;
    PRPackedBool mIMECompositionEnding;
    
    
    nsAutoString mIMECompositionText;
    PRUint32 mIMECompositionStart;
    PRUint32 mIMESeqno;

private:
    already_AddRefed<nsFrameLoader> GetFrameLoader() const;
    already_AddRefed<nsIWidget> GetWidget() const;
};

} 
} 

#endif
