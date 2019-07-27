





#include "BroadcastChannelService.h"
#include "BroadcastChannelParent.h"
#include "mozilla/dom/File.h"
#include "mozilla/dom/ipc/BlobParent.h"
#include "mozilla/ipc/BackgroundParent.h"

#ifdef XP_WIN
#undef PostMessage
#endif

namespace mozilla {

using namespace ipc;

namespace dom {

namespace {

BroadcastChannelService* sInstance = nullptr;

} 

BroadcastChannelService::BroadcastChannelService()
{
  AssertIsOnBackgroundThread();

  
  MOZ_ASSERT(!sInstance);
  sInstance = this;
}

BroadcastChannelService::~BroadcastChannelService()
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(sInstance == this);
  MOZ_ASSERT(mAgents.Count() == 0);

  sInstance = nullptr;
}


already_AddRefed<BroadcastChannelService>
BroadcastChannelService::GetOrCreate()
{
  AssertIsOnBackgroundThread();

  nsRefPtr<BroadcastChannelService> instance = sInstance;
  if (!instance) {
    instance = new BroadcastChannelService();
  }
  return instance.forget();
}

void
BroadcastChannelService::RegisterActor(BroadcastChannelParent* aParent)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aParent);
  MOZ_ASSERT(!mAgents.Contains(aParent));

  mAgents.PutEntry(aParent);
}

void
BroadcastChannelService::UnregisterActor(BroadcastChannelParent* aParent)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aParent);
  MOZ_ASSERT(mAgents.Contains(aParent));

  mAgents.RemoveEntry(aParent);
}

void
BroadcastChannelService::PostMessage(BroadcastChannelParent* aParent,
                                     const ClonedMessageData& aData,
                                     const nsACString& aOrigin,
                                     uint64_t aAppId,
                                     bool aIsInBrowserElement,
                                     const nsAString& aChannel,
                                     bool aPrivateBrowsing)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aParent);
  MOZ_ASSERT(mAgents.Contains(aParent));

  
  nsTArray<nsRefPtr<BlobImpl>> blobs;
  if (!aData.blobsParent().IsEmpty()) {
    blobs.SetCapacity(aData.blobsParent().Length());

    for (uint32_t i = 0, len = aData.blobsParent().Length(); i < len; ++i) {
      nsRefPtr<BlobImpl> impl =
        static_cast<BlobParent*>(aData.blobsParent()[i])->GetBlobImpl();
     MOZ_ASSERT(impl);
     blobs.AppendElement(impl);
    }
  }

  for (auto iter = mAgents.Iter(); !iter.Done(); iter.Next()) {
    BroadcastChannelParent* parent = iter.Get()->GetKey();
    MOZ_ASSERT(parent);

    if (parent != aParent) {
      parent->CheckAndDeliver(aData, PromiseFlatCString(aOrigin),
                              aAppId, aIsInBrowserElement,
                              PromiseFlatString(aChannel), aPrivateBrowsing);
    }
  }
}

} 
} 
