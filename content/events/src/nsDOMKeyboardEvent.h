





































#ifndef nsDOMKeyboardEvent_h__
#define nsDOMKeyboardEvent_h__

#include "nsIDOMKeyEvent.h"
#include "nsDOMUIEvent.h"

class nsDOMKeyboardEvent : public nsDOMUIEvent,
                           public nsIDOMKeyEvent
{
public:
  nsDOMKeyboardEvent(nsPresContext* aPresContext, nsKeyEvent* aEvent);
  virtual ~nsDOMKeyboardEvent();

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMKEYEVENT

  
  NS_FORWARD_TO_NSDOMUIEVENT

  
  NS_IMETHOD GetWhich(PRUint32 *aWhich);
};


#endif 
