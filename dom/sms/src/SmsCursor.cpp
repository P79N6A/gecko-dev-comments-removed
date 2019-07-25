




































#include "SmsCursor.h"
#include "nsIDOMClassInfo.h"
#include "nsDOMError.h"
#include "nsIDOMSmsFilter.h"
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

NS_IMPL_CYCLE_COLLECTION_3(SmsCursor, mFilter, mRequest, mMessage)

NS_IMPL_CYCLE_COLLECTING_ADDREF(SmsCursor)
NS_IMPL_CYCLE_COLLECTING_RELEASE(SmsCursor)

SmsCursor::SmsCursor(nsIDOMMozSmsFilter* aFilter)
  : mListId(-1)
  , mFilter(aFilter)
{
}

SmsCursor::SmsCursor(PRInt32 aListId, nsIDOMMozSmsFilter* aFilter, nsIDOMMozSmsRequest* aRequest)
  : mListId(aListId)
  , mFilter(aFilter)
  , mRequest(aRequest)
{
}

NS_IMETHODIMP
SmsCursor::GetFilter(nsIDOMMozSmsFilter** aFilter)
{
  NS_ADDREF(*aFilter = mFilter);
  return NS_OK;
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

