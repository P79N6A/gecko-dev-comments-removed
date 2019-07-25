





































#ifndef mozilla_tabs_TabParent_h
#define mozilla_tabs_TabParent_h

#include "mozilla/dom/PIFrameEmbeddingParent.h"

#include "mozilla/ipc/GeckoChildProcessHost.h"

#include "nsCOMPtr.h"
#include "nsIBrowserDOMWindow.h"

class nsIURI;
class nsIDOMElement;
class gfxMatrix;

struct JSContext;
struct JSObject;

namespace mozilla {

namespace jsipc {
class PContextWrapperParent;
}

namespace dom {

class TabParent : public PIFrameEmbeddingParent
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
    virtual bool AnswercreateWindow(PIFrameEmbeddingParent** retval);
    virtual bool RecvsendSyncMessageToParent(const nsString& aMessage,
                                             const nsString& aJSON,
                                             nsTArray<nsString>* aJSONRetVal);
    virtual bool RecvsendAsyncMessageToParent(const nsString& aMessage,
                                              const nsString& aJSON);

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
            const PRInt32& bufw,
            const PRInt32& bufh,
            Shmem& buf);
    virtual bool DeallocPDocumentRendererShmem(PDocumentRendererShmemParent* actor);

    virtual PContextWrapperParent* AllocPContextWrapper();
    virtual bool DeallocPContextWrapper(PContextWrapperParent* actor);

    bool GetGlobalJSObject(JSContext* cx, JSObject** globalp);

protected:
    nsIDOMElement* mFrameElement;
    nsCOMPtr<nsIBrowserDOMWindow> mBrowserDOMWindow;
};

} 
} 

#endif
