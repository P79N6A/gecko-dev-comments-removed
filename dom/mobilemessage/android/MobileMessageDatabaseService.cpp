




#include "MobileMessageDatabaseService.h"
#include "AndroidBridge.h"

namespace mozilla {
namespace dom {
namespace mobilemessage {

NS_IMPL_ISUPPORTS(MobileMessageDatabaseService, nsIMobileMessageDatabaseService)

NS_IMETHODIMP
MobileMessageDatabaseService::GetMessageMoz(int32_t aMessageId,
                                            nsIMobileMessageCallback* aRequest)
{
  if (!AndroidBridge::Bridge()) {
    return NS_OK;
  }

  AndroidBridge::Bridge()->GetMessage(aMessageId, aRequest);
  return NS_OK;
}

NS_IMETHODIMP
MobileMessageDatabaseService::DeleteMessage(int32_t *aMessageIds,
                                            uint32_t aLength,
                                            nsIMobileMessageCallback* aRequest)
{
  if (!AndroidBridge::Bridge()) {
    return NS_OK;
  }

  if (!aMessageIds) {
    return NS_OK;
  }

  if (aLength != 1) {
    return NS_ERROR_FAILURE;
  }

  AndroidBridge::Bridge()->DeleteMessage(aMessageIds[0], aRequest);
  return NS_OK;
}

NS_IMETHODIMP
MobileMessageDatabaseService::CreateMessageCursor(bool aHasStartDate,
                                                  uint64_t aStartDate,
                                                  bool aHasEndDate,
                                                  uint64_t aEndDate,
                                                  const char16_t** aNumbers,
                                                  uint32_t aNumbersCount,
                                                  const nsAString& aDelivery,
                                                  bool aHasRead,
                                                  bool aRead,
                                                  uint64_t aThreadId,
                                                  bool aReverse,
                                                  nsIMobileMessageCursorCallback* aCallback,
                                                  nsICursorContinueCallback** aResult)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
MobileMessageDatabaseService::MarkMessageRead(int32_t aMessageId,
                                              bool aValue,
                                              bool aSendReadReport,
                                              nsIMobileMessageCallback* aRequest)
{
  
  return NS_OK;
}

NS_IMETHODIMP
MobileMessageDatabaseService::CreateThreadCursor(nsIMobileMessageCursorCallback* aCallback,
                                                 nsICursorContinueCallback** aResult)
{
  NS_NOTYETIMPLEMENTED("Implement me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

} 
} 
} 
