




#include "FakeInputPortService.h"
#include "InputPortListeners.h"
#include "InputPortServiceFactory.h"
#include "mozilla/Preferences.h"
#include "nsIInputPortService.h"
#include "nsServiceManagerUtils.h"

namespace mozilla {
namespace dom {

 already_AddRefed<FakeInputPortService>
InputPortServiceFactory::CreateFakeInputPortService()
{
  nsRefPtr<FakeInputPortService> service = new FakeInputPortService();
  return service.forget();
}

 already_AddRefed<nsIInputPortService>
InputPortServiceFactory::AutoCreateInputPortService()
{
  nsresult rv;
  nsCOMPtr<nsIInputPortService> service =
    do_GetService(INPUTPORT_SERVICE_CONTRACTID);
  if (!service) {
    
    service = do_GetService(FAKE_INPUTPORT_SERVICE_CONTRACTID, &rv);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return nullptr;
    }
  }
  MOZ_ASSERT(service);
  rv = service->SetInputPortListener(new InputPortListener());
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return nullptr;
  }

  return service.forget();
}

} 
} 
