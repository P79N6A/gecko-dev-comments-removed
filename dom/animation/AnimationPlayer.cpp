




#include "AnimationPlayer.h"
#include "AnimationUtils.h"
#include "mozilla/dom/AnimationPlayerBinding.h"
#include "nsLayoutUtils.h" 

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

Nullable<double>
AnimationPlayer::GetStartTime() const
{
  return AnimationUtils::TimeDurationToDouble(mStartTime);
}

Nullable<double>
AnimationPlayer::GetCurrentTime() const
{
  return AnimationUtils::TimeDurationToDouble(GetCurrentTimeDuration());
}

void
AnimationPlayer::Play(UpdateFlags aFlags)
{
  
  
  if (!mIsPaused) {
    return;
  }
  mIsPaused = false;

  Nullable<TimeDuration> timelineTime = mTimeline->GetCurrentTimeDuration();
  if (timelineTime.IsNull()) {
    
    
    return;
  }

  
  MOZ_ASSERT(!mHoldTime.IsNull(), "Hold time should not be null when paused");
  mStartTime.SetValue(timelineTime.Value() - mHoldTime.Value());
  mHoldTime.SetNull();

  if (aFlags == eUpdateStyle) {
    MaybePostRestyle();
  }
}

void
AnimationPlayer::Pause(UpdateFlags aFlags)
{
  if (mIsPaused) {
    return;
  }
  mIsPaused = true;
  mIsRunningOnCompositor = false;

  
  mHoldTime = GetCurrentTimeDuration();
  mStartTime.SetNull();

  if (aFlags == eUpdateStyle) {
    MaybePostRestyle();
  }
}

void
AnimationPlayer::PlayFromJS()
{
  

  Play(eUpdateStyle);
}

void
AnimationPlayer::PauseFromJS()
{
  Pause(eUpdateStyle);
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

Nullable<TimeDuration>
AnimationPlayer::GetCurrentTimeDuration() const
{
  Nullable<TimeDuration> result;
  if (!mHoldTime.IsNull()) {
    result = mHoldTime;
  } else {
    Nullable<TimeDuration> timelineTime = mTimeline->GetCurrentTimeDuration();
    if (!timelineTime.IsNull() && !mStartTime.IsNull()) {
      result.SetValue(timelineTime.Value() - mStartTime.Value());
    }
  }
  return result;
}

void
AnimationPlayer::FlushStyle() const
{
  if (mSource && mSource->GetTarget()) {
    nsIDocument* doc = mSource->GetTarget()->GetComposedDoc();
    if (doc) {
      doc->FlushPendingNotifications(Flush_Style);
    }
  }
}

void
AnimationPlayer::MaybePostRestyle() const
{
  if (!mSource || !mSource->GetTarget())
    return;

  
  
  nsLayoutUtils::PostRestyleEvent(mSource->GetTarget(),
                                  eRestyle_Self,
                                  nsChangeHint_AllReflowHints);
}

} 
} 
