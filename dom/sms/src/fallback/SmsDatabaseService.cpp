




#include "SmsDatabaseService.h"

namespace mozilla {
namespace dom {
namespace sms {

NS_IMPL_ISUPPORTS1(SmsDatabaseService, nsISmsDatabaseService)

NS_IMETHODIMP
SmsDatabaseService::SaveReceivedMessage(const nsAString& aSender,
                                        const nsAString& aBody,
                                        PRUint64 aDate, PRInt32* aId)
{
  *aId = -1;
  NS_ERROR("We should not be here!");
  return NS_OK;
}

NS_IMETHODIMP
SmsDatabaseService::SaveSentMessage(const nsAString& aReceiver,
                                    const nsAString& aBody,
                                    PRUint64 aDate, PRInt32* aId)
{
  *aId = -1;
  NS_ERROR("We should not be here!");
  return NS_OK;
}

NS_IMETHODIMP
SmsDatabaseService::GetMessageMoz(PRInt32 aMessageId, PRInt32 aRequestId,
                                  PRUint64 aProcessId)
{
  NS_ERROR("We should not be here!");
  return NS_OK;
}

NS_IMETHODIMP
SmsDatabaseService::DeleteMessage(PRInt32 aMessageId, PRInt32 aRequestId,
                                  PRUint64 aProcessId)
{
  NS_ERROR("We should not be here!");
  return NS_OK;
}

NS_IMETHODIMP
SmsDatabaseService::CreateMessageList(nsIDOMMozSmsFilter* aFilter,
                                      bool aReverse, PRInt32 aRequestId,
                                      PRUint64 aProcessId)
{
  NS_ERROR("We should not be here!");
  return NS_OK;
}

NS_IMETHODIMP
SmsDatabaseService::GetNextMessageInList(PRInt32 aListId, PRInt32 aRequestId,
                                         PRUint64 aProcessId)
{
  NS_ERROR("We should not be here!");
  return NS_OK;
}

NS_IMETHODIMP
SmsDatabaseService::ClearMessageList(PRInt32 aListId)
{
  NS_ERROR("We should not be here!");
  return NS_OK;
}

NS_IMETHODIMP
SmsDatabaseService::MarkMessageRead(PRInt32 aMessageId, bool aValue,
                                  PRInt32 aRequestId, PRUint64 aProcessId)
{
  NS_ERROR("We should not be here!");
  return NS_OK;
}

} 
} 
} 
