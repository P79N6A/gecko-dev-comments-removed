




#include "PendingPlayerTracker.h"

#include "mozilla/dom/AnimationTimeline.h"
#include "nsIFrame.h"
#include "nsIPresShell.h"

using namespace mozilla;

namespace mozilla {

NS_IMPL_CYCLE_COLLECTION(PendingPlayerTracker, mPlayPendingSet, mDocument)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(PendingPlayerTracker, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(PendingPlayerTracker, Release)

void
PendingPlayerTracker::AddPlayPending(dom::AnimationPlayer& aPlayer)
{
  mPlayPendingSet.PutEntry(&aPlayer);

  
  
  
  EnsurePaintIsScheduled();
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

PLDHashOperator
StartPlayerAtTime(nsRefPtrHashKey<dom::AnimationPlayer>* aKey,
                  void* aReadyTime)
{
  dom::AnimationPlayer* player = aKey->GetKey();

  
  
  
  
  
  
  
  
  
  
  dom::AnimationTimeline* timeline = player->Timeline();
  timeline->FastForward(*static_cast<const TimeStamp*>(aReadyTime));

  player->StartNow();

  return PL_DHASH_NEXT;
}

void
PendingPlayerTracker::StartPendingPlayers(const TimeStamp& aReadyTime)
{
  mPlayPendingSet.EnumerateEntries(StartPlayerAtTime,
                                   const_cast<TimeStamp*>(&aReadyTime));
  mPlayPendingSet.Clear();
}

void
PendingPlayerTracker::EnsurePaintIsScheduled()
{
  if (!mDocument) {
    return;
  }

  nsIPresShell* presShell = mDocument->GetShell();
  if (!presShell) {
    return;
  }

  nsIFrame* rootFrame = presShell->GetRootFrame();
  if (!rootFrame) {
    return;
  }

  rootFrame->SchedulePaint();
}

} 
