



#include "SmsMessage.h"
#include "MmsMessage.h"
#include "MobileMessageService.h"
#include "SmsSegmentInfo.h"
#include "jsapi.h"

namespace mozilla {
namespace dom {
namespace mobilemessage {

NS_IMPL_ISUPPORTS1(MobileMessageService, nsIMobileMessageService)

 StaticRefPtr<MobileMessageService> MobileMessageService::sSingleton;

 already_AddRefed<MobileMessageService>
MobileMessageService::GetInstance()
{
  if (!sSingleton) {
    sSingleton = new MobileMessageService();
    ClearOnShutdown(&sSingleton);
  }

  nsRefPtr<MobileMessageService> service = sSingleton.get();
  return service.forget();
}

NS_IMETHODIMP
MobileMessageService::CreateSmsMessage(int32_t aId,
                                       const nsAString& aDelivery,
                                       const nsAString& aDeliveryStatus,
                                       const nsAString& aSender,
                                       const nsAString& aReceiver,
                                       const nsAString& aBody,
                                       const nsAString& aMessageClass,
                                       const jsval& aTimestamp,
                                       const bool aRead,
                                       JSContext* aCx,
                                       nsIDOMMozSmsMessage** aMessage)
{
  return SmsMessage::Create(aId,
                            aDelivery,
                            aDeliveryStatus,
                            aSender,
                            aReceiver,
                            aBody,
                            aMessageClass,
                            aTimestamp,
                            aRead,
                            aCx,
                            aMessage);
}

NS_IMETHODIMP
MobileMessageService::CreateMmsMessage(int32_t               aId,
                                       const nsAString&      aState,
                                       const JS::Value&      aDeliveryStatus,
                                       const nsAString&      aSender,
                                       const JS::Value&      aReceivers,
                                       const JS::Value&      aTimestamp,
                                       bool                  aRead,
                                       const nsAString&      aSubject,
                                       const nsAString&      aSmil,
                                       const JS::Value&      aAttachments,
                                       JSContext*            aCx,
                                       nsIDOMMozMmsMessage** aMessage)
{
  return MmsMessage::Create(aId,
                            aState,
                            aDeliveryStatus,
                            aSender,
                            aReceivers,
                            aTimestamp,
                            aRead,
                            aSubject,
                            aSmil,
                            aAttachments,
                            aCx,
                            aMessage);
}

NS_IMETHODIMP
MobileMessageService::CreateSmsSegmentInfo(int32_t aSegments,
                                           int32_t aCharsPerSegment,
                                           int32_t aCharsAvailableInLastSegment,
                                           nsIDOMMozSmsSegmentInfo** aSegmentInfo)
{
  nsCOMPtr<nsIDOMMozSmsSegmentInfo> info =
      new SmsSegmentInfo(aSegments, aCharsPerSegment, aCharsAvailableInLastSegment);
  info.forget(aSegmentInfo);
  return NS_OK;
}

} 
} 
} 
