





#include "BroadcastChannelParent.h"
#include "BroadcastChannelService.h"
#include "mozilla/dom/File.h"
#include "mozilla/dom/ipc/BlobParent.h"
#include "mozilla/ipc/BackgroundParent.h"
#include "mozilla/unused.h"
#include "nsIScriptSecurityManager.h"

namespace mozilla {

using namespace ipc;

namespace dom {

BroadcastChannelParent::BroadcastChannelParent(
                                            const PrincipalInfo& aPrincipalInfo,
                                            const nsAString& aOrigin,
                                            const nsAString& aChannel,
                                            bool aPrivateBrowsing)
  : mService(BroadcastChannelService::GetOrCreate())
  , mOrigin(aOrigin)
  , mChannel(aChannel)
  , mAppId(nsIScriptSecurityManager::UNKNOWN_APP_ID)
  , mIsInBrowserElement(false)
  , mPrivateBrowsing(aPrivateBrowsing)
{
  AssertIsOnBackgroundThread();
  mService->RegisterActor(this);

  if (aPrincipalInfo.type() ==PrincipalInfo::TContentPrincipalInfo) {
    const ContentPrincipalInfo& info =
      aPrincipalInfo.get_ContentPrincipalInfo();
    mAppId = info.appId();
    mIsInBrowserElement = info.isInBrowserElement();
  }
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

  mService->PostMessage(this, aData, mOrigin, mAppId, mIsInBrowserElement,
                        mChannel, mPrivateBrowsing);
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
                                        uint64_t aAppId,
                                        bool aInBrowserElement,
                                        const nsString& aChannel,
                                        bool aPrivateBrowsing)
{
  AssertIsOnBackgroundThread();

  if (aOrigin == mOrigin &&
      aAppId == mAppId &&
      aInBrowserElement == mIsInBrowserElement &&
      aChannel == mChannel &&
      aPrivateBrowsing == mPrivateBrowsing) {
    
    
    if (aData.blobsParent().IsEmpty() ||
        static_cast<BlobParent*>(aData.blobsParent()[0])->GetBackgroundManager() == Manager()) {
      unused << SendNotify(aData);
      return;
    }

    
    ClonedMessageData newData(aData);

    
    for (uint32_t i = 0, len = newData.blobsParent().Length(); i < len; ++i) {
      nsRefPtr<BlobImpl> impl =
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
