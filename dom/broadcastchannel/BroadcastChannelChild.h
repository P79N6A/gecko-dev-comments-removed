



#ifndef mozilla_dom_BroadcastChannelChild_h
#define mozilla_dom_BroadcastChannelChild_h

#include "mozilla/dom/PBroadcastChannelChild.h"

namespace mozilla {

namespace ipc {
class BackgroundChildImpl;
}

namespace dom {

class BroadcastChannel;

class BroadcastChannelChild MOZ_FINAL : public PBroadcastChannelChild
{
  friend class mozilla::ipc::BackgroundChildImpl;

public:
  NS_INLINE_DECL_REFCOUNTING(BroadcastChannelChild)

  void SetParent(BroadcastChannel* aBC)
  {
    mBC = aBC;
  }

  virtual bool RecvNotify(const ClonedMessageData& aData) MOZ_OVERRIDE;

  bool IsActorDestroyed() const
  {
    return mActorDestroyed;
  }

private:
  BroadcastChannelChild(const nsAString& aOrigin,
                        const nsAString& aChannel);

  ~BroadcastChannelChild();

  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  
  
  BroadcastChannel* mBC;

  nsString mOrigin;
  nsString mChannel;

  bool mActorDestroyed;
};

} 
} 

#endif 
