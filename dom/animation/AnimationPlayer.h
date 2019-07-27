




#ifndef mozilla_dom_AnimationPlayer_h
#define mozilla_dom_AnimationPlayer_h

#include "nsWrapperCache.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Attributes.h"
#include "mozilla/TimeStamp.h" 
#include "mozilla/dom/AnimationPlayerBinding.h" 
#include "mozilla/dom/DocumentTimeline.h" 
#include "mozilla/dom/KeyframeEffect.h" 
#include "mozilla/dom/Promise.h" 
#include "nsCSSProperty.h" 


#ifdef CurrentTime
#undef CurrentTime
#endif



#ifdef GetCurrentTime
#undef GetCurrentTime
#endif

struct JSContext;
class nsCSSPropertySet;
class nsIDocument;
class nsPresContext;

namespace mozilla {
struct AnimationPlayerCollection;
namespace css {
class AnimValuesStyleRule;
class CommonAnimationManager;
} 

class CSSAnimationPlayer;
class CSSTransitionPlayer;

namespace dom {

class AnimationPlayer : public nsISupports,
                        public nsWrapperCache
{
protected:
  virtual ~AnimationPlayer() {}

public:
  explicit AnimationPlayer(DocumentTimeline* aTimeline)
    : mTimeline(aTimeline)
    , mPlaybackRate(1.0)
    , mPendingState(PendingState::NotPending)
    , mIsRunningOnCompositor(false)
    , mIsPreviousStateFinished(false)
    , mFinishedAtLastComposeStyle(false)
    , mIsRelevant(false)
  {
  }

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(AnimationPlayer)

  DocumentTimeline* GetParentObject() const { return mTimeline; }
  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aGivenProto) override;

  virtual CSSAnimationPlayer* AsCSSAnimationPlayer() { return nullptr; }
  virtual CSSTransitionPlayer* AsCSSTransitionPlayer() { return nullptr; }

  
  
  enum LimitBehavior {
    AutoRewind = 0,
    Continue = 1
  };

  
  KeyframeEffectReadonly* GetSource() const { return mSource; }
  DocumentTimeline* Timeline() const { return mTimeline; }
  Nullable<TimeDuration> GetStartTime() const { return mStartTime; }
  void SetStartTime(const Nullable<TimeDuration>& aNewStartTime);
  Nullable<TimeDuration> GetCurrentTime() const;
  void SilentlySetCurrentTime(const TimeDuration& aNewCurrentTime);
  void SetCurrentTime(const TimeDuration& aNewCurrentTime);
  double PlaybackRate() const { return mPlaybackRate; }
  void SetPlaybackRate(double aPlaybackRate);
  void SilentlySetPlaybackRate(double aPlaybackRate);
  AnimationPlayState PlayState() const;
  virtual Promise* GetReady(ErrorResult& aRv);
  virtual Promise* GetFinished(ErrorResult& aRv);
  virtual void Play(LimitBehavior aLimitBehavior);
  virtual void Pause();
  bool IsRunningOnCompositor() const { return mIsRunningOnCompositor; }

  
  
  
  
  Nullable<double> GetStartTimeAsDouble() const;
  void SetStartTimeAsDouble(const Nullable<double>& aStartTime);
  Nullable<double> GetCurrentTimeAsDouble() const;
  void SetCurrentTimeAsDouble(const Nullable<double>& aCurrentTime,
                              ErrorResult& aRv);
  virtual AnimationPlayState PlayStateFromJS() const { return PlayState(); }
  virtual void PlayFromJS() { Play(LimitBehavior::AutoRewind); }
  
  
  
  void PauseFromJS() { Pause(); }

  void SetSource(KeyframeEffectReadonly* aSource);
  void Tick();

  


















































  void TriggerOnNextTick(const Nullable<TimeDuration>& aReadyTime);

  
  
  
  
  
  
  
  
  void TriggerNow();

  

















  Nullable<TimeDuration> GetCurrentOrPendingStartTime() const;

  void Cancel();

  const nsString& Name() const
  {
    return mSource ? mSource->Name() : EmptyString();
  }

  bool IsPausedOrPausing() const
  {
    return PlayState() == AnimationPlayState::Paused ||
           mPendingState == PendingState::PausePending;
  }

  bool HasInPlaySource() const
  {
    return GetSource() && GetSource()->IsInPlay(*this);
  }
  bool HasCurrentSource() const
  {
    return GetSource() && GetSource()->IsCurrent(*this);
  }
  bool HasInEffectSource() const
  {
    return GetSource() && GetSource()->IsInEffect();
  }

  







  bool IsPlaying() const
  {
    
    
    return HasInPlaySource() &&
           (PlayState() == AnimationPlayState::Running ||
            mPendingState == PendingState::PlayPending);
  }

  bool IsRelevant() const { return mIsRelevant; }
  void UpdateRelevance();

  void SetIsRunningOnCompositor() { mIsRunningOnCompositor = true; }
  void ClearIsRunningOnCompositor() { mIsRunningOnCompositor = false; }

  
  
  
  bool CanThrottle() const;

  
  
  
  
  
  
  
  void ComposeStyle(nsRefPtr<css::AnimValuesStyleRule>& aStyleRule,
                    nsCSSPropertySet& aSetProperties,
                    bool& aNeedsRefreshes);

protected:
  void DoPlay(LimitBehavior aLimitBehavior);
  void DoPause();
  void ResumeAt(const TimeDuration& aReadyTime);
  void PauseAt(const TimeDuration& aReadyTime);
  void FinishPendingAt(const TimeDuration& aReadyTime)
  {
    if (mPendingState == PendingState::PlayPending) {
      ResumeAt(aReadyTime);
    } else if (mPendingState == PendingState::PausePending) {
      PauseAt(aReadyTime);
    } else {
      NS_NOTREACHED("Can't finish pending if we're not in a pending state");
    }
  }

  void UpdateTiming();
  void UpdateFinishedState(bool aSeekFlag = false);
  void UpdateSourceContent();
  void FlushStyle() const;
  void PostUpdate();
  




  void CancelPendingTasks();

  bool IsFinished() const;

  bool IsPossiblyOrphanedPendingPlayer() const;
  StickyTimeDuration SourceContentEnd() const;

  nsIDocument* GetRenderedDocument() const;
  nsPresContext* GetPresContext() const;
  virtual css::CommonAnimationManager* GetAnimationManager() const = 0;
  AnimationPlayerCollection* GetCollection() const;

  nsRefPtr<DocumentTimeline> mTimeline;
  nsRefPtr<KeyframeEffectReadonly> mSource;
  
  Nullable<TimeDuration> mStartTime; 
  Nullable<TimeDuration> mHoldTime;  
  Nullable<TimeDuration> mPendingReadyTime; 
  Nullable<TimeDuration> mPreviousCurrentTime; 
  double mPlaybackRate;

  
  
  
  
  nsRefPtr<Promise> mReady;

  
  
  
  
  
  nsRefPtr<Promise> mFinished;

  
  
  
  
  
  
  enum class PendingState { NotPending, PlayPending, PausePending };
  PendingState mPendingState;

  bool mIsRunningOnCompositor;
  
  
  bool mIsPreviousStateFinished; 
  bool mFinishedAtLastComposeStyle;
  
  
  bool mIsRelevant;
};

} 
} 

#endif
