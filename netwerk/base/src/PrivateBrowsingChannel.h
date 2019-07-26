





#ifndef mozilla_net_PrivateBrowsingChannel_h__
#define mozilla_net_PrivateBrowsingChannel_h__

#include "nsIPrivateBrowsingChannel.h"
#include "nsCOMPtr.h"
#include "nsILoadGroup.h"
#include "nsILoadContext.h"

namespace mozilla {
namespace net {

template <class Channel>
class PrivateBrowsingChannel : public nsIPrivateBrowsingChannel
{
public:
  PrivateBrowsingChannel() :
    mPrivateBrowsingOverriden(false),
    mPrivateBrowsing(false)
  {
  }

  NS_IMETHOD SetPrivate(bool aPrivate)
  {
      
      
      
      nsILoadGroup* loadGroup = static_cast<Channel*>(this)->mLoadGroup;
      nsCOMPtr<nsILoadContext> loadContext;
      if (!loadGroup) {
        NS_QueryNotificationCallbacks(static_cast<Channel*>(this), loadContext);
      }
      MOZ_ASSERT(!loadGroup && !loadContext);
      if (loadGroup || loadContext) {
        return NS_ERROR_FAILURE;
      }

      mPrivateBrowsingOverriden = true;
      mPrivateBrowsing = aPrivate;
      return NS_OK;
  }

  NS_IMETHOD IsPrivateModeOverriden(bool* aValue, bool *aResult)
  {
      NS_ENSURE_ARG_POINTER(aValue);
      NS_ENSURE_ARG_POINTER(aResult);
      *aResult = mPrivateBrowsingOverriden;
      if (mPrivateBrowsingOverriden) {
          *aValue = mPrivateBrowsing;
      }
      return NS_OK;
  }

  bool CanSetCallbacks() const
  {
      
      
      
      MOZ_ASSERT(!mPrivateBrowsingOverriden);
      return !mPrivateBrowsingOverriden;
  }

  bool CanSetLoadGroup() const
  {
      
      return CanSetCallbacks();
  }

protected:
  bool mPrivateBrowsingOverriden;
  bool mPrivateBrowsing;
};

}
}

#endif

