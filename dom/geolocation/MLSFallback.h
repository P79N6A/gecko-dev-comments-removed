




#include "nsCOMPtr.h"
#include "nsITimer.h"

class nsIGeolocationUpdate;
class nsIGeolocationProvider;


















class MLSFallback : public nsITimerCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITIMERCALLBACK

  explicit MLSFallback(uint32_t delayMs = 2000);
  nsresult Startup(nsIGeolocationUpdate* aWatcher);
  nsresult Shutdown();

private:
  nsresult CreateMLSFallbackProvider();
  virtual ~MLSFallback();
  nsCOMPtr<nsITimer> mHandoffTimer;
  nsCOMPtr<nsIGeolocationProvider> mMLSFallbackProvider;
  nsCOMPtr<nsIGeolocationUpdate> mUpdateWatcher;
  const uint32_t mDelayMs;
};

