





#ifndef mozilla_dom_AnimationTimeline_h
#define mozilla_dom_AnimationTimeline_h

#include "nsISupports.h"
#include "nsWrapperCache.h"
#include "nsCycleCollectionParticipant.h"
#include "js/TypeDecls.h"
#include "mozilla/AnimationUtils.h"
#include "mozilla/Attributes.h"
#include "nsHashKeys.h"
#include "nsIGlobalObject.h"
#include "nsTHashtable.h"

namespace mozilla {
namespace dom {

class AnimationTimeline
  : public nsISupports
  , public nsWrapperCache
{
public:
  explicit AnimationTimeline(nsIGlobalObject* aWindow)
    : mWindow(aWindow)
  {
    MOZ_ASSERT(mWindow);
  }

protected:
  virtual ~AnimationTimeline() { }

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(AnimationTimeline)

  nsIGlobalObject* GetParentObject() const { return mWindow; }

  typedef nsTArray<nsRefPtr<Animation>> AnimationSequence;

  
  virtual Nullable<TimeDuration> GetCurrentTime() const = 0;
  void GetAnimations(AnimationSequence& aAnimations);

  
  
  Nullable<double> GetCurrentTimeAsDouble() const {
    return AnimationUtils::TimeDurationToDouble(GetCurrentTime());
  }

  








  virtual bool TracksWallclockTime() const = 0;

  






  virtual Nullable<TimeDuration> ToTimelineTime(const TimeStamp&
                                                  aTimeStamp) const = 0;

  virtual TimeStamp ToTimeStamp(const TimeDuration& aTimelineTime) const = 0;

  void AddAnimation(Animation& aAnimation);
  void RemoveAnimation(Animation& aAnimation);

protected:
  nsCOMPtr<nsIGlobalObject> mWindow;

  
  typedef nsTHashtable<nsRefPtrHashKey<dom::Animation>> AnimationSet;
  AnimationSet mAnimations;
};

} 
} 

#endif 
