





































#ifndef nsDOMMessageEvent_h__
#define nsDOMMessageEvent_h__

#include "nsIDOMMessageEvent.h"
#include "nsDOMEvent.h"
#include "nsCycleCollectionParticipant.h"








class nsDOMMessageEvent : public nsDOMEvent,
                          public nsIDOMMessageEvent
{
public:
  nsDOMMessageEvent(nsPresContext* aPresContext, nsEvent* aEvent);
  ~nsDOMMessageEvent();
                     
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(nsDOMMessageEvent,
                                                         nsDOMEvent)

  NS_DECL_NSIDOMMESSAGEEVENT

  
  NS_FORWARD_TO_NSDOMEVENT

  void RootData();
  void UnrootData();
private:
  jsval mData;
  bool mDataRooted;
  nsString mOrigin;
  nsString mLastEventId;
  nsCOMPtr<nsIDOMWindow> mSource;
};

#endif 
