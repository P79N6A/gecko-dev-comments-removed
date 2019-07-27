




#ifndef mozilla_dom_AnimationTimeline_h
#define mozilla_dom_AnimationTimeline_h

#include "nsWrapperCache.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Attributes.h"
#include "mozilla/TimeStamp.h"
#include "js/TypeDecls.h"
#include "nsIDocument.h"

struct JSContext;
class nsRefreshDriver;

namespace mozilla {
namespace dom {

class AnimationTimeline MOZ_FINAL : public nsWrapperCache
{
public:
  explicit AnimationTimeline(nsIDocument* aDocument)
    : mDocument(aDocument)
  {
  }

protected:
  virtual ~AnimationTimeline() { }

public:
  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(AnimationTimeline)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(AnimationTimeline)

  nsIGlobalObject* GetParentObject() const
  {
    return mDocument->GetParentObject();
  }
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  
  Nullable<TimeDuration> GetCurrentTime() const;

  
  
  Nullable<double> GetCurrentTimeAsDouble() const;

  Nullable<TimeDuration> ToTimelineTime(const TimeStamp& aTimeStamp) const;
  TimeStamp ToTimeStamp(const TimeDuration& aTimelineTime) const;

  
  
  
  
  
  
  
  
  
  
  
  
  
  void FastForward(const TimeStamp& aTimeStamp);

protected:
  TimeStamp GetCurrentTimeStamp() const;
  nsRefreshDriver* GetRefreshDriver() const;

  nsCOMPtr<nsIDocument> mDocument;

  
  
  
  mutable TimeStamp mLastRefreshDriverTime;

  
  
  mutable TimeStamp mFastForwardTime;
};

} 
} 

#endif 
