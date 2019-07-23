




































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
 
  NS_IMETHOD Run();
  nsresult PostDOMEvent();

  nsCOMPtr<nsIDOMNode> mEventNode;
  nsString mEventType;
};

#endif
