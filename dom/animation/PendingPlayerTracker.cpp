




#include "PendingPlayerTracker.h"

using namespace mozilla;

namespace mozilla {

NS_IMPL_CYCLE_COLLECTION(PendingPlayerTracker, mPlayPendingSet)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(PendingPlayerTracker, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(PendingPlayerTracker, Release)

void
PendingPlayerTracker::AddPlayPending(dom::AnimationPlayer& aPlayer)
{
  mPlayPendingSet.PutEntry(&aPlayer);
}

void
PendingPlayerTracker::RemovePlayPending(dom::AnimationPlayer& aPlayer)
{
  mPlayPendingSet.RemoveEntry(&aPlayer);
}

bool
PendingPlayerTracker::IsWaitingToPlay(dom::AnimationPlayer const& aPlayer) const
{
  return mPlayPendingSet.Contains(const_cast<dom::AnimationPlayer*>(&aPlayer));
}

} 
