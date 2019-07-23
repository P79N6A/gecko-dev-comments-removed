





































#include "TabParent.h"

#include "mozilla/ipc/GeckoThread.h"
#include "mozilla/ipc/DocumentRendererParent.h"

#include "nsIURI.h"
#include "nsFocusManager.h"
#include "nsCOMPtr.h"
#include "nsServiceManagerUtils.h"
#include "nsIDOMElement.h"
#include "nsEventDispatcher.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMEvent.h"
#include "nsIPrivateDOMEvent.h"

using mozilla::ipc::BrowserProcessSubThread;
using mozilla::ipc::DocumentRendererParent;

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

void
TabParent::SendMouseEvent(const nsAString& aType, float aX, float aY,
                          PRInt32 aButton, PRInt32 aClickCount,
                          PRInt32 aModifiers, PRBool aIgnoreRootScrollFrame)
{
  SendsendMouseEvent(nsString(aType), aX, aY, aButton, aClickCount,
                     aModifiers, aIgnoreRootScrollFrame);
}

} 
} 
