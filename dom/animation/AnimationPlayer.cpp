




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
                                      mSource, mReady)
NS_IMPL_CYCLE_COLLECTING_ADDREF(AnimationPlayer)
NS_IMPL_CYCLE_COLLECTING_RELEASE(AnimationPlayer)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(AnimationPlayer)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

JSObject*
AnimationPlayer::WrapObject(JSContext* aCx)
{
  return dom::AnimationPlayerBinding::Wrap(aCx, this);
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
    
    
    
    mHoldTime.SetNull();
  } else {
    mHoldTime = previousCurrentTime;
  }

  CancelPendingPlay();
  if (mReady) {
    
    
    mReady->MaybeResolve(this);
  }

  UpdateSourceContent();
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
      result.SetValue(timelineTime.Value() - mStartTime.Value());
    }
  }
  return result;
}

AnimationPlayState
AnimationPlayer::PlayState() const
{
  if (mIsPending) {
    return AnimationPlayState::Pending;
  }

  Nullable<TimeDuration> currentTime = GetCurrentTime();
  if (currentTime.IsNull()) {
    return AnimationPlayState::Idle;
  }

  if (mStartTime.IsNull()) {
    return AnimationPlayState::Paused;
  }

  if (currentTime.Value() >= SourceContentEnd()) {
    return AnimationPlayState::Finished;
  }

  return AnimationPlayState::Running;
}

Promise*
AnimationPlayer::GetReady(ErrorResult& aRv)
{
  
  if (!mReady) {
    nsIGlobalObject* global = mTimeline->GetParentObject();
    if (global) {
      mReady = Promise::Create(global, aRv);
      if (mReady && PlayState() != AnimationPlayState::Pending) {
        mReady->MaybeResolve(this);
      }
    }
  }
  if (!mReady) {
    aRv.Throw(NS_ERROR_FAILURE);
  }

  return mReady;
}

void
AnimationPlayer::Play()
{
  DoPlay();
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
  
  
  
  
  if (mIsPending &&
      !mPendingReadyTime.IsNull() &&
      mPendingReadyTime.Value() <= mTimeline->GetCurrentTime().Value()) {
    ResumeAt(mPendingReadyTime.Value());
    mPendingReadyTime.SetNull();
  }

  if (IsPossiblyOrphanedPendingPlayer()) {
    MOZ_ASSERT(mTimeline && !mTimeline->GetCurrentTime().IsNull(),
               "Orphaned pending players should have an active timeline");
    ResumeAt(mTimeline->GetCurrentTime().Value());
  }

  UpdateSourceContent();
}

void
AnimationPlayer::StartOnNextTick(const Nullable<TimeDuration>& aReadyTime)
{
  
  
  
  
  if (PlayState() != AnimationPlayState::Pending) {
    return;
  }

  
  
  mPendingReadyTime = aReadyTime;
}

void
AnimationPlayer::StartNow()
{
  MOZ_ASSERT(PlayState() == AnimationPlayState::Pending,
             "Expected to start a pending player");
  MOZ_ASSERT(mTimeline && !mTimeline->GetCurrentTime().IsNull(),
             "Expected an active timeline");

  ResumeAt(mTimeline->GetCurrentTime().Value());
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
  if (mIsPending) {
    CancelPendingPlay();
    if (mReady) {
      mReady->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
    }
  }

  mHoldTime.SetNull();
  mStartTime.SetNull();
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

  mIsPreviousStateFinished = (playState == AnimationPlayState::Finished);
}

void
AnimationPlayer::DoPlay()
{
  
  
  

  Nullable<TimeDuration> currentTime = GetCurrentTime();
  if (currentTime.IsNull()) {
    mHoldTime.SetValue(TimeDuration(0));
  } else if (mHoldTime.IsNull()) {
    
    return;
  }

  
  mReady = nullptr;

  mIsPending = true;

  nsIDocument* doc = GetRenderedDocument();
  if (!doc) {
    StartOnNextTick(Nullable<TimeDuration>());
    return;
  }

  PendingPlayerTracker* tracker = doc->GetOrCreatePendingPlayerTracker();
  tracker->AddPlayPending(*this);

  
  
  UpdateSourceContent();
}

void
AnimationPlayer::DoPause()
{
  if (mIsPending) {
    CancelPendingPlay();
    
    
    
    
    if (mReady) {
      mReady->MaybeResolve(this);
    }
  }

  
  
  
  mIsRunningOnCompositor = false;

  
  mHoldTime = GetCurrentTime();
  mStartTime.SetNull();
}

void
AnimationPlayer::ResumeAt(const TimeDuration& aResumeTime)
{
  
  
  
  MOZ_ASSERT(PlayState() == AnimationPlayState::Pending,
             "Expected to resume a pending player");
  MOZ_ASSERT(!mHoldTime.IsNull(),
             "A player in the pending state should have a resolved hold time");

  mStartTime.SetValue(aResumeTime - mHoldTime.Value());
  mHoldTime.SetNull();
  mIsPending = false;

  UpdateSourceContent();

  if (mReady) {
    mReady->MaybeResolve(this);
  }
}

void
AnimationPlayer::UpdateSourceContent()
{
  if (mSource) {
    mSource->SetParentTime(GetCurrentTime());
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
AnimationPlayer::CancelPendingPlay()
{
  if (!mIsPending) {
    return;
  }

  nsIDocument* doc = GetRenderedDocument();
  if (doc) {
    PendingPlayerTracker* tracker = doc->GetPendingPlayerTracker();
    if (tracker) {
      tracker->RemovePlayPending(*this);
    }
  }

  mIsPending = false;
  mPendingReadyTime.SetNull();
}

bool
AnimationPlayer::IsPossiblyOrphanedPendingPlayer() const
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  if (!mIsPending) {
    return false;
  }

  
  
  if (!mPendingReadyTime.IsNull()) {
    return false;
  }

  
  
  if (!mTimeline || mTimeline->GetCurrentTime().IsNull()) {
    return false;
  }

  
  
  
  
  
  
  nsIDocument* doc = GetRenderedDocument();
  return !doc ||
         !doc->GetPendingPlayerTracker() ||
         !doc->GetPendingPlayerTracker()->IsWaitingToPlay(*this);
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

  return manager->GetAnimationPlayers(targetElement, targetPseudoType, false);
}

} 
} 
