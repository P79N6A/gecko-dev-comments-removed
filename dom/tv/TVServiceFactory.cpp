




#include "mozilla/dom/FakeTVService.h"
#include "mozilla/dom/TVListeners.h"
#include "mozilla/Preferences.h"
#include "nsITVService.h"
#include "nsServiceManagerUtils.h"
#include "TVServiceFactory.h"

namespace mozilla {
namespace dom {

 already_AddRefed<FakeTVService>
TVServiceFactory::CreateFakeTVService()
{
  nsRefPtr<FakeTVService> service = new FakeTVService();
  return service.forget();
}

 already_AddRefed<nsITVService>
TVServiceFactory::AutoCreateTVService()
{
  nsresult rv;
  nsCOMPtr<nsITVService> service = do_CreateInstance(TV_SERVICE_CONTRACTID);
  if (!service) {
    
    service = do_CreateInstance(FAKE_TV_SERVICE_CONTRACTID, &rv);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return nullptr;
    }
  }

  rv = service->SetSourceListener(new TVSourceListener());
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return nullptr;
  }

  return service.forget();
}

} 
} 
