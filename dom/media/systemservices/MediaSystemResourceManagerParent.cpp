




#include "mozilla/unused.h"

#include "MediaSystemResourceManagerParent.h"

namespace mozilla {
namespace media {

using namespace ipc;

MediaSystemResourceManagerParent::MediaSystemResourceManagerParent()
  : mDestroyed(false)
{
  mMediaSystemResourceService = MediaSystemResourceService::Get();
}

MediaSystemResourceManagerParent::~MediaSystemResourceManagerParent()
{
  MOZ_ASSERT(mDestroyed);
}

bool
MediaSystemResourceManagerParent::RecvAcquire(const uint32_t& aId,
                                              const MediaSystemResourceType& aResourceType,
                                              const bool& aWillWait)
{
  MediaSystemResourceRequest* request = mResourceRequests.Get(aId);
  MOZ_ASSERT(!request);
  if (request) {
    
    mozilla::unused << SendResponse(aId, false );
    return true;
  }

  request = new MediaSystemResourceRequest(aId, aResourceType);
  mResourceRequests.Put(aId, request);
  mMediaSystemResourceService->Acquire(this, aId, aResourceType, aWillWait);
  return true;
}

bool
MediaSystemResourceManagerParent::RecvRelease(const uint32_t& aId)
{
  MediaSystemResourceRequest* request = mResourceRequests.Get(aId);
  if (!request) {
    return true;
  }

  mMediaSystemResourceService->ReleaseResource(this, aId, request->mResourceType);
  mResourceRequests.Remove(aId);
  return true;
}

bool
MediaSystemResourceManagerParent::RecvRemoveResourceManager()
{
  return PMediaSystemResourceManagerParent::Send__delete__(this);
}

void
MediaSystemResourceManagerParent::ActorDestroy(ActorDestroyReason aReason)
{
  MOZ_ASSERT(!mDestroyed);

  
  
  mMediaSystemResourceService->ReleaseResource(this);

  mDestroyed = true;
}

} 
} 
