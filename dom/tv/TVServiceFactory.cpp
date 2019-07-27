




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
  nsresult rv = service->SetSourceListener(new TVSourceListener());
  NS_ENSURE_SUCCESS(rv, nullptr);

  return service.forget();
}

} 
} 
