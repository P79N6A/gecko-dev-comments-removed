



#ifndef mozilla_dom_BroadcastChannelParent_h
#define mozilla_dom_BroadcastChannelParent_h

#include "mozilla/dom/PBroadcastChannelParent.h"

namespace mozilla {

namespace ipc {
class BackgroundParentImpl;
}

namespace dom {

class BroadcastChannelService;

class BroadcastChannelParent MOZ_FINAL : public PBroadcastChannelParent
{
  friend class mozilla::ipc::BackgroundParentImpl;

public:
  void CheckAndDeliver(const BroadcastChannelMessageData& aData,
                       const nsString& aOrigin,
                       const nsString& aChannel);

private:
  BroadcastChannelParent(const nsAString& aOrigin,
                         const nsAString& aChannel);
  ~BroadcastChannelParent();

  virtual bool
  RecvPostMessage(const BroadcastChannelMessageData& aData) MOZ_OVERRIDE;

  virtual bool RecvClose() MOZ_OVERRIDE;

  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  nsRefPtr<BroadcastChannelService> mService;
  nsString mOrigin;
  nsString mChannel;
};

} 
} 

#endif 
