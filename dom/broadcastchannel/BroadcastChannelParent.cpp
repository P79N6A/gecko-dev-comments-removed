




#include "BroadcastChannelParent.h"
#include "BroadcastChannelService.h"
#include "mozilla/dom/File.h"
#include "mozilla/dom/ipc/BlobParent.h"
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
BroadcastChannelParent::RecvPostMessage(const ClonedMessageData& aData)
{
  AssertIsOnBackgroundThread();

  if (NS_WARN_IF(!mService)) {
    return false;
  }

  mService->PostMessage(this, aData, mOrigin, mChannel);
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
BroadcastChannelParent::CheckAndDeliver(const ClonedMessageData& aData,
                                        const nsString& aOrigin,
                                        const nsString& aChannel)
{
  AssertIsOnBackgroundThread();

  if (aOrigin == mOrigin && aChannel == mChannel) {
    
    
    if (aData.blobsParent().IsEmpty() ||
        static_cast<BlobParent*>(aData.blobsParent()[0])->GetBackgroundManager() == Manager()) {
      unused << SendNotify(aData);
      return;
    }

    
    ClonedMessageData newData(aData);

    
    for (uint32_t i = 0, len = newData.blobsParent().Length(); i < len; ++i) {
      nsRefPtr<FileImpl> impl =
        static_cast<BlobParent*>(newData.blobsParent()[i])->GetBlobImpl();

      PBlobParent* blobParent =
        BackgroundParent::GetOrCreateActorForBlobImpl(Manager(), impl);
      if (!blobParent) {
        return;
      }

      newData.blobsParent()[i] = blobParent;
    }

    unused << SendNotify(newData);
  }
}

} 
} 
