




































#include "mozilla/dom/ContentChild.h"
#include "SmsIPCService.h"
#include "nsXULAppAPI.h"
#include "mozilla/dom/sms/SmsChild.h"

namespace mozilla {
namespace dom {
namespace sms {

PSmsChild* SmsIPCService::sSmsChild = nsnull;

NS_IMPL_ISUPPORTS1(SmsIPCService, nsISmsService)

 PSmsChild*
SmsIPCService::GetSmsChild()
{
  if (!sSmsChild) {
    sSmsChild = ContentChild::GetSingleton()->SendPSmsConstructor();
  }

  return sSmsChild;
}

NS_IMETHODIMP
SmsIPCService::HasSupport(bool* aHasSupport)
{
  GetSmsChild()->SendHasSupport(aHasSupport);

  return NS_OK;
}

NS_IMETHODIMP
SmsIPCService::GetNumberOfMessagesForText(const nsAString& aText, PRUint16* aResult)
{
  GetSmsChild()->SendGetNumberOfMessagesForText(nsString(aText), aResult);

  return NS_OK;
}

} 
} 
} 
