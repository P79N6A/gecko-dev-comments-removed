




































#ifndef nsPLDOMEvent_h___
#define nsPLDOMEvent_h___

#include "nsCOMPtr.h"
#include "nsThreadUtils.h"
#include "nsINode.h"
#include "nsIDOMEvent.h"
#include "nsString.h"











 
class nsPLDOMEvent : public nsRunnable {
public:
  nsPLDOMEvent(nsINode *aEventNode, const nsAString& aEventType,
               PRBool aDispatchChromeOnly)
    : mEventNode(aEventNode), mEventType(aEventType),
      mDispatchChromeOnly(aDispatchChromeOnly)
  { }

  nsPLDOMEvent(nsINode *aEventNode, nsIDOMEvent *aEvent)
    : mEventNode(aEventNode), mEvent(aEvent), mDispatchChromeOnly(PR_FALSE)
  { }

  NS_IMETHOD Run();
  nsresult PostDOMEvent();
  nsresult RunDOMEventWhenSafe();

  nsCOMPtr<nsINode>     mEventNode;
  nsCOMPtr<nsIDOMEvent> mEvent;
  nsString              mEventType;
  PRBool                mDispatchChromeOnly;
};

#endif
