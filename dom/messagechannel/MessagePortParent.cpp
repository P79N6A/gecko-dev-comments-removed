




#include "MessagePortParent.h"
#include "MessagePortService.h"
#include "SharedMessagePortMessage.h"
#include "mozilla/unused.h"

namespace mozilla {
namespace dom {

MessagePortParent::MessagePortParent(const nsID& aUUID)
  : mService(MessagePortService::GetOrCreate())
  , mUUID(aUUID)
  , mEntangled(false)
  , mCanSendData(true)
{
  MOZ_ASSERT(mService);
}

MessagePortParent::~MessagePortParent()
{
  MOZ_ASSERT(!mService);
  MOZ_ASSERT(!mEntangled);
}

bool
MessagePortParent::Entangle(const nsID& aDestinationUUID,
                            const uint32_t& aSequenceID)
{
  if (!mService) {
    NS_WARNING("Entangle is called after a shutdown!");
    return false;
  }

  MOZ_ASSERT(!mEntangled);

  return mService->RequestEntangling(this, aDestinationUUID, aSequenceID);
}

bool
MessagePortParent::RecvPostMessages(nsTArray<MessagePortMessage>&& aMessages)
{
  
  FallibleTArray<nsRefPtr<SharedMessagePortMessage>> messages;
  if (NS_WARN_IF(
      !SharedMessagePortMessage::FromMessagesToSharedParent(aMessages,
                                                            messages))) {
    return false;
  }

  if (!mEntangled) {
    return false;
  }

  if (!mService) {
    NS_WARNING("Entangle is called after a shutdown!");
    return false;
  }

  if (messages.IsEmpty()) {
    return false;
  }

  return mService->PostMessages(this, messages);
}

bool
MessagePortParent::RecvDisentangle(nsTArray<MessagePortMessage>&& aMessages)
{
  
  FallibleTArray<nsRefPtr<SharedMessagePortMessage>> messages;
  if (NS_WARN_IF(
      !SharedMessagePortMessage::FromMessagesToSharedParent(aMessages,
                                                            messages))) {
    return false;
  }

  if (!mEntangled) {
    return false;
  }

  if (!mService) {
    NS_WARNING("Entangle is called after a shutdown!");
    return false;
  }

  if (!mService->DisentanglePort(this, messages)) {
    return false;
  }

  CloseAndDelete();
  return true;
}

bool
MessagePortParent::RecvStopSendingData()
{
  if (!mEntangled) {
    return true;
  }

  mCanSendData = false;
  unused << SendStopSendingDataConfirmed();
  return true;
}

bool
MessagePortParent::RecvClose()
{
  if (mService) {
    MOZ_ASSERT(mEntangled);

    if (!mService->ClosePort(this)) {
      return false;
    }

    Close();
  }

  MOZ_ASSERT(!mEntangled);

  unused << Send__delete__(this);
  return true;
}

void
MessagePortParent::ActorDestroy(ActorDestroyReason aWhy)
{
  if (mService && mEntangled) {
    
    
    nsRefPtr<MessagePortService> kungFuDeathGrip = mService;
    mService->ParentDestroy(this);
  }
}

bool
MessagePortParent::Entangled(const nsTArray<MessagePortMessage>& aMessages)
{
  MOZ_ASSERT(!mEntangled);
  mEntangled = true;
  return SendEntangled(aMessages);
}

void
MessagePortParent::CloseAndDelete()
{
  Close();
  unused << Send__delete__(this);
}

void
MessagePortParent::Close()
{
  mService = nullptr;
  mEntangled = false;
}

} 
} 
