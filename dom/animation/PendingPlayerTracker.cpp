




#include "PendingPlayerTracker.h"

#include "mozilla/dom/AnimationTimeline.h"
#include "nsIFrame.h"
#include "nsIPresShell.h"

using namespace mozilla;

namespace mozilla {

NS_IMPL_CYCLE_COLLECTION(PendingPlayerTracker,
                         mPlayPendingSet,
                         mPausePendingSet,
                         mDocument)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(PendingPlayerTracker, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(PendingPlayerTracker, Release)

void
PendingPlayerTracker::AddPending(dom::AnimationPlayer& aPlayer,
                                 AnimationPlayerSet& aSet)
{
  aSet.PutEntry(&aPlayer);

  
  
  
  EnsurePaintIsScheduled();
}

void
PendingPlayerTracker::RemovePending(dom::AnimationPlayer& aPlayer,
                                    AnimationPlayerSet& aSet)
{
  aSet.RemoveEntry(&aPlayer);
}

bool
PendingPlayerTracker::IsWaiting(const dom::AnimationPlayer& aPlayer,
                                const AnimationPlayerSet& aSet) const
{
  return aSet.Contains(const_cast<dom::AnimationPlayer*>(&aPlayer));
}

PLDHashOperator
StartPlayerAtTime(nsRefPtrHashKey<dom::AnimationPlayer>* aKey,
                  void* aReadyTime)
{
  dom::AnimationPlayer* player = aKey->GetKey();
  dom::AnimationTimeline* timeline = player->Timeline();

  
  
  
  
  
  if (timeline->IsUnderTestControl()) {
    return PL_DHASH_NEXT;
  }

  Nullable<TimeDuration> readyTime =
    timeline->ToTimelineTime(*static_cast<const TimeStamp*>(aReadyTime));
  player->StartOnNextTick(readyTime);

  return PL_DHASH_REMOVE;
}

void
PendingPlayerTracker::StartPendingPlayersOnNextTick(const TimeStamp& aReadyTime)
{
  mPlayPendingSet.EnumerateEntries(StartPlayerAtTime,
                                   const_cast<TimeStamp*>(&aReadyTime));
}

PLDHashOperator
StartPlayerNow(nsRefPtrHashKey<dom::AnimationPlayer>* aKey, void*)
{
  aKey->GetKey()->StartNow();
  return PL_DHASH_NEXT;
}

void
PendingPlayerTracker::StartPendingPlayersNow()
{
  mPlayPendingSet.EnumerateEntries(StartPlayerNow, nullptr);
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
