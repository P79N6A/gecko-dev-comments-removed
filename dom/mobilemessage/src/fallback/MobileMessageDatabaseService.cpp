




#include "MobileMessageDatabaseService.h"

namespace mozilla {
namespace dom {
namespace mobilemessage {

NS_IMPL_ISUPPORTS1(MobileMessageDatabaseService, nsIMobileMessageDatabaseService)

NS_IMETHODIMP
MobileMessageDatabaseService::GetMessageMoz(int32_t aMessageId,
                                            nsIMobileMessageCallback* aRequest)
{
  NS_ERROR("We should not be here!");
  return NS_OK;
}

NS_IMETHODIMP
MobileMessageDatabaseService::DeleteMessage(int32_t aMessageId,
                                            nsIMobileMessageCallback* aRequest)
{
  NS_ERROR("We should not be here!");
  return NS_OK;
}

NS_IMETHODIMP
MobileMessageDatabaseService::CreateMessageList(nsIDOMMozSmsFilter* aFilter,
                                                bool aReverse,
                                                nsIMobileMessageCallback* aRequest)
{
  NS_ERROR("We should not be here!");
  return NS_OK;
}

NS_IMETHODIMP
MobileMessageDatabaseService::GetNextMessageInList(int32_t aListId,
                                                   nsIMobileMessageCallback* aRequest)
{
  NS_ERROR("We should not be here!");
  return NS_OK;
}

NS_IMETHODIMP
MobileMessageDatabaseService::ClearMessageList(int32_t aListId)
{
  NS_ERROR("We should not be here!");
  return NS_OK;
}

NS_IMETHODIMP
MobileMessageDatabaseService::MarkMessageRead(int32_t aMessageId,
                                              bool aValue,
                                              nsIMobileMessageCallback* aRequest)
{
  NS_ERROR("We should not be here!");
  return NS_OK;
}

NS_IMETHODIMP
MobileMessageDatabaseService::GetThreadList(nsIMobileMessageCallback* aRequest)
{
  NS_ERROR("We should not be here!");
  return NS_OK;
}

} 
} 
} 
