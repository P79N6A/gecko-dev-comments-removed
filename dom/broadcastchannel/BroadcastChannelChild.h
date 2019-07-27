



#ifndef mozilla_dom_BroadcastChannelChild_h
#define mozilla_dom_BroadcastChannelChild_h

#include "mozilla/dom/EventTarget.h"
#include "mozilla/dom/PBroadcastChannelChild.h"

namespace mozilla {

namespace ipc {
class BackgroundChildImpl;
}

namespace dom {

class EventTarget;

class BroadcastChannelChild MOZ_FINAL : public PBroadcastChannelChild
{
  friend class mozilla::ipc::BackgroundChildImpl;

public:
  NS_INLINE_DECL_REFCOUNTING(BroadcastChannelChild)

  void SetEventTarget(EventTarget* aEventTarget)
  {
    mEventTarget = aEventTarget;
  }

  virtual bool RecvNotify(const nsString& aMessage) MOZ_OVERRIDE;

  bool IsActorDestroyed() const
  {
    return mActorDestroyed;
  }

private:
  BroadcastChannelChild(const nsAString& aOrigin,
                        const nsAString& aChannel);

  ~BroadcastChannelChild();

  void Notify(JSContext* aCx, const nsString& aMessage);

  void ActorDestroy(ActorDestroyReason aWhy);

  
  
  EventTarget* mEventTarget;

  nsString mOrigin;
  nsString mChannel;

  bool mActorDestroyed;
};

} 
} 

#endif 
