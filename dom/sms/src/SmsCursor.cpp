




#include "SmsCursor.h"
#include "nsIDOMClassInfo.h"
#include "nsError.h"
#include "nsIDOMMozSmsMessage.h"
#include "SmsRequest.h"
#include "nsIMobileMessageDatabaseService.h"

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

SmsCursor::SmsCursor(int32_t aListId, nsISmsRequest* aRequest)
  : mListId(aListId)
  , mRequest(aRequest)
{
}

SmsCursor::~SmsCursor()
{
  NS_ASSERTION(!mMessage, "mMessage shouldn't be set!");

  if (mListId != -1) {
    nsCOMPtr<nsIMobileMessageDatabaseService> mobileMessageDBService =
      do_GetService(MOBILE_MESSAGE_DATABASE_SERVICE_CONTRACTID);

    if (!mobileMessageDBService) {
      NS_ERROR("Can't find MobileMessageDBService!");
    }

    mobileMessageDBService->ClearMessageList(mListId);
  }
}

void
SmsCursor::Disconnect()
{
  NS_ASSERTION(!mMessage, "mMessage shouldn't be set!");

  mRequest = nullptr;
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

  nsRefPtr<SmsRequest> request = static_cast<SmsRequest*>(mRequest.get());

  mMessage = nullptr;
  request->Reset();

  nsCOMPtr<nsIMobileMessageDatabaseService> mobileMessageDBService =
    do_GetService(MOBILE_MESSAGE_DATABASE_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(mobileMessageDBService, NS_ERROR_FAILURE);

  nsCOMPtr<nsISmsRequest> forwarder = new SmsRequestForwarder(request);
  mobileMessageDBService->GetNextMessageInList(mListId, forwarder);

  
  
  request.forget();
  return NS_OK;
}

} 
} 
} 

