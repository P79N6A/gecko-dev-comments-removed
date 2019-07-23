





































#ifndef nsDOMPopupBlockedEvent_h__
#define nsDOMPopupBlockedEvent_h__

#include "nsIDOMPopupBlockedEvent.h"
#include "nsDOMEvent.h"
#include "nsIURI.h"

class nsDOMPopupBlockedEvent : public nsDOMEvent,
                               public nsIDOMPopupBlockedEvent
{
public:

  nsDOMPopupBlockedEvent(nsPresContext* aPresContext, nsEvent* aEvent)
  : nsDOMEvent(aPresContext, aEvent) {}

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_TO_NSDOMEVENT

  
  NS_DECL_NSIDOMPOPUPBLOCKEDEVENT
protected:
  nsWeakPtr        mRequestingWindow;
  nsCOMPtr<nsIURI> mPopupWindowURI;
  nsString         mPopupWindowFeatures;
  nsString         mPopupWindowName;
};

#endif 
