




#include "ShutdownTracker.h"

#include "mozilla/Services.h"
#include "nsAutoPtr.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"

namespace mozilla {
namespace image {

class ShutdownTrackerImpl;






static bool sShutdownHasStarted = false;






struct ShutdownObserver : public nsIObserver
{
  NS_DECL_ISUPPORTS

  NS_IMETHOD Observe(nsISupports*, const char* aTopic, const char16_t*)
  {
    if (strcmp(aTopic, "xpcom-shutdown") != 0) {
      return NS_OK;
    }

    nsCOMPtr<nsIObserverService> os = services::GetObserverService();
    if (os) {
      os->RemoveObserver(this, "xpcom-shutdown");
    }

    sShutdownHasStarted = true;
    return NS_OK;
  }

private:
  virtual ~ShutdownObserver() { }
};

NS_IMPL_ISUPPORTS(ShutdownObserver, nsIObserver)






 void
ShutdownTracker::Initialize()
{
  nsCOMPtr<nsIObserverService> os = services::GetObserverService();
  if (os) {
    os->AddObserver(new ShutdownObserver, "xpcom-shutdown", false);
  }
}

 bool
ShutdownTracker::ShutdownHasStarted()
{
  return sShutdownHasStarted;
}

} 
} 
