




#ifndef mozilla_dom_AnimationTimeline_h
#define mozilla_dom_AnimationTimeline_h

#include "nsWrapperCache.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Attributes.h"
#include "mozilla/TimeStamp.h"
#include "nsIGlobalObject.h"
#include "js/TypeDecls.h"
#include "nsIDocument.h"
#include "nsRefreshDriver.h"

struct JSContext;

namespace mozilla {
namespace dom {

class AnimationTimeline MOZ_FINAL : public nsWrapperCache
{
public:
  explicit AnimationTimeline(nsIDocument* aDocument)
    : mDocument(aDocument)
    , mWindow(aDocument->GetParentObject())
  {
    MOZ_ASSERT(mWindow);
  }

protected:
  virtual ~AnimationTimeline() { }

public:
  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(AnimationTimeline)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(AnimationTimeline)

  nsIGlobalObject* GetParentObject() const
  {
    return mWindow;
  }
  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) MOZ_OVERRIDE;

  
  Nullable<TimeDuration> GetCurrentTime() const;

  
  
  Nullable<double> GetCurrentTimeAsDouble() const;

  
  
  
  
  
  Nullable<TimeDuration> ToTimelineTime(const TimeStamp& aTimeStamp) const;
  TimeStamp ToTimeStamp(const TimeDuration& aTimelineTime) const;

  nsRefreshDriver* GetRefreshDriver() const;
  
  
  
  
  
  bool IsUnderTestControl() const
  {
    nsRefreshDriver* refreshDriver = GetRefreshDriver();
    return refreshDriver && refreshDriver->IsTestControllingRefreshesEnabled();
  }

protected:
  TimeStamp GetCurrentTimeStamp() const;

  
  
  
  
  nsCOMPtr<nsIDocument> mDocument;
  nsCOMPtr<nsIGlobalObject> mWindow;

  
  
  
  mutable TimeStamp mLastRefreshDriverTime;
};

} 
} 

#endif 
