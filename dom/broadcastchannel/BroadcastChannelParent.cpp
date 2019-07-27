




#include "BroadcastChannelParent.h"
#include "BroadcastChannelService.h"
#include "mozilla/ipc/BackgroundParent.h"
#include "mozilla/unused.h"

namespace mozilla {

using namespace ipc;

namespace dom {

BroadcastChannelParent::BroadcastChannelParent(
                                            const nsAString& aOrigin,
                                            const nsAString& aChannel)
  : mService(BroadcastChannelService::GetOrCreate())
  , mOrigin(aOrigin)
  , mChannel(aChannel)
{
  AssertIsOnBackgroundThread();
  mService->RegisterActor(this);
}

BroadcastChannelParent::~BroadcastChannelParent()
{
  AssertIsOnBackgroundThread();
}

bool
BroadcastChannelParent::RecvPostMessage(const nsString& aMessage)
{
  AssertIsOnBackgroundThread();

  if (NS_WARN_IF(!mService)) {
    return false;
  }

  mService->PostMessage(this, aMessage, mOrigin, mChannel);
  return true;
}

bool
BroadcastChannelParent::RecvClose()
{
  AssertIsOnBackgroundThread();

  if (NS_WARN_IF(!mService)) {
    return false;
  }

  mService->UnregisterActor(this);
  mService = nullptr;

  unused << Send__delete__(this);

  return true;
}

void
BroadcastChannelParent::ActorDestroy(ActorDestroyReason aWhy)
{
  AssertIsOnBackgroundThread();

  if (mService) {
    
    
    mService->UnregisterActor(this);
  }
}

void
BroadcastChannelParent::CheckAndDeliver(const nsString& aMessage,
                                        const nsString& aOrigin,
                                        const nsString& aChannel)
{
  AssertIsOnBackgroundThread();

  if (aOrigin == mOrigin && aChannel == mChannel) {
    unused << SendNotify(aMessage);
  }
}

} 
} 
