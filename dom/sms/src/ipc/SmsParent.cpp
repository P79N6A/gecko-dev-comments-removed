




































#include "SmsParent.h"
#include "nsISmsService.h"

namespace mozilla {
namespace dom {
namespace sms {

bool
SmsParent::RecvHasSupport(bool* aHasSupport)
{
  *aHasSupport = false;

  nsCOMPtr<nsISmsService> smsService = do_GetService(SMSSERVICE_CONTRACTID);
  NS_ENSURE_TRUE(smsService, true);

  smsService->HasSupport(aHasSupport);
  return true;
}

bool
SmsParent::RecvGetNumberOfMessagesForText(const nsString& aText, PRUint16* aResult)
{
  *aResult = 0;

  nsCOMPtr<nsISmsService> smsService = do_GetService(SMSSERVICE_CONTRACTID);
  NS_ENSURE_TRUE(smsService, true);

  smsService->GetNumberOfMessagesForText(aText, aResult);
  return true;
}

bool
SmsParent::RecvSendMessage(const nsString& aNumber, const nsString& aMessage)
{
  nsCOMPtr<nsISmsService> smsService = do_GetService(SMSSERVICE_CONTRACTID);
  NS_ENSURE_TRUE(smsService, true);

  smsService->Send(aNumber, aMessage);
  return true;
}

} 
} 
} 
