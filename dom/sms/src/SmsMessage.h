




#ifndef mozilla_dom_sms_SmsMessage_h
#define mozilla_dom_sms_SmsMessage_h

#include "mozilla/dom/sms/PSms.h"
#include "nsIDOMSmsMessage.h"
#include "nsString.h"
#include "jspubtd.h"
#include "Types.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace dom {
namespace sms {

class SmsMessage MOZ_FINAL : public nsIDOMMozSmsMessage
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMMOZSMSMESSAGE

  SmsMessage(int32_t aId, DeliveryState aDelivery, const nsString& aSender,
             const nsString& aReceiver, const nsString& aBody,
             uint64_t aTimestamp, bool aRead);
  SmsMessage(const SmsMessageData& aData);

  static nsresult Create(int32_t aId,
                         const nsAString& aDelivery,
                         const nsAString& aSender,
                         const nsAString& aReceiver,
                         const nsAString& aBody,
                         const JS::Value& aTimestamp,
                         const bool aRead,
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
