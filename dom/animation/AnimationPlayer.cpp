




#include "AnimationPlayer.h"
#include "AnimationUtils.h"
#include "mozilla/dom/AnimationPlayerBinding.h"
#include "AnimationCommon.h" 
                             
#include "nsIDocument.h" 
#include "nsIPresShell.h" 
#include "nsLayoutUtils.h" 
#include "PendingPlayerTracker.h" 

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(AnimationPlayer, mTimeline,
                                      mSource, mReady, mFinished)
NS_IMPL_CYCLE_COLLECTING_ADDREF(AnimationPlayer)
NS_IMPL_CYCLE_COLLECTING_RELEASE(AnimationPlayer)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(AnimationPlayer)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

JSObject*
AnimationPlayer::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return dom::AnimationPlayerBinding::Wrap(aCx, this, aGivenProto);
}

void
AnimationPlayer::SetStartTime(const Nullable<TimeDuration>& aNewStartTime)
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

  UpdateTiming();
  PostUpdate();
}

Nullable<TimeDuration>
AnimationPlayer::GetCurrentTime() const
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
AnimationPlayer::SilentlySetCurrentTime(const TimeDuration& aSeekTime)
{
  if (!mHoldTime.IsNull() ||
      !mTimeline ||
      mTimeline->GetCurrentTime().IsNull() ||
      mPlaybackRate == 0.0
      ) {
    mHoldTime.SetValue(aSeekTime);
    if (!mTimeline || mTimeline->GetCurrentTime().IsNull()) {
      mStartTime.SetNull();
    }
  } else {
    mStartTime.SetValue(mTimeline->GetCurrentTime().Value() -
                          (aSeekTime / mPlaybackRate));
  }

  mPreviousCurrentTime.SetNull();
}


void
AnimationPlayer::SetCurrentTime(const TimeDuration& aSeekTime)
{
  SilentlySetCurrentTime(aSeekTime);

  if (mPendingState == PendingState::PausePending) {
    CancelPendingTasks();
    if (mReady) {
      mReady->MaybeResolve(this);
    }
  }

  UpdateFinishedState(true);
  UpdateSourceContent();
  PostUpdate();
}

void
AnimationPlayer::SetPlaybackRate(double aPlaybackRate)
{
  Nullable<TimeDuration> previousTime = GetCurrentTime();
  mPlaybackRate = aPlaybackRate;
  if (!previousTime.IsNull()) {
    ErrorResult rv;
    SetCurrentTime(previousTime.Value());
    MOZ_ASSERT(!rv.Failed(), "Should not assert for non-null time");
  }
}

void
AnimationPlayer::SilentlySetPlaybackRate(double aPlaybackRate)
{
  Nullable<TimeDuration> previousTime = GetCurrentTime();
  mPlaybackRate = aPlaybackRate;
  if (!previousTime.IsNull()) {
    ErrorResult rv;
    SilentlySetCurrentTime(previousTime.Value());
    MOZ_ASSERT(!rv.Failed(), "Should not assert for non-null time");
  }
}

AnimationPlayState
AnimationPlayer::PlayState() const
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

  if ((mPlaybackRate > 0.0 && currentTime.Value() >= SourceContentEnd()) ||
      (mPlaybackRate < 0.0 && currentTime.Value().ToMilliseconds() <= 0.0)) {
    return AnimationPlayState::Finished;
  }

  return AnimationPlayState::Running;
}

static inline already_AddRefed<Promise>
CreatePromise(AnimationTimeline* aTimeline, ErrorResult& aRv)
{
  nsIGlobalObject* global = aTimeline->GetParentObject();
  if (global) {
    return Promise::Create(global, aRv);
  }
  return nullptr;
}

Promise*
AnimationPlayer::GetReady(ErrorResult& aRv)
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
AnimationPlayer::GetFinished(ErrorResult& aRv)
{
  if (!mFinished) {
    mFinished = CreatePromise(mTimeline, aRv); 
  }
  if (!mFinished) {
    aRv.Throw(NS_ERROR_FAILURE);
  } else if (IsFinished()) {
    mFinished->MaybeResolve(this);
  }
  return mFinished;
}

void
AnimationPlayer::Play(LimitBehavior aLimitBehavior)
{
  DoPlay(aLimitBehavior);
  PostUpdate();
}

void
AnimationPlayer::Pause()
{
  
  
  DoPause();
  PostUpdate();
}

Nullable<double>
AnimationPlayer::GetStartTimeAsDouble() const
{
  return AnimationUtils::TimeDurationToDouble(mStartTime);
}

void
AnimationPlayer::SetStartTimeAsDouble(const Nullable<double>& aStartTime)
{
  return SetStartTime(AnimationUtils::DoubleToTimeDuration(aStartTime));
}
  
Nullable<double>
AnimationPlayer::GetCurrentTimeAsDouble() const
{
  return AnimationUtils::TimeDurationToDouble(GetCurrentTime());
}

void
AnimationPlayer::SetCurrentTimeAsDouble(const Nullable<double>& aCurrentTime,
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
AnimationPlayer::SetSource(Animation* aSource)
{
  if (mSource) {
    mSource->SetParentTime(Nullable<TimeDuration>());
  }
  mSource = aSource;
  if (mSource) {
    mSource->SetParentTime(GetCurrentTime());
  }
  UpdateRelevance();
}

void
AnimationPlayer::Tick()
{
  
  
  
  
  if (mPendingState != PendingState::NotPending &&
      !mPendingReadyTime.IsNull() &&
      mPendingReadyTime.Value() <= mTimeline->GetCurrentTime().Value()) {
    FinishPendingAt(mPendingReadyTime.Value());
    mPendingReadyTime.SetNull();
  }

  if (IsPossiblyOrphanedPendingPlayer()) {
    MOZ_ASSERT(mTimeline && !mTimeline->GetCurrentTime().IsNull(),
               "Orphaned pending players should have an active timeline");
    FinishPendingAt(mTimeline->GetCurrentTime().Value());
  }

  UpdateTiming();
}

void
AnimationPlayer::TriggerOnNextTick(const Nullable<TimeDuration>& aReadyTime)
{
  
  
  
  if (PlayState() != AnimationPlayState::Pending) {
    return;
  }

  
  
  mPendingReadyTime = aReadyTime;
}

void
AnimationPlayer::TriggerNow()
{
  MOZ_ASSERT(PlayState() == AnimationPlayState::Pending,
             "Expected to start a pending player");
  MOZ_ASSERT(mTimeline && !mTimeline->GetCurrentTime().IsNull(),
             "Expected an active timeline");

  FinishPendingAt(mTimeline->GetCurrentTime().Value());
}

Nullable<TimeDuration>
AnimationPlayer::GetCurrentOrPendingStartTime() const
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
AnimationPlayer::Cancel()
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

  UpdateSourceContent();
}

void
AnimationPlayer::UpdateRelevance()
{
  bool wasRelevant = mIsRelevant;
  mIsRelevant = HasCurrentSource() || HasInEffectSource();

  
  if (wasRelevant && !mIsRelevant) {
    nsNodeUtils::AnimationRemoved(this);
  } else if (!wasRelevant && mIsRelevant) {
    nsNodeUtils::AnimationAdded(this);
  }
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

  
  
  
  
  return mIsPreviousStateFinished;
}

void
AnimationPlayer::ComposeStyle(nsRefPtr<css::AnimValuesStyleRule>& aStyleRule,
                              nsCSSPropertySet& aSetProperties,
                              bool& aNeedsRefreshes)
{
  if (!mSource || mSource->IsFinishedTransition()) {
    return;
  }

  AnimationPlayState playState = PlayState();
  if (playState == AnimationPlayState::Running ||
      playState == AnimationPlayState::Pending) {
    aNeedsRefreshes = true;
  }

  mSource->ComposeStyle(aStyleRule, aSetProperties);
}

void
AnimationPlayer::DoPlay(LimitBehavior aLimitBehavior)
{
  bool reuseReadyPromise = false;
  if (mPendingState != PendingState::NotPending) {
    CancelPendingTasks();
    reuseReadyPromise = true;
  }

  Nullable<TimeDuration> currentTime = GetCurrentTime();
  if (mPlaybackRate > 0.0 &&
      (currentTime.IsNull() ||
       (aLimitBehavior == LimitBehavior::AutoRewind &&
        (currentTime.Value().ToMilliseconds() < 0.0 ||
         currentTime.Value() >= SourceContentEnd())))) {
    mHoldTime.SetValue(TimeDuration(0));
  } else if (mPlaybackRate < 0.0 &&
             (currentTime.IsNull() ||
              (aLimitBehavior == LimitBehavior::AutoRewind &&
               (currentTime.Value().ToMilliseconds() <= 0.0 ||
                currentTime.Value() > SourceContentEnd())))) {
    mHoldTime.SetValue(TimeDuration(SourceContentEnd()));
  } else if (mPlaybackRate == 0.0 && currentTime.IsNull()) {
    mHoldTime.SetValue(TimeDuration(0));
  }

  if (mHoldTime.IsNull()) {
    return;
  }

  
  mStartTime.SetNull();

  if (!reuseReadyPromise) {
    
    mReady = nullptr;
  }

  mPendingState = PendingState::PlayPending;

  nsIDocument* doc = GetRenderedDocument();
  if (!doc) {
    TriggerOnNextTick(Nullable<TimeDuration>());
    return;
  }

  PendingPlayerTracker* tracker = doc->GetOrCreatePendingPlayerTracker();
  tracker->AddPlayPending(*this);

  
  UpdateTiming();
}

void
AnimationPlayer::DoPause()
{
  if (mPendingState == PendingState::PlayPending) {
    CancelPendingTasks();
    
    
    
    
    if (mReady) {
      mReady->MaybeResolve(this);
    }
  }

  
  
  
  mIsRunningOnCompositor = false;

  
  mHoldTime = GetCurrentTime();
  mStartTime.SetNull();

  UpdateFinishedState();
}

void
AnimationPlayer::ResumeAt(const TimeDuration& aReadyTime)
{
  
  
  
  MOZ_ASSERT(mPendingState == PendingState::PlayPending,
             "Expected to resume a play-pending player");
  MOZ_ASSERT(!mHoldTime.IsNull(),
             "A player in the play-pending state should have a resolved"
             " hold time");

  if (mPlaybackRate != 0) {
    mStartTime.SetValue(aReadyTime - (mHoldTime.Value() / mPlaybackRate));
    mHoldTime.SetNull();
  } else {
    mStartTime.SetValue(aReadyTime);
  }
  mPendingState = PendingState::NotPending;

  UpdateTiming();

  if (mReady) {
    mReady->MaybeResolve(this);
  }
}

void
AnimationPlayer::PauseAt(const TimeDuration& aReadyTime)
{
  MOZ_ASSERT(mPendingState == PendingState::PausePending,
             "Expected to pause a pause-pending player");

  if (!mStartTime.IsNull()) {
    mHoldTime.SetValue((aReadyTime - mStartTime.Value())
                        .MultDouble(mPlaybackRate));
  }
  mStartTime.SetNull();
  mPendingState = PendingState::NotPending;

  UpdateTiming();

  if (mReady) {
    mReady->MaybeResolve(this);
  }
}

void
AnimationPlayer::UpdateTiming()
{
  
  
  UpdateFinishedState();
  UpdateSourceContent();
}

void
AnimationPlayer::UpdateFinishedState(bool aSeekFlag)
{
  Nullable<TimeDuration> currentTime = GetCurrentTime();
  TimeDuration targetEffectEnd = TimeDuration(SourceContentEnd());

  if (!mStartTime.IsNull() &&
      mPendingState == PendingState::NotPending) {
    if (mPlaybackRate > 0.0 &&
        !currentTime.IsNull() &&
        currentTime.Value() >= targetEffectEnd) {
      if (aSeekFlag) {
        mHoldTime = currentTime;
      } else if (!mPreviousCurrentTime.IsNull()) {
        mHoldTime.SetValue(std::max(mPreviousCurrentTime.Value(),
                                    targetEffectEnd));
      } else {
        mHoldTime.SetValue(targetEffectEnd);
      }
    } else if (mPlaybackRate < 0.0 &&
               !currentTime.IsNull() &&
               currentTime.Value().ToMilliseconds() <= 0.0) {
      if (aSeekFlag) {
        mHoldTime = currentTime;
      } else {
        mHoldTime.SetValue(0);
      }
    } else if (mPlaybackRate != 0.0 &&
               !currentTime.IsNull()) {
      if (aSeekFlag && !mHoldTime.IsNull()) {
        mStartTime.SetValue(mTimeline->GetCurrentTime().Value() -
                              (mHoldTime.Value() / mPlaybackRate));
      }
      mHoldTime.SetNull();
    }
  }

  bool currentFinishedState = IsFinished();
  if (currentFinishedState && !mIsPreviousStateFinished) {
    if (mFinished) {
      mFinished->MaybeResolve(this);
    }
  } else if (!currentFinishedState && mIsPreviousStateFinished) {
    
    mFinished = nullptr;
  }
  mIsPreviousStateFinished = currentFinishedState;
  
  
  mPreviousCurrentTime = GetCurrentTime();
}

void
AnimationPlayer::UpdateSourceContent()
{
  if (mSource) {
    mSource->SetParentTime(GetCurrentTime());
    UpdateRelevance();
  }
}

void
AnimationPlayer::FlushStyle() const
{
  nsIDocument* doc = GetRenderedDocument();
  if (doc) {
    doc->FlushPendingNotifications(Flush_Style);
  }
}

void
AnimationPlayer::PostUpdate()
{
  AnimationPlayerCollection* collection = GetCollection();
  if (collection) {
    collection->NotifyPlayerUpdated();
  }
}

void
AnimationPlayer::CancelPendingTasks()
{
  if (mPendingState == PendingState::NotPending) {
    return;
  }

  nsIDocument* doc = GetRenderedDocument();
  if (doc) {
    PendingPlayerTracker* tracker = doc->GetPendingPlayerTracker();
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
AnimationPlayer::IsFinished() const
{
  
  
  
  
  Nullable<TimeDuration> currentTime = GetCurrentTime();
  return !currentTime.IsNull() &&
      ((mPlaybackRate > 0.0 && currentTime.Value() >= SourceContentEnd()) ||
       (mPlaybackRate < 0.0 && currentTime.Value().ToMilliseconds() <= 0.0));
}

bool
AnimationPlayer::IsPossiblyOrphanedPendingPlayer() const
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

  PendingPlayerTracker* tracker = doc->GetPendingPlayerTracker();
  return !tracker ||
         (!tracker->IsWaitingToPlay(*this) &&
          !tracker->IsWaitingToPause(*this));
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

nsIDocument*
AnimationPlayer::GetRenderedDocument() const
{
  if (!mSource) {
    return nullptr;
  }

  Element* targetElement;
  nsCSSPseudoElements::Type pseudoType;
  mSource->GetTarget(targetElement, pseudoType);
  if (!targetElement) {
    return nullptr;
  }

  return targetElement->GetComposedDoc();
}

nsPresContext*
AnimationPlayer::GetPresContext() const
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

AnimationPlayerCollection*
AnimationPlayer::GetCollection() const
{
  css::CommonAnimationManager* manager = GetAnimationManager();
  if (!manager) {
    return nullptr;
  }
  MOZ_ASSERT(mSource, "A player with an animation manager must have a source");

  Element* targetElement;
  nsCSSPseudoElements::Type targetPseudoType;
  mSource->GetTarget(targetElement, targetPseudoType);
  MOZ_ASSERT(targetElement,
             "A player with an animation manager must have a target");

  return manager->GetAnimations(targetElement, targetPseudoType, false);
}

} 
} 
