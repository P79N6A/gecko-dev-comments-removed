




#include "AnimationTimeline.h"
#include "mozilla/dom/AnimationTimelineBinding.h"
#include "AnimationUtils.h"
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

Nullable<TimeDuration>
AnimationTimeline::GetCurrentTime() const
{
  return ToTimelineTime(GetCurrentTimeStamp());
}

Nullable<double>
AnimationTimeline::GetCurrentTimeAsDouble() const
{
  return AnimationUtils::TimeDurationToDouble(GetCurrentTime());
}

void
AnimationTimeline::FastForward(const TimeStamp& aTimeStamp)
{
  
  
  if (!mFastForwardTime.IsNull() && aTimeStamp <= mFastForwardTime) {
    return;
  }

  
  
  
  
  
  
  
  
  nsRefreshDriver* refreshDriver = GetRefreshDriver();
  if (refreshDriver && refreshDriver->IsTestControllingRefreshesEnabled()) {
    return;
  }

  
  
  
  if (refreshDriver &&
      aTimeStamp < refreshDriver->MostRecentRefresh()) {
    mFastForwardTime = refreshDriver->MostRecentRefresh();
    return;
  }

  
  
  
  

  mFastForwardTime = aTimeStamp;
}

TimeStamp
AnimationTimeline::GetCurrentTimeStamp() const
{
  nsRefreshDriver* refreshDriver = GetRefreshDriver();
  TimeStamp refreshTime = refreshDriver
                          ? refreshDriver->MostRecentRefresh()
                          : TimeStamp();

  
  TimeStamp result = !refreshTime.IsNull()
                     ? refreshTime
                     : mLastRefreshDriverTime;

  
  
  if (result.IsNull()) {
    nsRefPtr<nsDOMNavigationTiming> timing = mDocument->GetNavigationTiming();
    if (timing) {
      result = timing->GetNavigationStartTimeStamp();
      
      
      
      refreshTime = result;
    }
  }

  
  
  
  
  
  
  
  
  
  
  MOZ_ASSERT(refreshTime.IsNull() || mLastRefreshDriverTime.IsNull() ||
             refreshTime >= mLastRefreshDriverTime ||
             mFastForwardTime.IsNull(),
             "The refresh driver time should not go backwards when the"
             " fast-forward time is set");

  
  
  
  
  
  if (result.IsNull() ||
       (!mFastForwardTime.IsNull() && mFastForwardTime > result)) {
    result = mFastForwardTime;
  } else {
    
    mFastForwardTime = TimeStamp();
  }

  if (!refreshTime.IsNull()) {
    mLastRefreshDriverTime = refreshTime;
  }

  return result;
}

Nullable<TimeDuration>
AnimationTimeline::ToTimelineTime(const TimeStamp& aTimeStamp) const
{
  Nullable<TimeDuration> result; 
  if (aTimeStamp.IsNull()) {
    return result;
  }

  nsRefPtr<nsDOMNavigationTiming> timing = mDocument->GetNavigationTiming();
  if (MOZ_UNLIKELY(!timing)) {
    return result;
  }

  result.SetValue(aTimeStamp - timing->GetNavigationStartTimeStamp());
  return result;
}

TimeStamp
AnimationTimeline::ToTimeStamp(const TimeDuration& aTimeDuration) const
{
  TimeStamp result;
  nsRefPtr<nsDOMNavigationTiming> timing = mDocument->GetNavigationTiming();
  if (MOZ_UNLIKELY(!timing)) {
    return result;
  }

  result = timing->GetNavigationStartTimeStamp() + aTimeDuration;
  return result;
}

nsRefreshDriver*
AnimationTimeline::GetRefreshDriver() const
{
  nsIPresShell* presShell = mDocument->GetShell();
  if (MOZ_UNLIKELY(!presShell)) {
    return nullptr;
  }

  nsPresContext* presContext = presShell->GetPresContext();
  if (MOZ_UNLIKELY(!presContext)) {
    return nullptr;
  }

  return presContext->RefreshDriver();
}

} 
} 
