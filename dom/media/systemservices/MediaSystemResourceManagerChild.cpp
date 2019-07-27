




#include "MediaSystemResourceManager.h"

#include "MediaSystemResourceManagerChild.h"

namespace mozilla {
namespace media {

MediaSystemResourceManagerChild::MediaSystemResourceManagerChild()
  : mDestroyed(false)
{
}

MediaSystemResourceManagerChild::~MediaSystemResourceManagerChild()
{
}

bool
MediaSystemResourceManagerChild::RecvResponse(const uint32_t& aId,
                                              const bool& aSuccess)
{
  if (mManager) {
    mManager->RecvResponse(aId, aSuccess);
  }
  return true;
}

void
MediaSystemResourceManagerChild::ActorDestroy(ActorDestroyReason aActorDestroyReason)
{
  MOZ_ASSERT(!mDestroyed);
  if (mManager) {
    mManager->OnIpcClosed();
  }
  mDestroyed = true;
}

void
MediaSystemResourceManagerChild::Destroy()
{
  if (mDestroyed) {
    return;
  }
  SendRemoveResourceManager();
  
}

} 
} 
