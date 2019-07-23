





































#ifndef nsDOMMouseEvent_h__
#define nsDOMMouseEvent_h__

#include "nsIDOMMouseEvent.h"
#include "nsDOMUIEvent.h"

class nsIContent;
class nsIScrollableView;
class nsEvent;

class nsDOMMouseEvent : public nsIDOMMouseEvent,
                        public nsDOMUIEvent
{
public:
  nsDOMMouseEvent(nsPresContext* aPresContext, nsInputEvent* aEvent);
  virtual ~nsDOMMouseEvent();

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMMOUSEEVENT
  
  
  NS_FORWARD_TO_NSDOMUIEVENT

  
  NS_IMETHOD GetWhich(PRUint32 *aWhich);
};

#endif 
