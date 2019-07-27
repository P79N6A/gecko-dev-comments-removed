




#include "AnimationTimeline.h"
#include "mozilla/dom/AnimationTimelineBinding.h"
#include "nsContentUtils.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsRefreshDriver.h"
#include "nsDOMNavigationTiming.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(AnimationTimeline, mDocument)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(AnimationTimeline, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(AnimationTimeline, Release)

JSObject*
AnimationTimeline::WrapObject(JSContext* aCx)
{
  return AnimationTimelineBinding::Wrap(aCx, this);
}

Nullable<double>
AnimationTimeline::GetCurrentTime() const
{
  return ToTimelineTime(GetCurrentTimeStamp());
}

TimeStamp
AnimationTimeline::GetCurrentTimeStamp() const
{
  
  TimeStamp result = mLastCurrentTime;

  
  
  
  if (result.IsNull()) {
    nsRefPtr<nsDOMNavigationTiming> timing = mDocument->GetNavigationTiming();
    if (!timing) {
      return result;
    }
    result = timing->GetNavigationStartTimeStamp();
  }

  nsIPresShell* presShell = mDocument->GetShell();
  if (MOZ_UNLIKELY(!presShell)) {
    return result;
  }

  nsPresContext* presContext = presShell->GetPresContext();
  if (MOZ_UNLIKELY(!presContext)) {
    return result;
  }

  result = presContext->RefreshDriver()->MostRecentRefresh();
  
  
  
  
  mLastCurrentTime = result;
  return result;
}

Nullable<double>
AnimationTimeline::ToTimelineTime(const mozilla::TimeStamp& aTimeStamp) const
{
  Nullable<double> result; 
  if (aTimeStamp.IsNull()) {
    return result;
  }

  nsRefPtr<nsDOMNavigationTiming> timing = mDocument->GetNavigationTiming();
  if (MOZ_UNLIKELY(!timing)) {
    return result;
  }

  result.SetValue(timing->TimeStampToDOMHighRes(aTimeStamp));
  return result;
}

} 
} 
