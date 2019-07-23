




































#include "nsPLDOMEvent.h"
#include "nsIDOMEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentEvent.h"
#include "nsIDOMEventTarget.h"

NS_IMETHODIMP nsPLDOMEvent::Run()
{
  if (!mEventNode) {
    return NS_OK;
  }
  
  nsCOMPtr<nsIDOMDocument> domDoc;
  mEventNode->GetOwnerDocument(getter_AddRefs(domDoc));
  nsCOMPtr<nsIDOMDocumentEvent> domEventDoc = do_QueryInterface(domDoc);
  if (domEventDoc) {
    nsCOMPtr<nsIDOMEvent> domEvent;
    domEventDoc->CreateEvent(NS_LITERAL_STRING("Events"),
                        getter_AddRefs(domEvent));

    nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(domEvent));
    if (privateEvent && NS_SUCCEEDED(domEvent->InitEvent(mEventType, PR_TRUE, PR_TRUE))) {
      privateEvent->SetTrusted(PR_TRUE);

      nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(mEventNode);
      PRBool defaultActionEnabled; 
      target->DispatchEvent(domEvent, &defaultActionEnabled);
    }
  }
  
  return NS_OK;
}

nsresult nsPLDOMEvent::PostDOMEvent()
{
  return NS_DispatchToCurrentThread(this);
}
