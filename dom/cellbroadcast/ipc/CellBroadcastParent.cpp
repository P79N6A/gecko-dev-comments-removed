





#include "mozilla/dom/cellbroadcast/CellBroadcastParent.h"
#include "nsServiceManagerUtils.h"

namespace mozilla {
namespace dom {
namespace cellbroadcast {

NS_IMPL_ISUPPORTS(CellBroadcastParent, nsICellBroadcastListener)

bool
CellBroadcastParent::Init()
{
  nsCOMPtr<nsICellBroadcastService> service =
    do_GetService(CELLBROADCAST_SERVICE_CONTRACTID);

  if (service) {
    return NS_SUCCEEDED(service->RegisterListener(this));
  }

  return false;
}

void
CellBroadcastParent::ActorDestroy(ActorDestroyReason aWhy)
{
  nsCOMPtr<nsICellBroadcastService> service =
    do_GetService(CELLBROADCAST_SERVICE_CONTRACTID);
  if (service) {
    service->UnregisterListener(this);
  }
}




NS_IMETHODIMP
CellBroadcastParent::NotifyMessageReceived(uint32_t aServiceId,
                                           const nsAString& aGsmGeographicalScope,
                                           uint16_t aMessageCode,
                                           uint16_t aMessageId,
                                           const nsAString& aLanguage,
                                           const nsAString& aBody,
                                           const nsAString& aMessageClass,
                                           DOMTimeStamp aTimestamp,
                                           uint32_t aCdmaServiceCategory,
                                           bool aHasEtwsInfo,
                                           const nsAString& aEtwsWarningType,
                                           bool aEtwsEmergencyUserAlert,
                                           bool aEtwsPopup)
{
  return SendNotifyReceivedMessage(aServiceId,
                                   nsString(aGsmGeographicalScope),
                                   aMessageCode,
                                   aMessageId,
                                   nsString(aLanguage),
                                   nsString(aBody),
                                   nsString(aMessageClass),
                                   aTimestamp,
                                   aCdmaServiceCategory,
                                   aHasEtwsInfo,
                                   nsString(aEtwsWarningType),
                                   aEtwsEmergencyUserAlert,
                                   aEtwsPopup) ? NS_OK : NS_ERROR_FAILURE;
}

} 
} 
} 