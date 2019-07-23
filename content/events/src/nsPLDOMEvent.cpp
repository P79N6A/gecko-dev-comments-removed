




































#include "nsPLDOMEvent.h"
#include "nsIDOMEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsContentUtils.h"

NS_IMETHODIMP nsPLDOMEvent::Run()
{
  if (!mEventNode) {
    return NS_OK;
  }

  if (mEvent) {
    NS_ASSERTION(!mDispatchChromeOnly, "Can't do that");
    nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(mEventNode);
    PRBool defaultActionEnabled; 
    target->DispatchEvent(mEvent, &defaultActionEnabled);
  } else {
    nsIDocument* doc = mEventNode->GetOwnerDoc();
    if (doc) {
      if (mDispatchChromeOnly) {
        nsContentUtils::DispatchChromeEvent(doc, mEventNode, mEventType,
                                            mBubbles, PR_FALSE);
      } else {
        nsContentUtils::DispatchTrustedEvent(doc, mEventNode, mEventType,
                                             mBubbles, PR_FALSE);
      }
    }
  }

  return NS_OK;
}

nsresult nsPLDOMEvent::PostDOMEvent()
{
  return NS_DispatchToCurrentThread(this);
}

nsresult nsPLDOMEvent::RunDOMEventWhenSafe()
{
  return nsContentUtils::AddScriptRunner(this) ? NS_OK : NS_ERROR_FAILURE;
}

nsLoadBlockingPLDOMEvent::~nsLoadBlockingPLDOMEvent()
{
  if (mBlockedDoc) {
    mBlockedDoc->UnblockOnload(PR_TRUE);
  }
}
