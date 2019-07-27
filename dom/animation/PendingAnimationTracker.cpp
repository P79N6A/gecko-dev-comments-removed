





#include "PendingAnimationTracker.h"

#include "mozilla/dom/AnimationTimeline.h"
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

void
PendingAnimationTracker::TriggerPendingAnimationsOnNextTick(const TimeStamp&
                                                        aReadyTime)
{
  auto triggerAnimationsAtReadyTime = [aReadyTime](AnimationSet& aAnimationSet)
  {
    for (auto iter = aAnimationSet.Iter(); !iter.Done(); iter.Next()) {
      dom::Animation* animation = iter.Get()->GetKey();
      dom::AnimationTimeline* timeline = animation->GetTimeline();

      
      
      
      if (!timeline) {
        iter.Remove();
      }

      
      
      
      
      
      
      if (!timeline->TracksWallclockTime()) {
        continue;
      }

      Nullable<TimeDuration> readyTime = timeline->ToTimelineTime(aReadyTime);
      animation->TriggerOnNextTick(readyTime);

      iter.Remove();
    }
  };

  triggerAnimationsAtReadyTime(mPlayPendingSet);
  triggerAnimationsAtReadyTime(mPausePendingSet);
}

void
PendingAnimationTracker::TriggerPendingAnimationsNow()
{
  auto triggerAndClearAnimations = [](AnimationSet& aAnimationSet) {
    for (auto iter = aAnimationSet.Iter(); !iter.Done(); iter.Next()) {
      iter.Get()->GetKey()->TriggerNow();
    }
    aAnimationSet.Clear();
  };

  triggerAndClearAnimations(mPlayPendingSet);
  triggerAndClearAnimations(mPausePendingSet);
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
