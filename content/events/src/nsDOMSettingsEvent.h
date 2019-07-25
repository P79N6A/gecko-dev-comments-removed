




#ifndef nsDOMSettingsEvent_h__
#define nsDOMSettingsEvent_h__

#include "nsIDOMSettingsManager.h"
#include "nsDOMEvent.h"

class nsDOMMozSettingsEvent : public nsDOMEvent,
                              public nsIDOMMozSettingsEvent
{
public:
  nsDOMMozSettingsEvent(nsPresContext* aPresContext, nsEvent* aEvent)
    : nsDOMEvent(aPresContext, aEvent) {}
                     
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsDOMMozSettingsEvent, nsDOMEvent)
  
  NS_FORWARD_TO_NSDOMEVENT

  NS_DECL_NSIDOMMOZSETTINGSEVENT

  virtual nsresult InitFromCtor(const nsAString& aType,
                                JSContext* aCx, jsval* aVal);
private:
  nsString mSettingName;
  nsCOMPtr<nsIVariant> mSettingValue;
};

#endif 
