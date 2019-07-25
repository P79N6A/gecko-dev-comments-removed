




































#include "SmsFilter.h"
#include "SmsDatabaseService.h"
#include "AndroidBridge.h"

namespace mozilla {
namespace dom {
namespace sms {

NS_IMPL_ISUPPORTS1(SmsDatabaseService, nsISmsDatabaseService)

NS_IMETHODIMP
SmsDatabaseService::SaveSentMessage(const nsAString& aReceiver,
                                    const nsAString& aBody,
                                    PRUint64 aDate, PRInt32* aId)
{
  *aId = -1;

  if (!AndroidBridge::Bridge()) {
    return NS_OK;
  }

  *aId = AndroidBridge::Bridge()->SaveSentMessage(aReceiver, aBody, aDate);
  return NS_OK;
}

NS_IMETHODIMP
SmsDatabaseService::GetMessageMoz(PRInt32 aMessageId, PRInt32 aRequestId,
                                  PRUint64 aProcessId)
{
  if (!AndroidBridge::Bridge()) {
    return NS_OK;
  }

  AndroidBridge::Bridge()->GetMessage(aMessageId, aRequestId, aProcessId);
  return NS_OK;
}

NS_IMETHODIMP
SmsDatabaseService::DeleteMessage(PRInt32 aMessageId, PRInt32 aRequestId,
                                  PRUint64 aProcessId)
{
  if (!AndroidBridge::Bridge()) {
    return NS_OK;
  }

  AndroidBridge::Bridge()->DeleteMessage(aMessageId, aRequestId, aProcessId);
  return NS_OK;
}

NS_IMETHODIMP
SmsDatabaseService::CreateMessageList(nsIDOMMozSmsFilter* aFilter,
                                      bool aReverse, PRInt32 aRequestId,
                                      PRUint64 aProcessId)
{
  if (!AndroidBridge::Bridge()) {
    return NS_OK;
  }

  AndroidBridge::Bridge()->CreateMessageList(
    static_cast<SmsFilter*>(aFilter)->GetData(), aReverse, aRequestId, aProcessId
  );
  return NS_OK;
}

NS_IMETHODIMP
SmsDatabaseService::GetNextMessageInList(PRInt32 aListId, PRInt32 aRequestId,
                                         PRUint64 aProcessId)
{
  
  return NS_OK;
}

} 
} 
} 
