





#ifndef mozilla_dom_Animation_h
#define mozilla_dom_Animation_h

#include "nsWrapperCache.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Attributes.h"
#include "mozilla/TimeStamp.h" 
#include "mozilla/dom/AnimationBinding.h" 
#include "mozilla/dom/AnimationTimeline.h" 
#include "mozilla/dom/KeyframeEffect.h" 
#include "mozilla/dom/Promise.h" 
#include "nsCSSProperty.h" 
#include "nsIGlobalObject.h"


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
struct AnimationCollection;
namespace css {
class AnimValuesStyleRule;
class CommonAnimationManager;
} 

namespace dom {

class CSSAnimation;
class CSSTransition;

class Animation
  : public nsISupports
  , public nsWrapperCache
{
protected:
  virtual ~Animation() {}

public:
  explicit Animation(nsIGlobalObject* aGlobal)
    : mGlobal(aGlobal)
    , mPlaybackRate(1.0)
    , mPendingState(PendingState::NotPending)
    , mSequenceNum(kUnsequenced)
    , mIsRunningOnCompositor(false)
    , mIsPreviousStateFinished(false)
    , mFinishedAtLastComposeStyle(false)
    , mIsRelevant(false)
  {
  }

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(Animation)

  AnimationTimeline* GetParentObject() const { return mTimeline; }
  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aGivenProto) override;

  virtual CSSAnimation* AsCSSAnimation() { return nullptr; }
  virtual const CSSAnimation* AsCSSAnimation() const { return nullptr; }
  virtual CSSTransition* AsCSSTransition() { return nullptr; }
  virtual const CSSTransition* AsCSSTransition() const { return nullptr; }

  





  enum class LimitBehavior {
    AutoRewind,
    Continue
  };

  

  KeyframeEffectReadOnly* GetEffect() const { return mEffect; }
  void SetEffect(KeyframeEffectReadOnly* aEffect);
  AnimationTimeline* GetTimeline() const { return mTimeline; }
  void SetTimeline(AnimationTimeline* aTimeline);
  Nullable<TimeDuration> GetStartTime() const { return mStartTime; }
  void SetStartTime(const Nullable<TimeDuration>& aNewStartTime);
  Nullable<TimeDuration> GetCurrentTime() const;
  void SetCurrentTime(const TimeDuration& aNewCurrentTime);
  double PlaybackRate() const { return mPlaybackRate; }
  void SetPlaybackRate(double aPlaybackRate);
  AnimationPlayState PlayState() const;
  virtual Promise* GetReady(ErrorResult& aRv);
  virtual Promise* GetFinished(ErrorResult& aRv);
  void Cancel();
  virtual void Finish(ErrorResult& aRv);
  virtual void Play(ErrorResult& aRv, LimitBehavior aLimitBehavior);
  virtual void Pause(ErrorResult& aRv);
  bool IsRunningOnCompositor() const { return mIsRunningOnCompositor; }

  
  
  
  
  
  
  Nullable<double> GetStartTimeAsDouble() const;
  void SetStartTimeAsDouble(const Nullable<double>& aStartTime);
  Nullable<double> GetCurrentTimeAsDouble() const;
  void SetCurrentTimeAsDouble(const Nullable<double>& aCurrentTime,
                              ErrorResult& aRv);
  virtual AnimationPlayState PlayStateFromJS() const { return PlayState(); }
  virtual void PlayFromJS(ErrorResult& aRv)
  {
    Play(aRv, LimitBehavior::AutoRewind);
  }
  




  void PauseFromJS(ErrorResult& aRv) { Pause(aRv); }

  

  virtual void CancelFromStyle() { DoCancel(); }

  void Tick();

  


















































  void TriggerOnNextTick(const Nullable<TimeDuration>& aReadyTime);
  









  void TriggerNow();
  

















  Nullable<TimeDuration> GetCurrentOrPendingStartTime() const;

  bool IsPausedOrPausing() const
  {
    return PlayState() == AnimationPlayState::Paused ||
           mPendingState == PendingState::PausePending;
  }

  bool HasInPlayEffect() const
  {
    return GetEffect() && GetEffect()->IsInPlay(*this);
  }
  bool HasCurrentEffect() const
  {
    return GetEffect() && GetEffect()->IsCurrent(*this);
  }
  bool IsInEffect() const
  {
    return GetEffect() && GetEffect()->IsInEffect();
  }

  






  bool IsPlaying() const
  {
    
    
    return HasInPlayEffect() &&
           (PlayState() == AnimationPlayState::Running ||
            mPendingState == PendingState::PlayPending);
  }
  bool IsRelevant() const { return mIsRelevant; }
  void UpdateRelevance();

  


  virtual bool HasLowerCompositeOrderThan(const Animation& aOther) const;
  






  virtual bool IsUsingCustomCompositeOrder() const { return false; }

  void SetIsRunningOnCompositor() { mIsRunningOnCompositor = true; }
  void ClearIsRunningOnCompositor() { mIsRunningOnCompositor = false; }
  




  bool CanThrottle() const;
  








  void ComposeStyle(nsRefPtr<css::AnimValuesStyleRule>& aStyleRule,
                    nsCSSPropertySet& aSetProperties,
                    bool& aNeedsRefreshes);
protected:
  void SilentlySetCurrentTime(const TimeDuration& aNewCurrentTime);
  void SilentlySetPlaybackRate(double aPlaybackRate);
  void DoCancel();
  void DoPlay(ErrorResult& aRv, LimitBehavior aLimitBehavior);
  void DoPause(ErrorResult& aRv);
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

  



  enum class SeekFlag {
    NoSeek,
    DidSeek
  };

  void UpdateTiming(SeekFlag aSeekFlag);
  void UpdateFinishedState(SeekFlag aSeekFlag);
  void UpdateEffect();
  void FlushStyle() const;
  void PostUpdate();

  




  void CancelPendingTasks();

  bool IsPossiblyOrphanedPendingAnimation() const;
  StickyTimeDuration EffectEnd() const;

  nsIDocument* GetRenderedDocument() const;
  nsPresContext* GetPresContext() const;
  virtual css::CommonAnimationManager* GetAnimationManager() const = 0;
  AnimationCollection* GetCollection() const;

  nsCOMPtr<nsIGlobalObject> mGlobal;
  nsRefPtr<AnimationTimeline> mTimeline;
  nsRefPtr<KeyframeEffectReadOnly> mEffect;
  
  Nullable<TimeDuration> mStartTime; 
  Nullable<TimeDuration> mHoldTime;  
  Nullable<TimeDuration> mPendingReadyTime; 
  Nullable<TimeDuration> mPreviousCurrentTime; 
  double mPlaybackRate;

  
  
  
  
  nsRefPtr<Promise> mReady;

  
  
  
  
  
  nsRefPtr<Promise> mFinished;

  
  
  
  
  
  
  enum class PendingState { NotPending, PlayPending, PausePending };
  PendingState mPendingState;

  static uint64_t sNextSequenceNum;
  static const uint64_t kUnsequenced = UINT64_MAX;

  
  
  
  uint64_t mSequenceNum;

  bool mIsRunningOnCompositor;
  
  
  bool mIsPreviousStateFinished; 
  bool mFinishedAtLastComposeStyle;
  
  
  bool mIsRelevant;
};

} 
} 

#endif
