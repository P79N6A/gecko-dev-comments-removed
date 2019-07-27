





#include "MLSFallback.h"
#include "nsGeoPosition.h"
#include "nsIGeolocationProvider.h"
#include "nsServiceManagerUtils.h"

NS_IMPL_ISUPPORTS(MLSFallback, nsITimerCallback)

MLSFallback::MLSFallback(uint32_t delay)
: mDelayMs(delay)
{
}

MLSFallback::~MLSFallback()
{
}

nsresult
MLSFallback::Startup(nsIGeolocationUpdate* aWatcher)
{
  if (mHandoffTimer || mMLSFallbackProvider) {
    return NS_OK;
  }

  mUpdateWatcher = aWatcher;
  nsresult rv;
  mHandoffTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mHandoffTimer->InitWithCallback(this, mDelayMs, nsITimer::TYPE_ONE_SHOT);
  return rv;
}

nsresult
MLSFallback::Shutdown()
{
  mUpdateWatcher = nullptr;

  if (mHandoffTimer) {
    mHandoffTimer->Cancel();
    mHandoffTimer = nullptr;
  }

  nsresult rv = NS_OK;
  if (mMLSFallbackProvider) {
    rv = mMLSFallbackProvider->Shutdown();
    mMLSFallbackProvider = nullptr;
  }
  return rv;
}

NS_IMETHODIMP
MLSFallback::Notify(nsITimer* aTimer)
{
  return CreateMLSFallbackProvider();
}

nsresult
MLSFallback::CreateMLSFallbackProvider()
{
  if (mMLSFallbackProvider || !mUpdateWatcher) {
    return NS_OK;
  }

  nsresult rv;
  mMLSFallbackProvider = do_CreateInstance("@mozilla.org/geolocation/mls-provider;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mMLSFallbackProvider) {
    rv = mMLSFallbackProvider->Startup();
    if (NS_SUCCEEDED(rv)) {
      mMLSFallbackProvider->Watch(mUpdateWatcher);
    }
  }
  mUpdateWatcher = nullptr;
  return rv;
}

