





































#include "TabParent.h"

#include "mozilla/ipc/DocumentRendererParent.h"
#include "mozilla/ipc/DocumentRendererShmemParent.h"
#include "mozilla/jsipc/ContextWrapperParent.h"

#include "nsIURI.h"
#include "nsFocusManager.h"
#include "nsCOMPtr.h"
#include "nsServiceManagerUtils.h"
#include "nsIDOMElement.h"
#include "nsEventDispatcher.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "nsFrameLoader.h"

using mozilla::ipc::DocumentRendererParent;
using mozilla::ipc::DocumentRendererShmemParent;
using mozilla::jsipc::PContextWrapperParent;
using mozilla::jsipc::ContextWrapperParent;

namespace mozilla {
namespace dom {

TabParent::TabParent()
{
}

TabParent::~TabParent()
{
}

bool
TabParent::RecvmoveFocus(const bool& aForward)
{
  nsCOMPtr<nsIFocusManager> fm = do_GetService(FOCUSMANAGER_CONTRACTID);
  if (fm) {
    nsCOMPtr<nsIDOMElement> dummy;
    PRUint32 type = aForward ? nsIFocusManager::MOVEFOCUS_FORWARD
                             : nsIFocusManager::MOVEFOCUS_BACKWARD;
    fm->MoveFocus(nsnull, mFrameElement, type, nsIFocusManager::FLAG_BYKEY, 
                  getter_AddRefs(dummy));
  }
  return true;
}

bool
TabParent::RecvsendEvent(const RemoteDOMEvent& aEvent)
{
  nsCOMPtr<nsIDOMEvent> event = do_QueryInterface(aEvent.mEvent);
  NS_ENSURE_TRUE(event, true);

  nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(mFrameElement);
  NS_ENSURE_TRUE(target, true);

  PRBool dummy;
  target->DispatchEvent(event, &dummy);
  return true;
}

bool
TabParent::AnswercreateWindow(PIFrameEmbeddingParent** retval)
{
    if (!mBrowserDOMWindow) {
        return false;
    }

    
    
    nsCOMPtr<nsIFrameLoaderOwner> frameLoaderOwner;
    mBrowserDOMWindow->OpenURIInFrame(nsnull, nsnull,
                                      nsIBrowserDOMWindow::OPEN_NEWTAB,
                                      nsIBrowserDOMWindow::OPEN_NEW,
                                      getter_AddRefs(frameLoaderOwner));
    if (!frameLoaderOwner) {
        return false;
    }

    nsRefPtr<nsFrameLoader> frameLoader = frameLoaderOwner->GetFrameLoader();
    if (!frameLoader) {
        return false;
    }

    *retval = frameLoader->GetChildProcess();
    return true;
}

void
TabParent::LoadURL(nsIURI* aURI)
{
    nsCString spec;
    aURI->GetSpec(spec);

    SendloadURL(spec);
}

void
TabParent::Move(PRUint32 x, PRUint32 y, PRUint32 width, PRUint32 height)
{
    Sendmove(x, y, width, height);
}

void
TabParent::Activate()
{
    Sendactivate();
}

mozilla::ipc::PDocumentRendererParent*
TabParent::AllocPDocumentRenderer(const PRInt32& x,
        const PRInt32& y, const PRInt32& w, const PRInt32& h, const nsString& bgcolor,
        const PRUint32& flags, const bool& flush)
{
    return new DocumentRendererParent();
}

bool
TabParent::DeallocPDocumentRenderer(PDocumentRendererParent* actor)
{
    delete actor;
    return true;
}

mozilla::ipc::PDocumentRendererShmemParent*
TabParent::AllocPDocumentRendererShmem(const PRInt32& x,
        const PRInt32& y, const PRInt32& w, const PRInt32& h, const nsString& bgcolor,
        const PRUint32& flags, const bool& flush, const gfxMatrix& aMatrix,
        const PRInt32& bufw, const PRInt32& bufh, Shmem &buf)
{
    return new DocumentRendererShmemParent();
}

bool
TabParent::DeallocPDocumentRendererShmem(PDocumentRendererShmemParent* actor)
{
    delete actor;
    return true;
}

PContextWrapperParent*
TabParent::AllocPContextWrapper()
{
    return new ContextWrapperParent();
}

bool
TabParent::DeallocPContextWrapper(PContextWrapperParent* actor)
{
    delete actor;
    return true;
}

bool
TabParent::GetGlobalJSObject(JSContext* cx, JSObject** globalp)
{
    
    nsTArray<PContextWrapperParent*> cwps(1);
    ManagedPContextWrapperParent(cwps);
    if (cwps.Length() < 1)
        return false;
    NS_ASSERTION(cwps.Length() == 1, "More than one PContextWrapper?");
    ContextWrapperParent* cwp = static_cast<ContextWrapperParent*>(cwps[0]);
    return (cwp->GetGlobalJSObject(cx, globalp));
}

void
TabParent::SendMouseEvent(const nsAString& aType, float aX, float aY,
                          PRInt32 aButton, PRInt32 aClickCount,
                          PRInt32 aModifiers, PRBool aIgnoreRootScrollFrame)
{
  SendsendMouseEvent(nsString(aType), aX, aY, aButton, aClickCount,
                     aModifiers, aIgnoreRootScrollFrame);
}

void
TabParent::SendKeyEvent(const nsAString& aType,
                        PRInt32 aKeyCode,
                        PRInt32 aCharCode,
                        PRInt32 aModifiers,
                        PRBool aPreventDefault)
{
    SendsendKeyEvent(nsString(aType), aKeyCode, aCharCode, aModifiers,
                     aPreventDefault);
}

bool
TabParent::RecvsendSyncMessageToParent(const nsString& aMessage,
                                       const nsString& aJSON,
                                       nsTArray<nsString>* aJSONRetVal)
{
  nsCOMPtr<nsIFrameLoaderOwner> frameLoaderOwner =
    do_QueryInterface(mFrameElement);
  if (frameLoaderOwner) {
    nsRefPtr<nsFrameLoader> frameLoader = frameLoaderOwner->GetFrameLoader();
    if (frameLoader && frameLoader->GetFrameMessageManager()) {
      frameLoader->GetFrameMessageManager()->ReceiveMessage(mFrameElement,
                                                            aMessage,
                                                            PR_TRUE,
                                                            aJSON,
                                                            aJSONRetVal);
    }
  }
  return true;
}

bool
TabParent::RecvsendAsyncMessageToParent(const nsString& aMessage,
                                        const nsString& aJSON)
{
  nsCOMPtr<nsIFrameLoaderOwner> frameLoaderOwner =
    do_QueryInterface(mFrameElement);
  if (frameLoaderOwner) {
    nsRefPtr<nsFrameLoader> frameLoader = frameLoaderOwner->GetFrameLoader();
    if (frameLoader && frameLoader->GetFrameMessageManager()) {
      nsTArray<nsString> dummy;
      frameLoader->GetFrameMessageManager()->ReceiveMessage(mFrameElement,
                                                            aMessage,
                                                            PR_FALSE,
                                                            aJSON,
                                                            nsnull);
    }
  }
  return true;
}

} 
} 
