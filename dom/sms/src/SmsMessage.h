




































#ifndef mozilla_dom_sms_SmsMessage_h
#define mozilla_dom_sms_SmsMessage_h

#include "mozilla/dom/sms/PSms.h"
#include "nsIDOMSmsMessage.h"
#include "nsString.h"
#include "jspubtd.h"

namespace mozilla {
namespace dom {
namespace sms {

class SmsMessage : public nsIDOMMozSmsMessage
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMMOZSMSMESSAGE

  SmsMessage(const SmsMessageData& aData);

  static nsresult Create(PRInt32 aId,
                         const nsAString& aDelivery,
                         const nsAString& aSender,
                         const nsAString& aReceiver,
                         const nsAString& aBody,
                         const JS::Value& aTimestamp,
                         JSContext* aCx,
                         nsIDOMMozSmsMessage** aMessage);
  const SmsMessageData& GetData() const;

private:
  
  SmsMessage();

  SmsMessageData mData;
};

} 
} 
} 

#endif 
