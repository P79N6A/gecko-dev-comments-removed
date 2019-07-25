




































#include "SmsManager.h"
#include "nsIDOMClassInfo.h"
#include "nsISmsService.h"

DOMCI_DATA(MozSmsManager, mozilla::dom::sms::SmsManager)

namespace mozilla {
namespace dom {
namespace sms {

NS_INTERFACE_MAP_BEGIN(SmsManager)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMozSmsManager)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(MozSmsManager)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(SmsManager)
NS_IMPL_RELEASE(SmsManager)

NS_IMETHODIMP
SmsManager::GetNumberOfMessagesForText(const nsAString& aText, PRUint16* aResult)
{
  nsCOMPtr<nsISmsService> smsService = do_GetService(SMSSERVICE_CONTRACTID);
  NS_ENSURE_TRUE(smsService, NS_OK);

  smsService->GetNumberOfMessagesForText(aText, aResult);

  return NS_OK;
}

NS_IMETHODIMP
SmsManager::Send(const nsAString& aNumber, const nsAString& aMessage)
{
  nsCOMPtr<nsISmsService> smsService = do_GetService(SMSSERVICE_CONTRACTID);
  NS_ENSURE_TRUE(smsService, NS_OK);

  smsService->Send(aNumber, aMessage);

  return NS_OK;
}

} 
} 
} 
