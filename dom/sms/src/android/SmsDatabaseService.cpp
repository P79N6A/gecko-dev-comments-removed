




































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

} 
} 
} 
