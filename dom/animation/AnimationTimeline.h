




#ifndef mozilla_dom_AnimationTimeline_h
#define mozilla_dom_AnimationTimeline_h

#include "nsWrapperCache.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Attributes.h"
#include "mozilla/TimeStamp.h"
#include "js/TypeDecls.h"
#include "nsIDocument.h"

struct JSContext;

namespace mozilla {
namespace dom {

class AnimationTimeline MOZ_FINAL : public nsWrapperCache
{
public:
  explicit AnimationTimeline(nsIDocument* aDocument)
    : mDocument(aDocument)
  {
    SetIsDOMBinding();
  }

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(AnimationTimeline)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(AnimationTimeline)

  nsISupports* GetParentObject() const { return mDocument; }
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  
  Nullable<double> GetCurrentTime() const;

  Nullable<TimeDuration> GetCurrentTimeDuration() const;

  Nullable<TimeDuration> ToTimelineTime(const TimeStamp& aTimeStamp) const;
  TimeStamp ToTimeStamp(const TimeDuration& aTimelineTime) const;

  TimeStamp GetCurrentTimeStamp() const;

protected:
  virtual ~AnimationTimeline() { }

  nsCOMPtr<nsIDocument> mDocument;

  
  
  
  mutable TimeStamp mLastCurrentTime;
};

} 
} 

#endif 
