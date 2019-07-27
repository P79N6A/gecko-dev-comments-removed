




#ifndef mozilla_dom_mobilemessage_SmsMessage_h
#define mozilla_dom_mobilemessage_SmsMessage_h

#include "mozilla/dom/mobilemessage/SmsTypes.h"
#include "nsIDOMMozSmsMessage.h"
#include "nsString.h"
#include "mozilla/dom/mobilemessage/Types.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace dom {

class SmsMessage MOZ_FINAL : public nsIDOMMozSmsMessage
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMMOZSMSMESSAGE

  SmsMessage(int32_t aId,
             uint64_t aThreadId,
             const nsString& aIccId,
             mobilemessage::DeliveryState aDelivery,
             mobilemessage::DeliveryStatus aDeliveryStatus,
             const nsString& aSender,
             const nsString& aReceiver,
             const nsString& aBody,
             mobilemessage::MessageClass aMessageClass,
             uint64_t aTimestamp,
             uint64_t aSentTimestamp,
             uint64_t aDeliveryTimestamp,
             bool aRead);

  SmsMessage(const mobilemessage::SmsMessageData& aData);

  static nsresult Create(int32_t aId,
                         uint64_t aThreadId,
                         const nsAString& aIccId,
                         const nsAString& aDelivery,
                         const nsAString& aDeliveryStatus,
                         const nsAString& aSender,
                         const nsAString& aReceiver,
                         const nsAString& aBody,
                         const nsAString& aMessageClass,
                         uint64_t aTimestamp,
                         uint64_t aSentTimestamp,
                         uint64_t aDeliveryTimestamp,
                         bool aRead,
                         JSContext* aCx,
                         nsIDOMMozSmsMessage** aMessage);
  const mobilemessage::SmsMessageData& GetData() const;

private:
  ~SmsMessage() {}

  
  SmsMessage();

  mobilemessage::SmsMessageData mData;
};

} 
} 

#endif 
