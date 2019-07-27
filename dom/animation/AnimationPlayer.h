




#ifndef mozilla_dom_AnimationPlayer_h
#define mozilla_dom_AnimationPlayer_h

#include "nsWrapperCache.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Attributes.h"
#include "mozilla/TimeStamp.h" 
#include "mozilla/dom/Animation.h" 
#include "mozilla/dom/AnimationPlayerBinding.h" 
#include "mozilla/dom/AnimationTimeline.h" 
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
  virtual ~AnimationPlayer() { }

public:
  explicit AnimationPlayer(AnimationTimeline* aTimeline)
    : mTimeline(aTimeline)
    , mIsPending(false)
    , mIsRunningOnCompositor(false)
    , mIsPreviousStateFinished(false)
  {
  }

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(AnimationPlayer)

  AnimationTimeline* GetParentObject() const { return mTimeline; }
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  virtual CSSAnimationPlayer* AsCSSAnimationPlayer() { return nullptr; }
  virtual CSSTransitionPlayer* AsCSSTransitionPlayer() { return nullptr; }

  
  Animation* GetSource() const { return mSource; }
  AnimationTimeline* Timeline() const { return mTimeline; }
  Nullable<TimeDuration> GetStartTime() const { return mStartTime; }
  Nullable<TimeDuration> GetCurrentTime() const;
  AnimationPlayState PlayState() const;
  virtual Promise* GetReady(ErrorResult& aRv);
  virtual void Play();
  virtual void Pause();
  bool IsRunningOnCompositor() const { return mIsRunningOnCompositor; }

  
  
  
  
  Nullable<double> GetStartTimeAsDouble() const;
  Nullable<double> GetCurrentTimeAsDouble() const;
  virtual AnimationPlayState PlayStateFromJS() const { return PlayState(); }
  virtual void PlayFromJS() { Play(); }
  
  
  
  void PauseFromJS() { Pause(); }

  void SetSource(Animation* aSource);
  void Tick();

  









































  void StartOnNextTick(const Nullable<TimeDuration>& aReadyTime);

  
  
  
  
  
  
  
  
  void StartNow();

  

















  Nullable<TimeDuration> GetCurrentOrPendingStartTime() const;

  void Cancel();

  const nsString& Name() const {
    return mSource ? mSource->Name() : EmptyString();
  }

  bool IsPaused() const { return PlayState() == AnimationPlayState::Paused; }
  bool IsRunning() const;

  bool HasCurrentSource() const {
    return GetSource() && GetSource()->IsCurrent();
  }
  bool HasInEffectSource() const {
    return GetSource() && GetSource()->IsInEffect();
  }

  void SetIsRunningOnCompositor() { mIsRunningOnCompositor = true; }
  void ClearIsRunningOnCompositor() { mIsRunningOnCompositor = false; }

  
  
  
  bool CanThrottle() const;

  
  
  
  
  
  
  
  void ComposeStyle(nsRefPtr<css::AnimValuesStyleRule>& aStyleRule,
                    nsCSSPropertySet& aSetProperties,
                    bool& aNeedsRefreshes);

protected:
  void DoPlay();
  void DoPause();
  void ResumeAt(const TimeDuration& aResumeTime);

  void UpdateSourceContent();
  void FlushStyle() const;
  void PostUpdate();
  
  
  
  void CancelPendingPlay();

  bool IsPossiblyOrphanedPendingPlayer() const;
  StickyTimeDuration SourceContentEnd() const;

  nsIDocument* GetRenderedDocument() const;
  nsPresContext* GetPresContext() const;
  virtual css::CommonAnimationManager* GetAnimationManager() const = 0;
  AnimationPlayerCollection* GetCollection() const;

  nsRefPtr<AnimationTimeline> mTimeline;
  nsRefPtr<Animation> mSource;
  
  Nullable<TimeDuration> mStartTime; 
  Nullable<TimeDuration> mHoldTime;  
  Nullable<TimeDuration> mPendingReadyTime; 

  
  
  
  nsRefPtr<Promise> mReady;

  
  
  
  
  
  bool mIsPending;
  bool mIsRunningOnCompositor;
  
  
  
  
  
  bool mIsPreviousStateFinished; 
};

} 
} 

#endif 
