



#ifndef mozilla_dom_BroadcastChannelService_h
#define mozilla_dom_BroadcastChannelService_h

#include "nsISupportsImpl.h"
#include "nsHashKeys.h"
#include "nsTHashtable.h"

#ifdef XP_WIN
#undef PostMessage
#endif

namespace mozilla {
namespace dom {

class BroadcastChannelParent;
class ClonedMessageData;

class BroadcastChannelService final
{
public:
  NS_INLINE_DECL_REFCOUNTING(BroadcastChannelService)

  static already_AddRefed<BroadcastChannelService> GetOrCreate();

  void RegisterActor(BroadcastChannelParent* aParent);
  void UnregisterActor(BroadcastChannelParent* aParent);

  void PostMessage(BroadcastChannelParent* aParent,
                   const ClonedMessageData& aData,
                   const nsAString& aOrigin,
                   const nsAString& aChannel,
                   bool aPrivateBrowsing);

private:
  BroadcastChannelService();
  ~BroadcastChannelService();

  nsTHashtable<nsPtrHashKey<BroadcastChannelParent>> mAgents;
};

} 
} 

#endif 
