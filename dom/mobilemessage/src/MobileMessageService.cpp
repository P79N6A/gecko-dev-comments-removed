



#include "SmsMessage.h"
#include "MmsMessage.h"
#include "MobileMessageThread.h"
#include "MobileMessageService.h"
#include "SmsSegmentInfo.h"

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
                                       uint64_t aThreadId,
                                       const nsAString& aIccId,
                                       const nsAString& aDelivery,
                                       const nsAString& aDeliveryStatus,
                                       const nsAString& aSender,
                                       const nsAString& aReceiver,
                                       const nsAString& aBody,
                                       const nsAString& aMessageClass,
                                       const JS::Value& aTimestamp,
                                       const JS::Value& aDeliveryTimestamp,
                                       const bool aRead,
                                       JSContext* aCx,
                                       nsIDOMMozSmsMessage** aMessage)
{
  return SmsMessage::Create(aId,
                            aThreadId,
                            aIccId,
                            aDelivery,
                            aDeliveryStatus,
                            aSender,
                            aReceiver,
                            aBody,
                            aMessageClass,
                            aTimestamp,
                            aDeliveryTimestamp,
                            aRead,
                            aCx,
                            aMessage);
}

NS_IMETHODIMP
MobileMessageService::CreateMmsMessage(int32_t               aId,
                                       uint64_t              aThreadId,
                                       const nsAString&      aIccId,
                                       const nsAString&      aDelivery,
                                       const JS::Value&      aDeliveryInfo,
                                       const nsAString&      aSender,
                                       const JS::Value&      aReceivers,
                                       const JS::Value&      aTimestamp,
                                       bool                  aRead,
                                       const nsAString&      aSubject,
                                       const nsAString&      aSmil,
                                       const JS::Value&      aAttachments,
                                       const JS::Value&      aExpiryDate,
                                       bool                  aIsReadReportRequested,
                                       JSContext*            aCx,
                                       nsIDOMMozMmsMessage** aMessage)
{
  return MmsMessage::Create(aId,
                            aThreadId,
                            aIccId,
                            aDelivery,
                            aDeliveryInfo,
                            aSender,
                            aReceivers,
                            aTimestamp,
                            aRead,
                            aSubject,
                            aSmil,
                            aAttachments,
                            aExpiryDate,
                            aIsReadReportRequested,
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

NS_IMETHODIMP
MobileMessageService::CreateThread(uint64_t aId,
                                   const JS::Value& aParticipants,
                                   const JS::Value& aTimestamp,
                                   const nsAString& aBody,
                                   uint64_t aUnreadCount,
                                   const nsAString& aLastMessageType,
                                   JSContext* aCx,
                                   nsIDOMMozMobileMessageThread** aThread)
{
  return MobileMessageThread::Create(aId,
                                     aParticipants,
                                     aTimestamp,
                                     aBody,
                                     aUnreadCount,
                                     aLastMessageType,
                                     aCx,
                                     aThread);
}

} 
} 
} 
