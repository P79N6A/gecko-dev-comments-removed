




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
PendingAnimationTracker::AddPending(dom::Animation& aAnimation,
                                    AnimationSet& aSet)
{
  aSet.PutEntry(&aAnimation);

  
  
  
  EnsurePaintIsScheduled();
}

void
PendingAnimationTracker::RemovePending(dom::Animation& aAnimation,
                                       AnimationSet& aSet)
{
  aSet.RemoveEntry(&aAnimation);
}

bool
PendingAnimationTracker::IsWaiting(const dom::Animation& aAnimation,
                                   const AnimationSet& aSet) const
{
  return aSet.Contains(const_cast<dom::Animation*>(&aAnimation));
}

PLDHashOperator
TriggerAnimationAtTime(nsRefPtrHashKey<dom::Animation>* aKey,
                    void* aReadyTime)
{
  dom::Animation* animation = aKey->GetKey();
  dom::DocumentTimeline* timeline = animation->Timeline();

  
  
  
  
  
  if (timeline->IsUnderTestControl()) {
    return PL_DHASH_NEXT;
  }

  Nullable<TimeDuration> readyTime =
    timeline->ToTimelineTime(*static_cast<const TimeStamp*>(aReadyTime));
  animation->TriggerOnNextTick(readyTime);

  return PL_DHASH_REMOVE;
}

void
PendingAnimationTracker::TriggerPendingAnimationsOnNextTick(const TimeStamp&
                                                        aReadyTime)
{
  mPlayPendingSet.EnumerateEntries(TriggerAnimationAtTime,
                                   const_cast<TimeStamp*>(&aReadyTime));
  mPausePendingSet.EnumerateEntries(TriggerAnimationAtTime,
                                    const_cast<TimeStamp*>(&aReadyTime));
}

PLDHashOperator
TriggerAnimationNow(nsRefPtrHashKey<dom::Animation>* aKey, void*)
{
  aKey->GetKey()->TriggerNow();
  return PL_DHASH_NEXT;
}

void
PendingAnimationTracker::TriggerPendingAnimationsNow()
{
  mPlayPendingSet.EnumerateEntries(TriggerAnimationNow, nullptr);
  mPlayPendingSet.Clear();
  mPausePendingSet.EnumerateEntries(TriggerAnimationNow, nullptr);
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
