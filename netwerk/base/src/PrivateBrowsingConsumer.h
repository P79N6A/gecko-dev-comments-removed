




































#ifndef mozilla_net_PrivateBrowsingConsumer_h
#define mozilla_net_PrivateBrowsingConsumer_h

#include "nsIPrivateBrowsingConsumer.h"
#include "nsILoadContext.h"
#include "nsNetUtil.h"
#include "nsXULAppAPI.h"

namespace mozilla {
namespace net {

class PrivateBrowsingConsumer : public nsIPrivateBrowsingConsumer
{
 public:
  PrivateBrowsingConsumer(nsIChannel* aSelf) : mSelf(aSelf), mUsingPB(false), mOverride(false) {}

  NS_IMETHOD GetUsingPrivateBrowsing(bool *aUsingPB)
  {
    *aUsingPB = (mOverride ? mUsingPB : UsingPrivateBrowsingInternal());
    return NS_OK;
  }
  
  void OverridePrivateBrowsing(bool aUsingPrivateBrowsing)
  {
    MOZ_ASSERT(!mOverride);
    MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
    mOverride = true;
    mUsingPB = aUsingPrivateBrowsing;
  }

 protected:
  bool UsingPrivateBrowsingInternal()
  {
    nsCOMPtr<nsILoadContext> loadContext;
    NS_QueryNotificationCallbacks(mSelf, loadContext);
    return loadContext && loadContext->UsePrivateBrowsing();
  }

 private:
   nsIChannel* mSelf;

  
  
  
  bool mUsingPB;
  bool mOverride;
};

}
}

#endif 
