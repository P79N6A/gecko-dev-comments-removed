




































#include "nsPLDOMEvent.h"
#include "nsIDOMEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIDOMDocument.h"
#include "nsIDOMEventTarget.h"
#include "nsContentUtils.h"
#include "nsEventDispatcher.h"
#include "nsGUIEvent.h"

nsPLDOMEvent::nsPLDOMEvent(nsINode *aEventNode, nsEvent &aEvent)
  : mEventNode(aEventNode), mDispatchChromeOnly(false)
{
  bool trusted = NS_IS_TRUSTED_EVENT(&aEvent);
  nsEventDispatcher::CreateEvent(nsnull, &aEvent, EmptyString(),
                                 getter_AddRefs(mEvent));
  NS_ASSERTION(mEvent, "Should never fail to create an event");
  nsCOMPtr<nsIPrivateDOMEvent> priv = do_QueryInterface(mEvent);
  NS_ASSERTION(priv, "Should also not fail to QI to nsIDOMEventPrivate");
  priv->DuplicatePrivateData();
  priv->SetTrusted(trusted);
}

NS_IMETHODIMP nsPLDOMEvent::Run()
{
  if (!mEventNode) {
    return NS_OK;
  }

  if (mEvent) {
    NS_ASSERTION(!mDispatchChromeOnly, "Can't do that");
    nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(mEventNode);
    bool defaultActionEnabled; 
    target->DispatchEvent(mEvent, &defaultActionEnabled);
  } else {
    nsIDocument* doc = mEventNode->OwnerDoc();
    if (mDispatchChromeOnly) {
      nsContentUtils::DispatchChromeEvent(doc, mEventNode, mEventType,
                                          mBubbles, false);
    } else {
      nsContentUtils::DispatchTrustedEvent(doc, mEventNode, mEventType,
                                           mBubbles, false);
    }
  }

  return NS_OK;
}

nsresult nsPLDOMEvent::PostDOMEvent()
{
  return NS_DispatchToCurrentThread(this);
}

void nsPLDOMEvent::RunDOMEventWhenSafe()
{
  nsContentUtils::AddScriptRunner(this);
}

nsLoadBlockingPLDOMEvent::~nsLoadBlockingPLDOMEvent()
{
  if (mBlockedDoc) {
    mBlockedDoc->UnblockOnload(true);
  }
}
