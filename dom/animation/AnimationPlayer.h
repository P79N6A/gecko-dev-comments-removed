




#ifndef mozilla_dom_AnimationPlayer_h
#define mozilla_dom_AnimationPlayer_h

#include <algorithm> 
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
namespace dom {

class AnimationPlayer MOZ_FINAL : public nsWrapperCache
{
protected:
  virtual ~AnimationPlayer() { }

public:
  explicit AnimationPlayer(AnimationTimeline* aTimeline)
    : mIsRunningOnCompositor(false)
    , mTimeline(aTimeline)
  {
    SetIsDOMBinding();
  }

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(AnimationPlayer)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(AnimationPlayer)

  AnimationTimeline* GetParentObject() const { return mTimeline; }
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  
  Animation* GetSource() const { return mSource; }
  AnimationTimeline* Timeline() const { return mTimeline; }
  double StartTime() const;
  double CurrentTime() const;

  void SetSource(Animation* aSource);
  void Tick();

  bool IsPaused() const {
    return mPlayState == NS_STYLE_ANIMATION_PLAY_STATE_PAUSED;
  }

  bool IsRunning() const;
  bool IsCurrent() const;

  
  
  
  
  
  Nullable<TimeDuration> GetCurrentTimeDuration() const {
    const TimeStamp& timelineTime = mTimeline->GetCurrentTimeStamp();
    
    
    MOZ_ASSERT(timelineTime.IsNull() || !IsPaused() ||
               timelineTime >= mPauseStart,
               "if paused, any non-null value of aTime must be at least"
               " mPauseStart");

    Nullable<TimeDuration> result; 
    if (!timelineTime.IsNull() && !mStartTime.IsNull()) {
      result.SetValue((IsPaused() ? mPauseStart : timelineTime) - mStartTime);
    }
    return result;
  }

  nsString mName;
  
  TimeStamp mStartTime;
  TimeStamp mPauseStart;
  uint8_t mPlayState;
  bool mIsRunningOnCompositor;

  nsRefPtr<AnimationTimeline> mTimeline;
  nsRefPtr<Animation> mSource;
};

} 
} 

#endif 
