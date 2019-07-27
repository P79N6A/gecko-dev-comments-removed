




#include "AnimationPlayer.h"
#include "mozilla/dom/AnimationPlayerBinding.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(AnimationPlayer, mTimeline, mSource)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(AnimationPlayer, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(AnimationPlayer, Release)

JSObject*
AnimationPlayer::WrapObject(JSContext* aCx)
{
  return dom::AnimationPlayerBinding::Wrap(aCx, this);
}

double
AnimationPlayer::StartTime() const
{
  Nullable<double> startTime = mTimeline->ToTimelineTime(mStartTime);
  return startTime.IsNull() ? 0.0 : startTime.Value();
}

double
AnimationPlayer::CurrentTime() const
{
  Nullable<TimeDuration> currentTime = GetCurrentTimeDuration();

  
  
  
  
  
  
  
  
  
  
  
  if (currentTime.IsNull()) {
    return 0.0;
  }

  return currentTime.Value().ToMilliseconds();
}

void
AnimationPlayer::SetSource(Animation* aSource)
{
  if (mSource) {
    mSource->SetParentTime(Nullable<TimeDuration>());
  }
  mSource = aSource;
  if (mSource) {
    mSource->SetParentTime(GetCurrentTimeDuration());
  }
}

void
AnimationPlayer::Tick()
{
  if (mSource) {
    mSource->SetParentTime(GetCurrentTimeDuration());
  }
}

bool
AnimationPlayer::IsRunning() const
{
  if (IsPaused() || !GetSource() || GetSource()->IsFinishedTransition()) {
    return false;
  }

  ComputedTiming computedTiming = GetSource()->GetComputedTiming();
  return computedTiming.mPhase == ComputedTiming::AnimationPhase_Active;
}

bool
AnimationPlayer::IsCurrent() const
{
  return GetSource() && GetSource()->IsCurrent();
}

} 
} 
