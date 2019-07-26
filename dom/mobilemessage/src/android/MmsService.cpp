




#include "MobileMessageCallback.h"
#include "MmsService.h"
#include "AndroidBridge.h"
#include "jsapi.h"

namespace mozilla {
namespace dom {
namespace mobilemessage {

NS_IMPL_ISUPPORTS1(MmsService, nsIMmsService)

NS_IMETHODIMP
MmsService::Send(const JS::Value& aParameters,
                 nsIMobileMessageCallback *aRequest)
{
  NS_NOTYETIMPLEMENTED("Implement me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

} 
} 
} 
