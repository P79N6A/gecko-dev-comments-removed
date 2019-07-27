



#ifndef mozilla_dom_BroadcastChannelService_h
#define mozilla_dom_BroadcastChannelService_h

#include "nsISupportsImpl.h"
#include "nsHashKeys.h"
#include "nsTHashtable.h"

namespace mozilla {
namespace dom {

class BroadcastChannelParent;
class BroadcastChannelMessageData;

class BroadcastChannelService MOZ_FINAL
{
public:
  NS_INLINE_DECL_REFCOUNTING(BroadcastChannelService)

  static already_AddRefed<BroadcastChannelService> GetOrCreate();

  void RegisterActor(BroadcastChannelParent* aParent);
  void UnregisterActor(BroadcastChannelParent* aParent);

  void PostMessage(BroadcastChannelParent* aParent,
                   const BroadcastChannelMessageData& aData,
                   const nsAString& aOrigin,
                   const nsAString& aChannel);

private:
  BroadcastChannelService();
  ~BroadcastChannelService();

  nsTHashtable<nsPtrHashKey<BroadcastChannelParent>> mAgents;
};

} 
} 

#endif 
