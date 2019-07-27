



#ifndef mozilla_dom_BroadcastChannelChild_h
#define mozilla_dom_BroadcastChannelChild_h

#include "mozilla/dom/PBroadcastChannelChild.h"

namespace mozilla {

namespace ipc {
class BackgroundChildImpl;
}

namespace dom {

class BroadcastChannel;

class BroadcastChannelChild final : public PBroadcastChannelChild
{
  friend class mozilla::ipc::BackgroundChildImpl;

public:
  NS_INLINE_DECL_REFCOUNTING(BroadcastChannelChild)

  void SetParent(BroadcastChannel* aBC)
  {
    mBC = aBC;
  }

  virtual bool RecvNotify(const ClonedMessageData& aData) override;

  bool IsActorDestroyed() const
  {
    return mActorDestroyed;
  }

private:
  BroadcastChannelChild(const nsAString& aOrigin);
  ~BroadcastChannelChild();

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

  
  
  BroadcastChannel* mBC;

  nsString mOrigin;

  bool mActorDestroyed;
};

} 
} 

#endif 
