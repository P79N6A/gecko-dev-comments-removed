




































#ifndef mozilla_dom_sms_SmsEvent_h
#define mozilla_dom_sms_SmsEvent_h

#include "nsIDOMSmsEvent.h"
#include "nsDOMEvent.h"

class nsIDOMMozSmsMessage;

namespace mozilla {
namespace dom {
namespace sms {

class SmsEvent : public nsIDOMMozSmsEvent
               , public nsDOMEvent
{
public:
  SmsEvent(nsPresContext* aPresContext, nsEvent* aEvent)
    : nsDOMEvent(aPresContext, aEvent)
    , mMessage(nsnull)
  {}

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMMOZSMSEVENT

  NS_FORWARD_TO_NSDOMEVENT

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(SmsEvent, nsDOMEvent)

  nsresult Init(const nsAString & aEventTypeArg, bool aCanBubbleArg,
                bool aCancelableArg, nsIDOMMozSmsMessage* aMessage);

private:
  nsCOMPtr<nsIDOMMozSmsMessage> mMessage;
};

} 
} 
} 

#endif 
