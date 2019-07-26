





#ifndef nsDOMProgressEvent_h__
#define nsDOMProgressEvent_h__

#include "nsIDOMProgressEvent.h"
#include "nsDOMEvent.h"








class nsDOMProgressEvent : public nsDOMEvent,
                           public nsIDOMProgressEvent
{
public:
  nsDOMProgressEvent(nsPresContext* aPresContext, nsEvent* aEvent)
    : nsDOMEvent(aPresContext, aEvent)
  {
  }
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMPROGRESSEVENT
    
  
  NS_FORWARD_TO_NSDOMEVENT

private:
  bool    mLengthComputable;
  uint64_t mLoaded;
  uint64_t mTotal;
};

#endif 
