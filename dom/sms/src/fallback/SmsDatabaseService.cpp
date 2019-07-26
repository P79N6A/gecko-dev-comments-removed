




#include "SmsDatabaseService.h"

namespace mozilla {
namespace dom {
namespace sms {

NS_IMPL_ISUPPORTS1(SmsDatabaseService, nsISmsDatabaseService)

NS_IMETHODIMP
SmsDatabaseService::GetMessageMoz(int32_t aMessageId,
                                  nsISmsRequest* aRequest)
{
  NS_ERROR("We should not be here!");
  return NS_OK;
}

NS_IMETHODIMP
SmsDatabaseService::DeleteMessage(int32_t aMessageId,
                                  nsISmsRequest* aRequest)
{
  NS_ERROR("We should not be here!");
  return NS_OK;
}

NS_IMETHODIMP
SmsDatabaseService::CreateMessageList(nsIDOMMozSmsFilter* aFilter,
                                      bool aReverse,
                                      nsISmsRequest* aRequest)
{
  NS_ERROR("We should not be here!");
  return NS_OK;
}

NS_IMETHODIMP
SmsDatabaseService::GetNextMessageInList(int32_t aListId,
                                         nsISmsRequest* aRequest)
{
  NS_ERROR("We should not be here!");
  return NS_OK;
}

NS_IMETHODIMP
SmsDatabaseService::ClearMessageList(int32_t aListId)
{
  NS_ERROR("We should not be here!");
  return NS_OK;
}

NS_IMETHODIMP
SmsDatabaseService::MarkMessageRead(int32_t aMessageId,
                                    bool aValue,
                                    nsISmsRequest* aRequest)
{
  NS_ERROR("We should not be here!");
  return NS_OK;
}

NS_IMETHODIMP
SmsDatabaseService::GetThreadList(nsISmsRequest* aRequest)
{
  NS_ERROR("We should not be here!");
  return NS_OK;
}

} 
} 
} 
