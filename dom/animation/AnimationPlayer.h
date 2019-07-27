




#ifndef mozilla_dom_AnimationPlayer_h
#define mozilla_dom_AnimationPlayer_h

#include "nsWrapperCache.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Attributes.h"
#include "mozilla/TimeStamp.h" 
#include "mozilla/dom/Animation.h" 
#include "mozilla/dom/AnimationTimeline.h" 
#include "nsCSSProperty.h" 


#ifdef CurrentTime
#undef CurrentTime
#endif

struct JSContext;

namespace mozilla {

class CSSAnimationPlayer;

namespace dom {

class AnimationPlayer : public nsWrapperCache
{
protected:
  virtual ~AnimationPlayer() { }

public:
  explicit AnimationPlayer(AnimationTimeline* aTimeline)
    : mIsPaused(false)
    , mIsRunningOnCompositor(false)
    , mTimeline(aTimeline)
  {
  }

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(AnimationPlayer)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(AnimationPlayer)

  AnimationTimeline* GetParentObject() const { return mTimeline; }
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  virtual CSSAnimationPlayer* AsCSSAnimationPlayer() { return nullptr; }

  
  
  enum UpdateFlags {
    eNoUpdate,
    eUpdateStyle
  };

  
  Animation* GetSource() const { return mSource; }
  AnimationTimeline* Timeline() const { return mTimeline; }
  Nullable<double> GetStartTime() const;
  Nullable<double> GetCurrentTime() const;
  void Play(UpdateFlags aUpdateFlags);
  void Pause(UpdateFlags aUpdateFlags);
  bool IsRunningOnCompositor() const { return mIsRunningOnCompositor; }

  
  
  void PlayFromJS();
  void PauseFromJS();

  void SetSource(Animation* aSource);
  void Tick();

  const nsString& Name() const {
    return mSource ? mSource->Name() : EmptyString();
  }

  bool IsPaused() const { return mIsPaused; }

  bool IsRunning() const;

  bool HasCurrentSource() const {
    return GetSource() && GetSource()->IsCurrent();
  }
  bool HasInEffectSource() const {
    return GetSource() && GetSource()->IsInEffect();
  }

  
  
  Nullable<TimeDuration> GetCurrentTimeDuration() const;

  
  Nullable<TimeDuration> mStartTime; 
  Nullable<TimeDuration> mHoldTime;  
  bool mIsPaused;
  bool mIsRunningOnCompositor;

  nsRefPtr<AnimationTimeline> mTimeline;
  nsRefPtr<Animation> mSource;

protected:
  void FlushStyle() const;
  void MaybePostRestyle() const;
};

} 

class CSSAnimationPlayer MOZ_FINAL : public dom::AnimationPlayer
{
public:
 explicit CSSAnimationPlayer(dom::AnimationTimeline* aTimeline)
    : dom::AnimationPlayer(aTimeline)
  {
  }

  virtual CSSAnimationPlayer*
  AsCSSAnimationPlayer() MOZ_OVERRIDE { return this; }

protected:
  virtual ~CSSAnimationPlayer() { }
};

} 

#endif 
