





#include "Animation.h"
#include "AnimationUtils.h"
#include "mozilla/dom/AnimationBinding.h"
#include "mozilla/AutoRestore.h"
#include "AnimationCommon.h" 
                             
#include "nsIDocument.h" 
#include "nsIPresShell.h" 
#include "nsLayoutUtils.h" 
#include "PendingAnimationTracker.h" 

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(Animation, mTimeline,
                                      mEffect, mReady, mFinished)
NS_IMPL_CYCLE_COLLECTING_ADDREF(Animation)
NS_IMPL_CYCLE_COLLECTING_RELEASE(Animation)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(Animation)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

JSObject*
Animation::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return dom::AnimationBinding::Wrap(aCx, this, aGivenProto);
}







void
Animation::SetEffect(KeyframeEffectReadOnly* aEffect)
{
  if (mEffect) {
    mEffect->SetParentTime(Nullable<TimeDuration>());
  }
  mEffect = aEffect;
  if (mEffect) {
    mEffect->SetParentTime(GetCurrentTime());
  }
  UpdateRelevance();
}

void
Animation::SetStartTime(const Nullable<TimeDuration>& aNewStartTime)
{
#if 1
  
  
  MOZ_ASSERT(mTimeline && !mTimeline->GetCurrentTime().IsNull(),
             "We don't support inactive/missing timelines yet");
#else
  Nullable<TimeDuration> timelineTime = mTimeline->GetCurrentTime();
  if (mTimeline) {
    
    
    
    timelineTime = mTimeline->GetCurrentTime();
  }
  if (timelineTime.IsNull() && !aNewStartTime.IsNull()) {
    mHoldTime.SetNull();
  }
#endif
  Nullable<TimeDuration> previousCurrentTime = GetCurrentTime();
  mStartTime = aNewStartTime;
  if (!aNewStartTime.IsNull()) {
    if (mPlaybackRate != 0.0) {
      mHoldTime.SetNull();
    }
  } else {
    mHoldTime = previousCurrentTime;
  }

  CancelPendingTasks();
  if (mReady) {
    
    
    mReady->MaybeResolve(this);
  }

  UpdateTiming(SeekFlag::NoSeek);
  PostUpdate();
}


Nullable<TimeDuration>
Animation::GetCurrentTime() const
{
  Nullable<TimeDuration> result;
  if (!mHoldTime.IsNull()) {
    result = mHoldTime;
    return result;
  }

  if (!mStartTime.IsNull()) {
    Nullable<TimeDuration> timelineTime = mTimeline->GetCurrentTime();
    if (!timelineTime.IsNull()) {
      result.SetValue((timelineTime.Value() - mStartTime.Value())
                        .MultDouble(mPlaybackRate));
    }
  }
  return result;
}


void
Animation::SetCurrentTime(const TimeDuration& aSeekTime)
{
  SilentlySetCurrentTime(aSeekTime);

  if (mPendingState == PendingState::PausePending) {
    
    mHoldTime.SetValue(aSeekTime);
    mStartTime.SetNull();

    if (mReady) {
      mReady->MaybeResolve(this);
    }
    CancelPendingTasks();
  }

  UpdateTiming(SeekFlag::DidSeek);
  PostUpdate();
}

void
Animation::SetPlaybackRate(double aPlaybackRate)
{
  Nullable<TimeDuration> previousTime = GetCurrentTime();
  mPlaybackRate = aPlaybackRate;
  if (!previousTime.IsNull()) {
    ErrorResult rv;
    SetCurrentTime(previousTime.Value());
    MOZ_ASSERT(!rv.Failed(), "Should not assert for non-null time");
  }
}

AnimationPlayState
Animation::PlayState() const
{
  if (mPendingState != PendingState::NotPending) {
    return AnimationPlayState::Pending;
  }

  Nullable<TimeDuration> currentTime = GetCurrentTime();
  if (currentTime.IsNull()) {
    return AnimationPlayState::Idle;
  }

  if (mStartTime.IsNull()) {
    return AnimationPlayState::Paused;
  }

  if ((mPlaybackRate > 0.0 && currentTime.Value() >= EffectEnd()) ||
      (mPlaybackRate < 0.0 && currentTime.Value().ToMilliseconds() <= 0.0)) {
    return AnimationPlayState::Finished;
  }

  return AnimationPlayState::Running;
}

static inline already_AddRefed<Promise>
CreatePromise(DocumentTimeline* aTimeline, ErrorResult& aRv)
{
  nsIGlobalObject* global = aTimeline->GetParentObject();
  if (global) {
    return Promise::Create(global, aRv);
  }
  return nullptr;
}

Promise*
Animation::GetReady(ErrorResult& aRv)
{
  if (!mReady) {
    mReady = CreatePromise(mTimeline, aRv); 
  }
  if (!mReady) {
    aRv.Throw(NS_ERROR_FAILURE);
  } else if (PlayState() != AnimationPlayState::Pending) {
    mReady->MaybeResolve(this);
  }
  return mReady;
}

Promise*
Animation::GetFinished(ErrorResult& aRv)
{
  if (!mFinished) {
    mFinished = CreatePromise(mTimeline, aRv); 
  }
  if (!mFinished) {
    aRv.Throw(NS_ERROR_FAILURE);
  } else if (PlayState() == AnimationPlayState::Finished) {
    mFinished->MaybeResolve(this);
  }
  return mFinished;
}

void
Animation::Cancel()
{
  DoCancel();
  PostUpdate();
}


void
Animation::Finish(ErrorResult& aRv)
{
  if (mPlaybackRate == 0 ||
      (mPlaybackRate > 0 && EffectEnd() == TimeDuration::Forever())) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }

  TimeDuration limit =
    mPlaybackRate > 0 ? TimeDuration(EffectEnd()) : TimeDuration(0);

  SetCurrentTime(limit);

  
  
  
  
  
  
  
  if (mStartTime.IsNull() &&
      mTimeline &&
      !mTimeline->GetCurrentTime().IsNull()) {
    mStartTime.SetValue(mTimeline->GetCurrentTime().Value() -
                        limit.MultDouble(1.0 / mPlaybackRate));
  }

  
  
  
  
  if (mPendingState == PendingState::PlayPending &&
      !mStartTime.IsNull()) {
    CancelPendingTasks();
    if (mReady) {
      mReady->MaybeResolve(this);
    }
  }
  UpdateTiming(SeekFlag::DidSeek);
  PostUpdate();
}

void
Animation::Play(ErrorResult& aRv, LimitBehavior aLimitBehavior)
{
  DoPlay(aRv, aLimitBehavior);
  PostUpdate();
}

void
Animation::Pause(ErrorResult& aRv)
{
  DoPause(aRv);
  PostUpdate();
}







Nullable<double>
Animation::GetStartTimeAsDouble() const
{
  return AnimationUtils::TimeDurationToDouble(mStartTime);
}

void
Animation::SetStartTimeAsDouble(const Nullable<double>& aStartTime)
{
  return SetStartTime(AnimationUtils::DoubleToTimeDuration(aStartTime));
}

Nullable<double>
Animation::GetCurrentTimeAsDouble() const
{
  return AnimationUtils::TimeDurationToDouble(GetCurrentTime());
}

void
Animation::SetCurrentTimeAsDouble(const Nullable<double>& aCurrentTime,
                                        ErrorResult& aRv)
{
  if (aCurrentTime.IsNull()) {
    if (!GetCurrentTime().IsNull()) {
      aRv.Throw(NS_ERROR_DOM_TYPE_ERR);
    }
    return;
  }

  return SetCurrentTime(TimeDuration::FromMilliseconds(aCurrentTime.Value()));
}



void
Animation::Tick()
{
  
  
  
  
  if (mPendingState != PendingState::NotPending &&
      !mPendingReadyTime.IsNull() &&
      mPendingReadyTime.Value() <= mTimeline->GetCurrentTime().Value()) {
    FinishPendingAt(mPendingReadyTime.Value());
    mPendingReadyTime.SetNull();
  }

  if (IsPossiblyOrphanedPendingAnimation()) {
    MOZ_ASSERT(mTimeline && !mTimeline->GetCurrentTime().IsNull(),
               "Orphaned pending animtaions should have an active timeline");
    FinishPendingAt(mTimeline->GetCurrentTime().Value());
  }

  UpdateTiming(SeekFlag::NoSeek);
}

void
Animation::TriggerOnNextTick(const Nullable<TimeDuration>& aReadyTime)
{
  
  
  
  
  if (PlayState() != AnimationPlayState::Pending) {
    return;
  }

  
  
  mPendingReadyTime = aReadyTime;
}

void
Animation::TriggerNow()
{
  MOZ_ASSERT(PlayState() == AnimationPlayState::Pending,
             "Expected to start a pending animation");
  MOZ_ASSERT(mTimeline && !mTimeline->GetCurrentTime().IsNull(),
             "Expected an active timeline");

  FinishPendingAt(mTimeline->GetCurrentTime().Value());
}

Nullable<TimeDuration>
Animation::GetCurrentOrPendingStartTime() const
{
  Nullable<TimeDuration> result;

  if (!mStartTime.IsNull()) {
    result = mStartTime;
    return result;
  }

  if (mPendingReadyTime.IsNull() || mHoldTime.IsNull()) {
    return result;
  }

  
  
  
  result.SetValue(mPendingReadyTime.Value() - mHoldTime.Value());
  return result;
}


void
Animation::SilentlySetCurrentTime(const TimeDuration& aSeekTime)
{
  if (!mHoldTime.IsNull() ||
      mStartTime.IsNull() ||
      !mTimeline ||
      mTimeline->GetCurrentTime().IsNull() ||
      mPlaybackRate == 0.0) {
    mHoldTime.SetValue(aSeekTime);
    if (!mTimeline || mTimeline->GetCurrentTime().IsNull()) {
      mStartTime.SetNull();
    }
  } else {
    mStartTime.SetValue(mTimeline->GetCurrentTime().Value() -
                          (aSeekTime.MultDouble(1 / mPlaybackRate)));
  }

  mPreviousCurrentTime.SetNull();
}

void
Animation::SilentlySetPlaybackRate(double aPlaybackRate)
{
  Nullable<TimeDuration> previousTime = GetCurrentTime();
  mPlaybackRate = aPlaybackRate;
  if (!previousTime.IsNull()) {
    ErrorResult rv;
    SilentlySetCurrentTime(previousTime.Value());
    MOZ_ASSERT(!rv.Failed(), "Should not assert for non-null time");
  }
}

void
Animation::DoCancel()
{
  if (mPendingState != PendingState::NotPending) {
    CancelPendingTasks();
    if (mReady) {
      mReady->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
    }
  }

  if (mFinished) {
    mFinished->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
  }
  
  mFinished = nullptr;

  mHoldTime.SetNull();
  mStartTime.SetNull();

  UpdateEffect();
}

void
Animation::UpdateRelevance()
{
  bool wasRelevant = mIsRelevant;
  mIsRelevant = HasCurrentEffect() || IsInEffect();

  
  if (wasRelevant && !mIsRelevant) {
    nsNodeUtils::AnimationRemoved(this);
  } else if (!wasRelevant && mIsRelevant) {
    nsNodeUtils::AnimationAdded(this);
  }
}

bool
Animation::CanThrottle() const
{
  if (!mEffect ||
      mEffect->IsFinishedTransition() ||
      mEffect->Properties().IsEmpty()) {
    return true;
  }

  if (!mIsRunningOnCompositor) {
    return false;
  }

  if (PlayState() != AnimationPlayState::Finished) {
    
    return true;
  }

  
  
  
  
  return mFinishedAtLastComposeStyle;
}

void
Animation::ComposeStyle(nsRefPtr<css::AnimValuesStyleRule>& aStyleRule,
                        nsCSSPropertySet& aSetProperties,
                        bool& aNeedsRefreshes)
{
  if (!mEffect || mEffect->IsFinishedTransition()) {
    return;
  }

  AnimationPlayState playState = PlayState();
  if (playState == AnimationPlayState::Running ||
      playState == AnimationPlayState::Pending) {
    aNeedsRefreshes = true;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  {
    AutoRestore<Nullable<TimeDuration>> restoreHoldTime(mHoldTime);
    bool updatedHoldTime = false;

    AnimationPlayState playState = PlayState();

    if (playState == AnimationPlayState::Pending &&
        mHoldTime.IsNull() &&
        !mStartTime.IsNull()) {
      Nullable<TimeDuration> timeToUse = mPendingReadyTime;
      if (timeToUse.IsNull() &&
          mTimeline &&
          !mTimeline->IsUnderTestControl()) {
        timeToUse = mTimeline->ToTimelineTime(TimeStamp::Now());
      }
      if (!timeToUse.IsNull()) {
        mHoldTime.SetValue((timeToUse.Value() - mStartTime.Value())
                            .MultDouble(mPlaybackRate));
        
        UpdateEffect();
        updatedHoldTime = true;
      }
    }

    mEffect->ComposeStyle(aStyleRule, aSetProperties);

    if (updatedHoldTime) {
      UpdateTiming(SeekFlag::NoSeek);
    }

    mFinishedAtLastComposeStyle = (playState == AnimationPlayState::Finished);
  }
}


void
Animation::DoPlay(ErrorResult& aRv, LimitBehavior aLimitBehavior)
{
  bool abortedPause = mPendingState == PendingState::PausePending;

  Nullable<TimeDuration> currentTime = GetCurrentTime();
  if (mPlaybackRate > 0.0 &&
      (currentTime.IsNull() ||
       (aLimitBehavior == LimitBehavior::AutoRewind &&
        (currentTime.Value().ToMilliseconds() < 0.0 ||
         currentTime.Value() >= EffectEnd())))) {
    mHoldTime.SetValue(TimeDuration(0));
  } else if (mPlaybackRate < 0.0 &&
             (currentTime.IsNull() ||
              (aLimitBehavior == LimitBehavior::AutoRewind &&
               (currentTime.Value().ToMilliseconds() <= 0.0 ||
                currentTime.Value() > EffectEnd())))) {
    if (EffectEnd() == TimeDuration::Forever()) {
      aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
      return;
    }
    mHoldTime.SetValue(TimeDuration(EffectEnd()));
  } else if (mPlaybackRate == 0.0 && currentTime.IsNull()) {
    mHoldTime.SetValue(TimeDuration(0));
  }

  bool reuseReadyPromise = false;
  if (mPendingState != PendingState::NotPending) {
    CancelPendingTasks();
    reuseReadyPromise = true;
  }

  
  
  
  
  if (mHoldTime.IsNull() && !abortedPause) {
    return;
  }

  
  
  
  
  
  
  
  
  
  
  
  if (!mHoldTime.IsNull()) {
    mStartTime.SetNull();
  }

  if (!reuseReadyPromise) {
    
    mReady = nullptr;
  }

  mPendingState = PendingState::PlayPending;

  nsIDocument* doc = GetRenderedDocument();
  if (!doc) {
    TriggerOnNextTick(Nullable<TimeDuration>());
    return;
  }

  PendingAnimationTracker* tracker = doc->GetOrCreatePendingAnimationTracker();
  tracker->AddPlayPending(*this);

  
  UpdateTiming(SeekFlag::NoSeek);
}


void
Animation::DoPause(ErrorResult& aRv)
{
  if (IsPausedOrPausing()) {
    return;
  }

  
  if (GetCurrentTime().IsNull()) {
    if (mPlaybackRate >= 0.0) {
      mHoldTime.SetValue(TimeDuration(0));
    } else {
      if (EffectEnd() == TimeDuration::Forever()) {
        aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
        return;
      }
      mHoldTime.SetValue(TimeDuration(EffectEnd()));
    }
  }

  bool reuseReadyPromise = false;
  if (mPendingState == PendingState::PlayPending) {
    CancelPendingTasks();
    reuseReadyPromise = true;
  }

  
  
  
  mIsRunningOnCompositor = false;

  if (!reuseReadyPromise) {
    
    mReady = nullptr;
  }

  mPendingState = PendingState::PausePending;

  nsIDocument* doc = GetRenderedDocument();
  if (!doc) {
    TriggerOnNextTick(Nullable<TimeDuration>());
    return;
  }

  PendingAnimationTracker* tracker = doc->GetOrCreatePendingAnimationTracker();
  tracker->AddPausePending(*this);

  UpdateTiming(SeekFlag::NoSeek);
}

void
Animation::ResumeAt(const TimeDuration& aReadyTime)
{
  
  
  
  MOZ_ASSERT(mPendingState == PendingState::PlayPending,
             "Expected to resume a play-pending animation");
  MOZ_ASSERT(mHoldTime.IsNull() != mStartTime.IsNull(),
             "An animation in the play-pending state should have either a"
             " resolved hold time or resolved start time (but not both)");

  
  
  if (mStartTime.IsNull()) {
    if (mPlaybackRate != 0) {
      mStartTime.SetValue(aReadyTime -
                          (mHoldTime.Value().MultDouble(1 / mPlaybackRate)));
      mHoldTime.SetNull();
    } else {
      mStartTime.SetValue(aReadyTime);
    }
  }
  mPendingState = PendingState::NotPending;

  UpdateTiming(SeekFlag::NoSeek);

  if (mReady) {
    mReady->MaybeResolve(this);
  }
}

void
Animation::PauseAt(const TimeDuration& aReadyTime)
{
  MOZ_ASSERT(mPendingState == PendingState::PausePending,
             "Expected to pause a pause-pending animation");

  if (!mStartTime.IsNull()) {
    mHoldTime.SetValue((aReadyTime - mStartTime.Value())
                        .MultDouble(mPlaybackRate));
  }
  mStartTime.SetNull();
  mPendingState = PendingState::NotPending;

  UpdateTiming(SeekFlag::NoSeek);

  if (mReady) {
    mReady->MaybeResolve(this);
  }
}

void
Animation::UpdateTiming(SeekFlag aSeekFlag)
{
  
  
  UpdateFinishedState(aSeekFlag);
  UpdateEffect();
}

void
Animation::UpdateFinishedState(SeekFlag aSeekFlag)
{
  Nullable<TimeDuration> currentTime = GetCurrentTime();
  TimeDuration effectEnd = TimeDuration(EffectEnd());

  if (!mStartTime.IsNull() &&
      mPendingState == PendingState::NotPending) {
    if (mPlaybackRate > 0.0 &&
        !currentTime.IsNull() &&
        currentTime.Value() >= effectEnd) {
      if (aSeekFlag == SeekFlag::DidSeek) {
        mHoldTime = currentTime;
      } else if (!mPreviousCurrentTime.IsNull()) {
        mHoldTime.SetValue(std::max(mPreviousCurrentTime.Value(), effectEnd));
      } else {
        mHoldTime.SetValue(effectEnd);
      }
    } else if (mPlaybackRate < 0.0 &&
               !currentTime.IsNull() &&
               currentTime.Value().ToMilliseconds() <= 0.0) {
      if (aSeekFlag == SeekFlag::DidSeek) {
        mHoldTime = currentTime;
      } else {
        mHoldTime.SetValue(0);
      }
    } else if (mPlaybackRate != 0.0 &&
               !currentTime.IsNull()) {
      if (aSeekFlag == SeekFlag::DidSeek && !mHoldTime.IsNull()) {
        mStartTime.SetValue(mTimeline->GetCurrentTime().Value() -
                              (mHoldTime.Value().MultDouble(1 / mPlaybackRate)));
      }
      mHoldTime.SetNull();
    }
  }

  bool currentFinishedState = PlayState() == AnimationPlayState::Finished;
  if (currentFinishedState && !mIsPreviousStateFinished) {
    if (mFinished) {
      mFinished->MaybeResolve(this);
    }
  } else if (!currentFinishedState && mIsPreviousStateFinished) {
    
    mFinished = nullptr;
    if (mEffect->AsTransition()) {
      mEffect->SetIsFinishedTransition(false);
    }
  }
  mIsPreviousStateFinished = currentFinishedState;
  
  
  mPreviousCurrentTime = GetCurrentTime();
}

void
Animation::UpdateEffect()
{
  if (mEffect) {
    mEffect->SetParentTime(GetCurrentTime());
    UpdateRelevance();
  }
}

void
Animation::FlushStyle() const
{
  nsIDocument* doc = GetRenderedDocument();
  if (doc) {
    doc->FlushPendingNotifications(Flush_Style);
  }
}

void
Animation::PostUpdate()
{
  AnimationCollection* collection = GetCollection();
  if (collection) {
    collection->NotifyAnimationUpdated();
  }
}

void
Animation::CancelPendingTasks()
{
  if (mPendingState == PendingState::NotPending) {
    return;
  }

  nsIDocument* doc = GetRenderedDocument();
  if (doc) {
    PendingAnimationTracker* tracker = doc->GetPendingAnimationTracker();
    if (tracker) {
      if (mPendingState == PendingState::PlayPending) {
        tracker->RemovePlayPending(*this);
      } else {
        tracker->RemovePausePending(*this);
      }
    }
  }

  mPendingState = PendingState::NotPending;
  mPendingReadyTime.SetNull();
}

bool
Animation::IsPossiblyOrphanedPendingAnimation() const
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  if (mPendingState == PendingState::NotPending) {
    return false;
  }

  
  
  if (!mPendingReadyTime.IsNull()) {
    return false;
  }

  
  
  if (!mTimeline || mTimeline->GetCurrentTime().IsNull()) {
    return false;
  }

  
  
  
  
  
  
  nsIDocument* doc = GetRenderedDocument();
  if (!doc) {
    return false;
  }

  PendingAnimationTracker* tracker = doc->GetPendingAnimationTracker();
  return !tracker ||
         (!tracker->IsWaitingToPlay(*this) &&
          !tracker->IsWaitingToPause(*this));
}

StickyTimeDuration
Animation::EffectEnd() const
{
  if (!mEffect) {
    return StickyTimeDuration(0);
  }

  return mEffect->Timing().mDelay
         + mEffect->GetComputedTiming().mActiveDuration;
}

nsIDocument*
Animation::GetRenderedDocument() const
{
  if (!mEffect) {
    return nullptr;
  }

  Element* targetElement;
  nsCSSPseudoElements::Type pseudoType;
  mEffect->GetTarget(targetElement, pseudoType);
  if (!targetElement) {
    return nullptr;
  }

  return targetElement->GetComposedDoc();
}

nsPresContext*
Animation::GetPresContext() const
{
  nsIDocument* doc = GetRenderedDocument();
  if (!doc) {
    return nullptr;
  }
  nsIPresShell* shell = doc->GetShell();
  if (!shell) {
    return nullptr;
  }
  return shell->GetPresContext();
}

AnimationCollection*
Animation::GetCollection() const
{
  css::CommonAnimationManager* manager = GetAnimationManager();
  if (!manager) {
    return nullptr;
  }
  MOZ_ASSERT(mEffect,
             "An animation with an animation manager must have an effect");

  Element* targetElement;
  nsCSSPseudoElements::Type targetPseudoType;
  mEffect->GetTarget(targetElement, targetPseudoType);
  MOZ_ASSERT(targetElement,
             "An animation with an animation manager must have a target");

  return manager->GetAnimations(targetElement, targetPseudoType, false);
}

} 
} 
