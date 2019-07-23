





































#ifndef nsDOMMouseEvent_h__
#define nsDOMMouseEvent_h__

#include "nsIDOMMouseEvent.h"
#include "nsDOMUIEvent.h"
#include "nsIDOMNSMouseEvent.h"

class nsIContent;
class nsIScrollableView;
class nsEvent;

class nsDOMMouseEvent : public nsDOMUIEvent,
                        public nsIDOMNSMouseEvent
{
public:
  nsDOMMouseEvent(nsPresContext* aPresContext, nsInputEvent* aEvent);
  virtual ~nsDOMMouseEvent();

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMMOUSEEVENT

  
  NS_DECL_NSIDOMNSMOUSEEVENT

  
  NS_FORWARD_TO_NSDOMUIEVENT

  
  NS_IMETHOD GetWhich(PRUint32 *aWhich);
};

#define NS_FORWARD_TO_NSDOMMOUSEEVENT         \
  NS_FORWARD_NSIDOMMOUSEEVENT(nsDOMMouseEvent::) \
  NS_FORWARD_TO_NSDOMUIEVENT

#endif 
