





































#ifndef nsDOMCloseEvent_h__
#define nsDOMCloseEvent_h__

#include "nsIDOMCloseEvent.h"
#include "nsDOMEvent.h"







class nsDOMCloseEvent : public nsDOMEvent,
                        public nsIDOMCloseEvent
{
public:
  nsDOMCloseEvent(nsPresContext* aPresContext, nsEvent* aEvent)
    : nsDOMEvent(aPresContext, aEvent),
    mWasClean(false),
    mReasonCode(1005) {}
                     
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_TO_NSDOMEVENT

  NS_DECL_NSIDOMCLOSEEVENT

private:
  bool mWasClean;
  PRUint16 mReasonCode;
  nsString mReason;
};

#endif 
