




#ifndef nsDOMContactChangeEvent_h__
#define nsDOMContactChangeEvent_h__

#include "nsIDOMContactManager.h"
#include "nsDOMEvent.h"

class nsDOMMozContactChangeEvent : public nsDOMEvent,
                                   public nsIDOMMozContactChangeEvent
{
public:
  nsDOMMozContactChangeEvent(nsPresContext* aPresContext, nsEvent* aEvent)
    : nsDOMEvent(aPresContext, aEvent) {}
                     
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsDOMMozContactChangeEvent, nsDOMEvent)
  
  NS_FORWARD_TO_NSDOMEVENT

  NS_DECL_NSIDOMMOZCONTACTCHANGEEVENT

  virtual nsresult InitFromCtor(const nsAString& aType,
                                JSContext* aCx, jsval* aVal);
private:
  nsString mContactID;
  nsString mReason;
};

#endif 
