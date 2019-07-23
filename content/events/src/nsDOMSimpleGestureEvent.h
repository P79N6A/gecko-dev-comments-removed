



































#ifndef nsDOMSimpleGestureEvent_h__
#define nsDOMSimpleGestureEvent_h__

#include "nsIDOMSimpleGestureEvent.h"
#include "nsDOMMouseEvent.h"

class nsPresContext;

class nsDOMSimpleGestureEvent : public nsIDOMSimpleGestureEvent,
                                public nsDOMMouseEvent
{
public:
  nsDOMSimpleGestureEvent(nsPresContext*, nsSimpleGestureEvent*);
  virtual ~nsDOMSimpleGestureEvent();

  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSIDOMSIMPLEGESTUREEVENT

  
  NS_FORWARD_TO_NSDOMMOUSEEVENT
};

#endif
