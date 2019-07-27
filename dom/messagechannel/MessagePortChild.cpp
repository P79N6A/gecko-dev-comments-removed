




#include "MessagePortChild.h"
#include "MessagePort.h"
#include "mozilla/dom/MessageEvent.h"
#include "mozilla/ipc/PBackgroundChild.h"

namespace mozilla {
namespace dom {

bool
MessagePortChild::RecvStopSendingDataConfirmed()
{
  MOZ_ASSERT(mPort);
  mPort->StopSendingDataConfirmed();
  MOZ_ASSERT(!mPort);
  return true;
}

bool
MessagePortChild::RecvEntangled(nsTArray<MessagePortMessage>&& aMessages)
{
  MOZ_ASSERT(mPort);
  mPort->Entangled(aMessages);
  return true;
}

bool
MessagePortChild::RecvReceiveData(nsTArray<MessagePortMessage>&& aMessages)
{
  MOZ_ASSERT(mPort);
  mPort->MessagesReceived(aMessages);
  return true;
}

void
MessagePortChild::ActorDestroy(ActorDestroyReason aWhy)
{
  if (mPort) {
    mPort->Closed();
    MOZ_ASSERT(!mPort);
  }
}

} 
} 
