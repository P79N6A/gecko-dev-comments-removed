





































#ifndef nsDOMMessageEvent_h__
#define nsDOMMessageEvent_h__

#include "nsIDOMMessageEvent.h"
#include "nsDOMEvent.h"
#include "nsCycleCollectionParticipant.h"







class nsDOMMessageEvent : public nsIDOMMessageEvent,
                          public nsDOMEvent
{
public:
  nsDOMMessageEvent(nsPresContext* aPresContext, nsEvent* aEvent)
    : nsDOMEvent(aPresContext, aEvent)
  {
  }
                     
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsDOMMessageEvent, nsDOMEvent)

  NS_DECL_NSIDOMMESSAGEEVENT

  
  NS_FORWARD_TO_NSDOMEVENT

private:
  nsString mData;
  nsString mDomain;
  nsString mURI;
  nsCOMPtr<nsIDOMWindow> mSource;
};

#endif 
