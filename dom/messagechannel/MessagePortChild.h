



#ifndef mozilla_dom_MessagePortChild_h
#define mozilla_dom_MessagePortChild_h

#include "mozilla/Assertions.h"
#include "mozilla/dom/PMessagePortChild.h"
#include "nsISupportsImpl.h"

namespace mozilla {
namespace dom {

class MessagePort;

class MessagePortChild final : public PMessagePortChild
{
public:
  NS_INLINE_DECL_REFCOUNTING(MessagePortChild)

  MessagePortChild() {}

  void SetPort(MessagePort* aPort)
  {
    mPort = aPort;
  }

private:
  ~MessagePortChild()
  {
    MOZ_ASSERT(!mPort);
  }

  virtual bool
  RecvEntangled(nsTArray<MessagePortMessage>&& aMessages) override;

  virtual bool
  RecvReceiveData(nsTArray<MessagePortMessage>&& aMessages) override;

  virtual bool RecvStopSendingDataConfirmed() override;

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

  
  MessagePort* mPort;
};

} 
} 

#endif 
