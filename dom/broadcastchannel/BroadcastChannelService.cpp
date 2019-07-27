




#include "BroadcastChannelService.h"
#include "BroadcastChannelParent.h"
#include "mozilla/ipc/BackgroundParent.h"

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

namespace {

struct MOZ_STACK_CLASS PostMessageData MOZ_FINAL
{
  PostMessageData(BroadcastChannelParent* aParent,
                  const BroadcastChannelMessageData& aData,
                  const nsAString& aOrigin,
                  const nsAString& aChannel)
    : mParent(aParent)
    , mData(aData)
    , mOrigin(aOrigin)
    , mChannel(aChannel)
  {
    MOZ_ASSERT(aParent);
    MOZ_COUNT_CTOR(PostMessageData);
  }

  ~PostMessageData()
  {
    MOZ_COUNT_DTOR(PostMessageData);
  }

  BroadcastChannelParent* mParent;
  const BroadcastChannelMessageData& mData;
  const nsString mOrigin;
  const nsString mChannel;
};

PLDHashOperator
PostMessageEnumerator(nsPtrHashKey<BroadcastChannelParent>* aKey, void* aPtr)
{
  AssertIsOnBackgroundThread();

  auto* data = static_cast<PostMessageData*>(aPtr);
  BroadcastChannelParent* parent = aKey->GetKey();
  MOZ_ASSERT(parent);

  if (parent != data->mParent) {
    parent->CheckAndDeliver(data->mData, data->mOrigin, data->mChannel);
  }

  return PL_DHASH_NEXT;
}

} 

void
BroadcastChannelService::PostMessage(BroadcastChannelParent* aParent,
                                     const BroadcastChannelMessageData& aData,
                                     const nsAString& aOrigin,
                                     const nsAString& aChannel)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aParent);
  MOZ_ASSERT(mAgents.Contains(aParent));

  PostMessageData data(aParent, aData, aOrigin, aChannel);
  mAgents.EnumerateEntries(PostMessageEnumerator, &data);
}

} 
} 
