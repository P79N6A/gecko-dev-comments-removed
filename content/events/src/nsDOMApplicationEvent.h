



 
#ifndef nsDOMApplicationEvent_h__
#define nsDOMApplicationEvent_h__
 
#include "nsIDOMApplicationRegistry.h"
#include "nsDOMEvent.h"
 
class nsDOMMozApplicationEvent : public nsDOMEvent,
                                 public nsIDOMMozApplicationEvent
{
public:
  nsDOMMozApplicationEvent(nsPresContext* aPresContext, nsEvent* aEvent)
    : nsDOMEvent(aPresContext, aEvent) {}

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsDOMMozApplicationEvent, nsDOMEvent)
  
  NS_FORWARD_TO_NSDOMEVENT
 
  NS_DECL_NSIDOMMOZAPPLICATIONEVENT
 
  virtual nsresult InitFromCtor(const nsAString& aType, JSContext* aCx, jsval* aVal);

private:
  nsCOMPtr<mozIDOMApplication> mApplication;
};
 
#endif 