




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

Nullable<TimeDuration>
AnimationPlayer::GetCurrentTime() const
{
  Nullable<TimeDuration> result;
  if (!mHoldTime.IsNull()) {
    result = mHoldTime;
  } else {
    Nullable<TimeDuration> timelineTime = mTimeline->GetCurrentTime();
    if (!timelineTime.IsNull() && !mStartTime.IsNull()) {
      result.SetValue(timelineTime.Value() - mStartTime.Value());
    }
  }
  return result;
}

AnimationPlayState
AnimationPlayer::PlayState() const
{
  Nullable<TimeDuration> currentTime = GetCurrentTime();
  if (currentTime.IsNull()) {
    return AnimationPlayState::Idle;
  }

  if (mIsPaused) {
    return AnimationPlayState::Paused;
  }

  if (currentTime.Value() >= SourceContentEnd()) {
    return AnimationPlayState::Finished;
  }

  return AnimationPlayState::Running;
}

void
AnimationPlayer::Play(UpdateFlags aFlags)
{
  
  
  
  
  if (!mIsPaused) {
    return;
  }
  mIsPaused = false;

  Nullable<TimeDuration> timelineTime = mTimeline->GetCurrentTime();
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

  
  mHoldTime = GetCurrentTime();
  mStartTime.SetNull();

  if (aFlags == eUpdateStyle) {
    MaybePostRestyle();
  }
}

Nullable<double>
AnimationPlayer::GetCurrentTimeAsDouble() const
{
  return AnimationUtils::TimeDurationToDouble(GetCurrentTime());
}

AnimationPlayState
AnimationPlayer::PlayStateFromJS() const
{
  
  
  
  FlushStyle();

  return PlayState();
}

void
AnimationPlayer::PlayFromJS()
{
  
  
  
  
  
  
  
  
  
  FlushStyle();

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
    mSource->SetParentTime(GetCurrentTime());
  }
}

void
AnimationPlayer::Tick()
{
  if (mSource) {
    mSource->SetParentTime(GetCurrentTime());
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
AnimationPlayer::CanThrottle() const
{
  if (!mSource ||
      mSource->IsFinishedTransition() ||
      mSource->Properties().IsEmpty()) {
    return true;
  }

  if (!mIsRunningOnCompositor) {
    return false;
  }

  if (PlayState() != AnimationPlayState::Finished) {
    
    return true;
  }

  
  
  
  
  
  
  
  
  
  return mSource->LastNotification() == Animation::LAST_NOTIFICATION_END;
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

StickyTimeDuration
AnimationPlayer::SourceContentEnd() const
{
  if (!mSource) {
    return StickyTimeDuration(0);
  }

  return mSource->Timing().mDelay
         + mSource->GetComputedTiming().mActiveDuration;
}

} 

void
CSSAnimationPlayer::Play(UpdateFlags aUpdateFlags)
{
  mPauseShouldStick = false;
  AnimationPlayer::Play(aUpdateFlags);
}

void
CSSAnimationPlayer::Pause(UpdateFlags aUpdateFlags)
{
  mPauseShouldStick = true;
  AnimationPlayer::Pause(aUpdateFlags);
}

void
CSSAnimationPlayer::PlayFromStyle()
{
  mIsStylePaused = false;
  if (!mPauseShouldStick) {
    AnimationPlayer::Play(eNoUpdate);
  }
}

void
CSSAnimationPlayer::PauseFromStyle()
{
  
  if (mIsStylePaused) {
    return;
  }

  mIsStylePaused = true;
  AnimationPlayer::Pause(eNoUpdate);
}

} 
