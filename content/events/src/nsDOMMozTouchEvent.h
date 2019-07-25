




































#ifndef nsDOMMozTouchEvent_h__
#define nsDOMMozTouchEvent_h__

#include "nsIDOMMozTouchEvent.h"
#include "nsDOMMouseEvent.h"

class nsPresContext;

class nsDOMMozTouchEvent : public nsDOMMouseEvent,
                           public nsIDOMMozTouchEvent
{
public:
  nsDOMMozTouchEvent(nsPresContext* aPresCOntext, nsMozTouchEvent* aEvent);
  virtual ~nsDOMMozTouchEvent();

  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSIDOMMOZTOUCHEVENT

  
  NS_FORWARD_TO_NSDOMMOUSEEVENT
};

#endif
