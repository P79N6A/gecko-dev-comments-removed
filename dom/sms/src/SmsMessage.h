




































#ifndef mozilla_dom_sms_SmsMessage_h
#define mozilla_dom_sms_SmsMessage_h

#include "nsIDOMSmsMessage.h"
#include "nsString.h"

namespace mozilla {
namespace dom {
namespace sms {

class SmsMessage : public nsIDOMMozSmsMessage
{
public:
  enum DeliveryState {
    eDeliveryState_Sent,
    eDeliveryState_Received
  };

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMMOZSMSMESSAGE

  SmsMessage(PRInt32 aId, DeliveryState aDelivery, const nsAString& aSender,
             const nsAString& aReceiver, const nsAString& aBody,
             PRUint64 aTimestamp);

private:
  
  SmsMessage();

  PRInt32       mId;
  DeliveryState mDelivery;
  nsString      mSender;
  nsString      mReceiver;
  nsString      mBody;
  PRUint64      mTimestamp; 
};

} 
} 
} 

#endif
