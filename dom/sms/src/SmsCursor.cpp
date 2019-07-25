




































#include "SmsCursor.h"
#include "nsIDOMClassInfo.h"
#include "nsDOMError.h"
#include "nsIDOMSmsMessage.h"
#include "nsIDOMSmsRequest.h"
#include "SmsRequest.h"
#include "SmsRequestManager.h"
#include "nsISmsDatabaseService.h"

DOMCI_DATA(MozSmsCursor, mozilla::dom::sms::SmsCursor)

namespace mozilla {
namespace dom {
namespace sms {

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(SmsCursor)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMozSmsCursor)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(MozSmsCursor)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_2(SmsCursor, mRequest, mMessage)

NS_IMPL_CYCLE_COLLECTING_ADDREF(SmsCursor)
NS_IMPL_CYCLE_COLLECTING_RELEASE(SmsCursor)

SmsCursor::SmsCursor()
  : mListId(-1)
{
}

SmsCursor::SmsCursor(PRInt32 aListId, nsIDOMMozSmsRequest* aRequest)
  : mListId(aListId)
  , mRequest(aRequest)
{
}

SmsCursor::~SmsCursor()
{
  NS_ASSERTION(!mMessage, "mMessage shouldn't be set!");

  if (mListId != -1) {
    nsCOMPtr<nsISmsDatabaseService> smsDBService =
      do_GetService(SMS_DATABASE_SERVICE_CONTRACTID);

    if (!smsDBService) {
      NS_ERROR("Can't find SmsDBService!");
    }

    smsDBService->ClearMessageList(mListId);
  }
}

void
SmsCursor::Disconnect()
{
  NS_ASSERTION(!mMessage, "mMessage shouldn't be set!");

  mRequest = nsnull;
  mListId = -1;
}

NS_IMETHODIMP
SmsCursor::GetMessage(nsIDOMMozSmsMessage** aMessage)
{
  NS_IF_ADDREF(*aMessage = mMessage);
  return NS_OK;
}

NS_IMETHODIMP
SmsCursor::Continue()
{
  
  if (!mMessage) {
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  mMessage = nsnull;
  static_cast<SmsRequest*>(mRequest.get())->Reset();

  PRInt32 requestId = SmsRequestManager::GetInstance()->AddRequest(mRequest);

  nsCOMPtr<nsISmsDatabaseService> smsDBService =
    do_GetService(SMS_DATABASE_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(smsDBService, NS_ERROR_FAILURE);

  smsDBService->GetNextMessageInList(mListId, requestId, 0);

  return NS_OK;
}

} 
} 
} 

