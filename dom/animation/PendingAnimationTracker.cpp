




#include "PendingAnimationTracker.h"

#include "mozilla/dom/DocumentTimeline.h"
#include "nsIFrame.h"
#include "nsIPresShell.h"

using namespace mozilla;

namespace mozilla {

NS_IMPL_CYCLE_COLLECTION(PendingAnimationTracker,
                         mPlayPendingSet,
                         mPausePendingSet,
                         mDocument)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(PendingAnimationTracker, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(PendingAnimationTracker, Release)

void
PendingAnimationTracker::AddPending(dom::Animation& aPlayer,
                                 AnimationPlayerSet& aSet)
{
  aSet.PutEntry(&aPlayer);

  
  
  
  EnsurePaintIsScheduled();
}

void
PendingAnimationTracker::RemovePending(dom::Animation& aPlayer,
                                    AnimationPlayerSet& aSet)
{
  aSet.RemoveEntry(&aPlayer);
}

bool
PendingAnimationTracker::IsWaiting(const dom::Animation& aPlayer,
                                const AnimationPlayerSet& aSet) const
{
  return aSet.Contains(const_cast<dom::Animation*>(&aPlayer));
}

PLDHashOperator
TriggerPlayerAtTime(nsRefPtrHashKey<dom::Animation>* aKey,
                    void* aReadyTime)
{
  dom::Animation* player = aKey->GetKey();
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
PendingAnimationTracker::TriggerPendingAnimationsOnNextTick(const TimeStamp&
                                                        aReadyTime)
{
  mPlayPendingSet.EnumerateEntries(TriggerPlayerAtTime,
                                   const_cast<TimeStamp*>(&aReadyTime));
  mPausePendingSet.EnumerateEntries(TriggerPlayerAtTime,
                                    const_cast<TimeStamp*>(&aReadyTime));
}

PLDHashOperator
TriggerPlayerNow(nsRefPtrHashKey<dom::Animation>* aKey, void*)
{
  aKey->GetKey()->TriggerNow();
  return PL_DHASH_NEXT;
}

void
PendingAnimationTracker::TriggerPendingAnimationsNow()
{
  mPlayPendingSet.EnumerateEntries(TriggerPlayerNow, nullptr);
  mPlayPendingSet.Clear();
  mPausePendingSet.EnumerateEntries(TriggerPlayerNow, nullptr);
  mPausePendingSet.Clear();
}

void
PendingAnimationTracker::EnsurePaintIsScheduled()
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
