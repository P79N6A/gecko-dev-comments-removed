





































#ifndef nsDOMProgressEvent_h__
#define nsDOMProgressEvent_h__

#include "nsIDOMProgressEvent.h"
#include "nsDOMEvent.h"








class nsDOMProgressEvent : public nsIDOMProgressEvent,
                           public nsDOMEvent
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
  PRBool  mLengthComputable;
  PRUint32 mLoaded;
  PRUint32 mTotal;
};

#endif 
