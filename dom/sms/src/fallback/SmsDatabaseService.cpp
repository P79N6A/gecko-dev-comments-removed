




#include "SmsDatabaseService.h"

namespace mozilla {
namespace dom {
namespace sms {

NS_IMPL_ISUPPORTS1(SmsDatabaseService, nsISmsDatabaseService)

NS_IMETHODIMP
SmsDatabaseService::SaveReceivedMessage(const nsAString& aSender,
                                        const nsAString& aBody,
                                        uint64_t aDate, int32_t* aId)
{
  *aId = -1;
  NS_ERROR("We should not be here!");
  return NS_OK;
}

NS_IMETHODIMP
SmsDatabaseService::SaveSentMessage(const nsAString& aReceiver,
                                    const nsAString& aBody,
                                    uint64_t aDate, int32_t* aId)
{
  *aId = -1;
  NS_ERROR("We should not be here!");
  return NS_OK;
}

NS_IMETHODIMP
SmsDatabaseService::GetMessageMoz(int32_t aMessageId, int32_t aRequestId,
                                  uint64_t aProcessId)
{
  NS_ERROR("We should not be here!");
  return NS_OK;
}

NS_IMETHODIMP
SmsDatabaseService::DeleteMessage(int32_t aMessageId, int32_t aRequestId,
                                  uint64_t aProcessId)
{
  NS_ERROR("We should not be here!");
  return NS_OK;
}

NS_IMETHODIMP
SmsDatabaseService::CreateMessageList(nsIDOMMozSmsFilter* aFilter,
                                      bool aReverse, int32_t aRequestId,
                                      uint64_t aProcessId)
{
  NS_ERROR("We should not be here!");
  return NS_OK;
}

NS_IMETHODIMP
SmsDatabaseService::GetNextMessageInList(int32_t aListId, int32_t aRequestId,
                                         uint64_t aProcessId)
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
SmsDatabaseService::MarkMessageRead(int32_t aMessageId, bool aValue,
                                  int32_t aRequestId, uint64_t aProcessId)
{
  NS_ERROR("We should not be here!");
  return NS_OK;
}

} 
} 
} 
