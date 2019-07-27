




#include "PendingPlayerTracker.h"

#include "mozilla/dom/DocumentTimeline.h"
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
TriggerPlayerAtTime(nsRefPtrHashKey<dom::AnimationPlayer>* aKey,
                    void* aReadyTime)
{
  dom::AnimationPlayer* player = aKey->GetKey();
  dom::DocumentTimeline* timeline = player->Timeline();

  
  
  
  
  
  if (timeline->IsUnderTestControl()) {
    return PL_DHASH_NEXT;
  }

  Nullable<TimeDuration> readyTime =
    timeline->ToTimelineTime(*static_cast<const TimeStamp*>(aReadyTime));
  player->TriggerOnNextTick(readyTime);

  return PL_DHASH_REMOVE;
}

void
PendingPlayerTracker::TriggerPendingPlayersOnNextTick(const TimeStamp&
                                                        aReadyTime)
{
  mPlayPendingSet.EnumerateEntries(TriggerPlayerAtTime,
                                   const_cast<TimeStamp*>(&aReadyTime));
  mPausePendingSet.EnumerateEntries(TriggerPlayerAtTime,
                                    const_cast<TimeStamp*>(&aReadyTime));
}

PLDHashOperator
TriggerPlayerNow(nsRefPtrHashKey<dom::AnimationPlayer>* aKey, void*)
{
  aKey->GetKey()->TriggerNow();
  return PL_DHASH_NEXT;
}

void
PendingPlayerTracker::TriggerPendingPlayersNow()
{
  mPlayPendingSet.EnumerateEntries(TriggerPlayerNow, nullptr);
  mPlayPendingSet.Clear();
  mPausePendingSet.EnumerateEntries(TriggerPlayerNow, nullptr);
  mPausePendingSet.Clear();
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
