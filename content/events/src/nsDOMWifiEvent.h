




#ifndef nsDOMWifiEvent_h__
#define nsDOMWifiEvent_h__

#include "nsIWifi.h"
#include "nsIWifiEventInits.h"
#include "nsDOMEvent.h"

class nsDOMMozWifiStatusChangeEvent : public nsDOMEvent,
                                   public nsIDOMMozWifiStatusChangeEvent
{
public:
  nsDOMMozWifiStatusChangeEvent(nsPresContext* aPresContext, nsEvent* aEvent)
    : nsDOMEvent(aPresContext, aEvent) {}
                     
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsDOMMozWifiStatusChangeEvent, nsDOMEvent)
  
  NS_FORWARD_TO_NSDOMEVENT

  NS_DECL_NSIDOMMOZWIFISTATUSCHANGEEVENT

  virtual nsresult InitFromCtor(const nsAString& aType,
                                JSContext* aCx, jsval* aVal);
private:
  nsCOMPtr<nsIVariant> mNetwork;
  nsString mStatus;
};

class nsDOMMozWifiConnectionInfoEvent : public nsDOMEvent,
                                        public nsIDOMMozWifiConnectionInfoEvent
{
public:
  nsDOMMozWifiConnectionInfoEvent(nsPresContext* aPresContext, nsEvent* aEvent)
    : nsDOMEvent(aPresContext, aEvent) {}
                     
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsDOMMozWifiConnectionInfoEvent, nsDOMEvent)
  
  NS_FORWARD_TO_NSDOMEVENT

  NS_DECL_NSIDOMMOZWIFICONNECTIONINFOEVENT

  virtual nsresult InitFromCtor(const nsAString& aType,
                                JSContext* aCx, jsval* aVal);
private:
  nsCOMPtr<nsIVariant> mNetwork;
  int16_t mSignalStrength;
  int16_t mRelSignalStrength;
  int32_t mLinkSpeed;
};

#endif 
