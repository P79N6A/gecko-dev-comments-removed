





































#ifndef mozilla_tabs_TabParent_h
#define mozilla_tabs_TabParent_h

#include "mozilla/dom/PIFrameEmbeddingParent.h"
#include "mozilla/dom/PContentDialogParent.h"
#include "mozilla/ipc/GeckoChildProcessHost.h"

#include "jsapi.h"
#include "nsCOMPtr.h"
#include "nsITabParent.h"
#include "nsIBrowserDOMWindow.h"
#include "nsIWebProgress.h"
#include "nsIWebProgressListener.h"
#include "nsWeakReference.h"
#include "nsIDialogParamBlock.h"

class nsIURI;
class nsIDOMElement;
class gfxMatrix;

struct JSContext;
struct JSObject;

namespace mozilla {
namespace dom {
struct TabParentListenerInfo 
{
  TabParentListenerInfo(nsIWeakReference *aListener, unsigned long aNotifyMask)
    : mWeakListener(aListener), mNotifyMask(aNotifyMask)
  {
  }

  TabParentListenerInfo(const TabParentListenerInfo& obj)
    : mWeakListener(obj.mWeakListener), mNotifyMask(obj.mNotifyMask) 
  {
  }

  nsWeakPtr mWeakListener;

  PRUint32 mNotifyMask;
};

inline    
bool operator==(const TabParentListenerInfo& lhs, const TabParentListenerInfo& rhs)
{
  return &lhs == &rhs;
}

class ContentDialogParent : public PContentDialogParent {};

class TabParent : public PIFrameEmbeddingParent, public nsITabParent, public nsIWebProgress
{
public:
    TabParent();
    virtual ~TabParent();
    void SetOwnerElement(nsIDOMElement* aElement) { mFrameElement = aElement; }
    void SetBrowserDOMWindow(nsIBrowserDOMWindow* aBrowserDOMWindow) {
        mBrowserDOMWindow = aBrowserDOMWindow;
    }

    virtual bool RecvmoveFocus(const bool& aForward);
    virtual bool RecvsendEvent(const RemoteDOMEvent& aEvent);
    virtual bool RecvnotifyProgressChange(const PRInt64& aProgress,
                                          const PRInt64& aProgressMax,
                                          const PRInt64& aTotalProgress,
                                          const PRInt64& aMaxTotalProgress);
    virtual bool RecvnotifyStateChange(const PRUint32& aStateFlags,
                                       const nsresult& aStatus);
    virtual bool RecvnotifyLocationChange(const nsCString& aUri);
    virtual bool RecvnotifyStatusChange(const nsresult& status,
                                        const nsString& message);
    virtual bool RecvnotifySecurityChange(const PRUint32& aState);
    virtual bool RecvrefreshAttempted(const nsCString& aURI,
                                      const PRInt32& aMillis,
                                      const bool& aSameURI,
                                      bool* aAllowRefresh);

    virtual bool AnswercreateWindow(PIFrameEmbeddingParent** retval);
    virtual bool RecvsendSyncMessageToParent(const nsString& aMessage,
                                             const nsString& aJSON,
                                             nsTArray<nsString>* aJSONRetVal);
    virtual bool RecvsendAsyncMessageToParent(const nsString& aMessage,
                                              const nsString& aJSON);
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
    void Move(PRUint32 x, PRUint32 y, PRUint32 width, PRUint32 height);
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


    virtual PGeolocationRequestParent* AllocPGeolocationRequest(const IPC::URI& uri);
    virtual bool DeallocPGeolocationRequest(PGeolocationRequestParent* actor);

    JSBool GetGlobalJSObject(JSContext* cx, JSObject** globalp);

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBPROGRESS

    void HandleDelayedDialogs();
protected:
    bool ReceiveMessage(const nsString& aMessage,
                        PRBool aSync,
                        const nsString& aJSON,
                        nsTArray<nsString>* aJSONRetVal = nsnull);

    TabParentListenerInfo* GetListenerInfo(nsIWebProgressListener *aListener);

    void ActorDestroy(ActorDestroyReason why);

    nsIDOMElement* mFrameElement;
    nsCOMPtr<nsIBrowserDOMWindow> mBrowserDOMWindow;

    nsTArray<TabParentListenerInfo> mListenerInfoList;

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
};

} 
} 

#endif
