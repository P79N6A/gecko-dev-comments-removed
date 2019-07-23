




































#ifndef nsPLDOMEvent_h___
#define nsPLDOMEvent_h___

#include "nsCOMPtr.h"
#include "nsThreadUtils.h"
#include "nsIDOMNode.h"
#include "nsString.h"











 
class nsPLDOMEvent : public nsRunnable {
public:
  nsPLDOMEvent (nsIDOMNode *aEventNode, const nsAString& aEventType)
    : mEventNode(aEventNode), mEventType(aEventType)
  { }

  nsPLDOMEvent(nsIDOMNode *aEventNode, nsIDOMEvent *aEvent)
    : mEventNode(aEventNode), mEvent(aEvent)
  { }

  NS_IMETHOD Run();
  nsresult PostDOMEvent();
  nsresult RunDOMEventWhenSafe();

  nsCOMPtr<nsIDOMNode>  mEventNode;
  nsCOMPtr<nsIDOMEvent> mEvent;
  nsString              mEventType;
};

#endif
