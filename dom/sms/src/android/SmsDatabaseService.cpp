




#include "SmsFilter.h"
#include "SmsDatabaseService.h"
#include "AndroidBridge.h"

namespace mozilla {
namespace dom {
namespace sms {

NS_IMPL_ISUPPORTS1(SmsDatabaseService, nsISmsDatabaseService)

NS_IMETHODIMP
SmsDatabaseService::GetMessageMoz(int32_t aMessageId, nsISmsRequest* aRequest)
{
  if (!AndroidBridge::Bridge()) {
    return NS_OK;
  }

  AndroidBridge::Bridge()->GetMessage(aMessageId, aRequest);
  return NS_OK;
}

NS_IMETHODIMP
SmsDatabaseService::DeleteMessage(int32_t aMessageId, nsISmsRequest* aRequest)
{
  if (!AndroidBridge::Bridge()) {
    return NS_OK;
  }

  AndroidBridge::Bridge()->DeleteMessage(aMessageId, aRequest);
  return NS_OK;
}

NS_IMETHODIMP
SmsDatabaseService::CreateMessageList(nsIDOMMozSmsFilter* aFilter,
                                      bool aReverse, nsISmsRequest* aRequest)
{
  if (!AndroidBridge::Bridge()) {
    return NS_OK;
  }

  AndroidBridge::Bridge()->CreateMessageList(
    static_cast<SmsFilter*>(aFilter)->GetData(), aReverse, aRequest);
  return NS_OK;
}

NS_IMETHODIMP
SmsDatabaseService::GetNextMessageInList(int32_t aListId, nsISmsRequest* aRequest)
{
  if (!AndroidBridge::Bridge()) {
    return NS_OK;
  }

  AndroidBridge::Bridge()->GetNextMessageInList(aListId, aRequest);
  return NS_OK;
}

NS_IMETHODIMP
SmsDatabaseService::ClearMessageList(int32_t aListId)
{
  if (!AndroidBridge::Bridge()) {
    return NS_OK;
  }

  AndroidBridge::Bridge()->ClearMessageList(aListId);
  return NS_OK;
}

NS_IMETHODIMP
SmsDatabaseService::MarkMessageRead(int32_t aMessageId, bool aValue,
                                    nsISmsRequest* aRequest)
{
  
  return NS_OK;
}

NS_IMETHODIMP
SmsDatabaseService::GetThreadList(nsISmsRequest* aRequest)
{
  NS_NOTYETIMPLEMENTED("Implement me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

} 
} 
} 
