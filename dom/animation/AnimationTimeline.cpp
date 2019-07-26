




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
  Nullable<double> result; 

  nsIPresShell* presShell = mDocument->GetShell();
  if (!presShell)
    return result;

  nsPresContext* presContext = presShell->GetPresContext();
  if (!presContext)
    return result;

  nsRefPtr<nsDOMNavigationTiming> timing = mDocument->GetNavigationTiming();
  if (!timing)
    return result;

  TimeStamp now = presContext->RefreshDriver()->MostRecentRefresh();
  result.SetValue(timing->TimeStampToDOMHighRes(now));

  return result;
}

} 
} 
