






































#ifndef nsDOMCompositionEvent_h__
#define nsDOMCompositionEvent_h__

#include "nsDOMUIEvent.h"
#include "nsIDOMCompositionEvent.h"

class nsDOMCompositionEvent : public nsDOMUIEvent,
                              public nsIDOMCompositionEvent
{
public:
  nsDOMCompositionEvent(nsPresContext* aPresContext,
                        nsCompositionEvent* aEvent);
  virtual ~nsDOMCompositionEvent();

  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_TO_NSDOMUIEVENT
  NS_DECL_NSIDOMCOMPOSITIONEVENT

protected:
  nsString mData;
  nsString mLocale;
};

#endif 
