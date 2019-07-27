





#include "MediaSystemResourceManagerParent.h"
#include "mozilla/layers/CompositorParent.h"
#include "mozilla/unused.h"

#include "MediaSystemResourceService.h"

using namespace mozilla::layers;

namespace mozilla {

 StaticRefPtr<MediaSystemResourceService> MediaSystemResourceService::sSingleton;

 MediaSystemResourceService*
MediaSystemResourceService::Get()
{
  if (sSingleton) {
    return sSingleton;
  }
  Init();
  return sSingleton;
}

 void
MediaSystemResourceService::Init()
{
  if (!sSingleton) {
    sSingleton = new MediaSystemResourceService();
  }
}

 void
MediaSystemResourceService::Shutdown()
{
  if (sSingleton) {
    sSingleton->Destroy();
    sSingleton = nullptr;
  }
}

MediaSystemResourceService::MediaSystemResourceService()
  : mDestroyed(false)
{
  MOZ_ASSERT(CompositorParent::IsInCompositorThread());
#ifdef MOZ_WIDGET_GONK
  
  
  enum
  {
    VIDEO_DECODER_COUNT = 1,
    VIDEO_ENCODER_COUNT = 1
  };

  MediaSystemResource* resource;

  resource = new MediaSystemResource(VIDEO_DECODER_COUNT);
  mResources.Put(static_cast<uint32_t>(MediaSystemResourceType::VIDEO_DECODER), resource);

  resource = new MediaSystemResource(VIDEO_ENCODER_COUNT);
  mResources.Put(static_cast<uint32_t>(MediaSystemResourceType::VIDEO_ENCODER), resource);
#endif
}

MediaSystemResourceService::~MediaSystemResourceService()
{
}

void
MediaSystemResourceService::Destroy()
{
  mDestroyed = true;
}

void
MediaSystemResourceService::Acquire(media::MediaSystemResourceManagerParent* aParent,
                                    uint32_t aId,
                                    MediaSystemResourceType aResourceType,
                                    bool aWillWait)
{
  MOZ_ASSERT(CompositorParent::IsInCompositorThread());
  MOZ_ASSERT(aParent);

  if (mDestroyed) {
    return;
  }

  MediaSystemResource* resource = mResources.Get(static_cast<uint32_t>(aResourceType));

  if (!resource ||
      resource->mResourceCount == 0) {
    
    
    mozilla::unused << aParent->SendResponse(aId, false );
    return;
  }

  
  if (resource->mAcquiredRequests.size() < resource->mResourceCount) {
    
    resource->mAcquiredRequests.push_back(
      MediaSystemResourceRequest(aParent, aId));
    
    mozilla::unused << aParent->SendResponse(aId, true );
    return;
  } else if (!aWillWait) {
    
    
    mozilla::unused << aParent->SendResponse(aId, false );
    return;
  }
  
  resource->mWaitingRequests.push_back(
    MediaSystemResourceRequest(aParent, aId));
}

void
MediaSystemResourceService::ReleaseResource(media::MediaSystemResourceManagerParent* aParent,
                                            uint32_t aId,
                                            MediaSystemResourceType aResourceType)
{
  MOZ_ASSERT(CompositorParent::IsInCompositorThread());
  MOZ_ASSERT(aParent);

  if (mDestroyed) {
    return;
  }

  MediaSystemResource* resource = mResources.Get(static_cast<uint32_t>(aResourceType));

  if (!resource ||
      resource->mResourceCount == 0) {
    
    return;
  }
  RemoveRequest(aParent, aId, aResourceType);
  UpdateRequests(aResourceType);
}

struct ReleaseResourceData
{
  MediaSystemResourceService* mSelf;
  media::MediaSystemResourceManagerParent* mParent;
};

PLDHashOperator
MediaSystemResourceService::ReleaseResourceForKey(const uint32_t& aKey,
                                                  nsAutoPtr<MediaSystemResource>& aData,
                                                  void* aClosure)
{
  ReleaseResourceData* closure = static_cast<ReleaseResourceData*>(aClosure);

  closure->mSelf->RemoveRequests(closure->mParent, static_cast<MediaSystemResourceType>(aKey));
  closure->mSelf->UpdateRequests(static_cast<MediaSystemResourceType>(aKey));

  return PLDHashOperator::PL_DHASH_NEXT;
}

void
MediaSystemResourceService::ReleaseResource(media::MediaSystemResourceManagerParent* aParent)
{
  MOZ_ASSERT(aParent);

  if (mDestroyed) {
    return;
  }

  ReleaseResourceData data = { this, aParent };
  mResources.Enumerate(ReleaseResourceForKey, &data);
}

void
MediaSystemResourceService::RemoveRequest(media::MediaSystemResourceManagerParent* aParent,
                                          uint32_t aId,
                                          MediaSystemResourceType aResourceType)
{
  MOZ_ASSERT(aParent);

  MediaSystemResource* resource = mResources.Get(static_cast<uint32_t>(aResourceType));
  if (!resource) {
    return;
  }

  std::deque<MediaSystemResourceRequest>::iterator it;
  std::deque<MediaSystemResourceRequest>& acquiredRequests =
    resource->mAcquiredRequests;
  for (it = acquiredRequests.begin(); it != acquiredRequests.end(); it++) {
    if (((*it).mParent == aParent) && ((*it).mId == aId)) {
      acquiredRequests.erase(it);
      return;
    }
  }

  std::deque<MediaSystemResourceRequest>& waitingRequests =
    resource->mWaitingRequests;
  for (it = waitingRequests.begin(); it != waitingRequests.end(); it++) {
    if (((*it).mParent == aParent) && ((*it).mId == aId)) {
      waitingRequests.erase(it);
      return;
    }
  }
}

void
MediaSystemResourceService::RemoveRequests(media::MediaSystemResourceManagerParent* aParent,
                                           MediaSystemResourceType aResourceType)
{
  MOZ_ASSERT(aParent);

  MediaSystemResource* resource = mResources.Get(static_cast<uint32_t>(aResourceType));

  if (!resource ||
      resource->mResourceCount == 0) {
    
    return;
  }

  std::deque<MediaSystemResourceRequest>::iterator it;
  std::deque<MediaSystemResourceRequest>& acquiredRequests =
    resource->mAcquiredRequests;
  for (it = acquiredRequests.begin(); it != acquiredRequests.end();) {
    if ((*it).mParent == aParent) {
      it = acquiredRequests.erase(it);
    } else {
      it++;
    }
  }

  std::deque<MediaSystemResourceRequest>& waitingRequests =
    resource->mWaitingRequests;
  for (it = waitingRequests.begin(); it != waitingRequests.end();) {
    if ((*it).mParent == aParent) {
      it = waitingRequests.erase(it);
      return;
    } else {
      it++;
    }
  }
}

void
MediaSystemResourceService::UpdateRequests(MediaSystemResourceType aResourceType)
{
  MediaSystemResource* resource = mResources.Get(static_cast<uint32_t>(aResourceType));

  if (!resource ||
      resource->mResourceCount == 0) {
    
    return;
  }

  std::deque<MediaSystemResourceRequest>& acquiredRequests =
    resource->mAcquiredRequests;
  std::deque<MediaSystemResourceRequest>& waitingRequests =
    resource->mWaitingRequests;

  while ((acquiredRequests.size() < resource->mResourceCount) &&
         (waitingRequests.size() > 0)) {
    MediaSystemResourceRequest& request = waitingRequests.front();
    MOZ_ASSERT(request.mParent);
    
    mozilla::unused << request.mParent->SendResponse(request.mId, true );
    
    acquiredRequests.push_back(waitingRequests.front());
    waitingRequests.pop_front();
  }
}

} 
