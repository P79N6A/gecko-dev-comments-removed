





#include "WheelHandlingHelper.h"

#include "mozilla/EventDispatcher.h"
#include "mozilla/EventStateManager.h"
#include "mozilla/MouseEvents.h"
#include "mozilla/Preferences.h"
#include "nsCOMPtr.h"
#include "nsContentUtils.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsIScrollableFrame.h"
#include "nsITimer.h"
#include "nsPresContext.h"
#include "prtime.h"
#include "Units.h"

namespace mozilla {





DeltaValues::DeltaValues(WidgetWheelEvent* aEvent)
  : deltaX(aEvent->deltaX)
  , deltaY(aEvent->deltaY)
{
}





 bool
WheelHandlingUtils::CanScrollInRange(nscoord aMin, nscoord aValue, nscoord aMax,
                                     double aDirection)
{
  return aDirection > 0.0 ? aValue < static_cast<double>(aMax) :
                            static_cast<double>(aMin) < aValue;
}

 bool
WheelHandlingUtils::CanScrollOn(nsIScrollableFrame* aScrollFrame,
                                double aDirectionX, double aDirectionY)
{
  MOZ_ASSERT(aScrollFrame);
  NS_ASSERTION(aDirectionX || aDirectionY,
               "One of the delta values must be non-zero at least");

  nsPoint scrollPt = aScrollFrame->GetScrollPosition();
  nsRect scrollRange = aScrollFrame->GetScrollRange();
  uint32_t directions = aScrollFrame->GetPerceivedScrollingDirections();

  return (aDirectionX && (directions & nsIScrollableFrame::HORIZONTAL) &&
          CanScrollInRange(scrollRange.x, scrollPt.x,
                           scrollRange.XMost(), aDirectionX)) ||
         (aDirectionY && (directions & nsIScrollableFrame::VERTICAL) &&
          CanScrollInRange(scrollRange.y, scrollPt.y,
                           scrollRange.YMost(), aDirectionY));
}





nsWeakFrame WheelTransaction::sTargetFrame(nullptr);
uint32_t WheelTransaction::sTime = 0;
uint32_t WheelTransaction::sMouseMoved = 0;
nsITimer* WheelTransaction::sTimer = nullptr;
int32_t WheelTransaction::sScrollSeriesCounter = 0;
bool WheelTransaction::sOwnScrollbars = false;

 bool
WheelTransaction::OutOfTime(uint32_t aBaseTime, uint32_t aThreshold)
{
  uint32_t now = PR_IntervalToMilliseconds(PR_IntervalNow());
  return (now - aBaseTime > aThreshold);
}

 void
WheelTransaction::OwnScrollbars(bool aOwn)
{
  sOwnScrollbars = aOwn;
}

 void
WheelTransaction::BeginTransaction(nsIFrame* aTargetFrame,
                                   WidgetWheelEvent* aEvent)
{
  NS_ASSERTION(!sTargetFrame, "previous transaction is not finished!");
  MOZ_ASSERT(aEvent->message == NS_WHEEL_WHEEL,
             "Transaction must be started with a wheel event");
  ScrollbarsForWheel::OwnWheelTransaction(false);
  sTargetFrame = aTargetFrame;
  sScrollSeriesCounter = 0;
  if (!UpdateTransaction(aEvent)) {
    NS_ERROR("BeginTransaction is called even cannot scroll the frame");
    EndTransaction();
  }
}

 bool
WheelTransaction::UpdateTransaction(WidgetWheelEvent* aEvent)
{
  nsIScrollableFrame* sf = GetTargetFrame()->GetScrollTargetFrame();
  NS_ENSURE_TRUE(sf, false);

  if (!WheelHandlingUtils::CanScrollOn(sf, aEvent->deltaX, aEvent->deltaY)) {
    OnFailToScrollTarget();
    
    
    return false;
  }

  SetTimeout();

  if (sScrollSeriesCounter != 0 && OutOfTime(sTime, kScrollSeriesTimeout)) {
    sScrollSeriesCounter = 0;
  }
  sScrollSeriesCounter++;

  
  
  
  
  sTime = PR_IntervalToMilliseconds(PR_IntervalNow());
  sMouseMoved = 0;
  return true;
}

 void
WheelTransaction::MayEndTransaction()
{
  if (!sOwnScrollbars && ScrollbarsForWheel::IsActive()) {
    ScrollbarsForWheel::OwnWheelTransaction(true);
  } else {
    EndTransaction();
  }
}

 void
WheelTransaction::EndTransaction()
{
  if (sTimer) {
    sTimer->Cancel();
  }
  sTargetFrame = nullptr;
  sScrollSeriesCounter = 0;
  if (sOwnScrollbars) {
    sOwnScrollbars = false;
    ScrollbarsForWheel::OwnWheelTransaction(false);
    ScrollbarsForWheel::Inactivate();
  }
}

 void
WheelTransaction::OnEvent(WidgetEvent* aEvent)
{
  if (!sTargetFrame) {
    return;
  }

  if (OutOfTime(sTime, GetTimeoutTime())) {
    
    
    
    
    OnTimeout(nullptr, nullptr);
    return;
  }

  switch (aEvent->message) {
    case NS_WHEEL_WHEEL:
      if (sMouseMoved != 0 &&
          OutOfTime(sMouseMoved, GetIgnoreMoveDelayTime())) {
        
        
        EndTransaction();
      }
      return;
    case NS_MOUSE_MOVE:
    case NS_DRAGDROP_OVER: {
      WidgetMouseEvent* mouseEvent = aEvent->AsMouseEvent();
      if (mouseEvent->IsReal()) {
        
        
        nsIntPoint pt = GetScreenPoint(mouseEvent);
        nsIntRect r = sTargetFrame->GetScreenRectExternal();
        if (!r.Contains(pt)) {
          EndTransaction();
          return;
        }

        
        
        
        
        if (!sMouseMoved && OutOfTime(sTime, GetIgnoreMoveDelayTime())) {
          sMouseMoved = PR_IntervalToMilliseconds(PR_IntervalNow());
        }
      }
      return;
    }
    case NS_KEY_PRESS:
    case NS_KEY_UP:
    case NS_KEY_DOWN:
    case NS_MOUSE_BUTTON_UP:
    case NS_MOUSE_BUTTON_DOWN:
    case NS_MOUSE_DOUBLECLICK:
    case NS_MOUSE_CLICK:
    case NS_CONTEXTMENU:
    case NS_DRAGDROP_DROP:
      EndTransaction();
      return;
  }
}

 void
WheelTransaction::Shutdown()
{
  NS_IF_RELEASE(sTimer);
}

 void
WheelTransaction::OnFailToScrollTarget()
{
  NS_PRECONDITION(sTargetFrame, "We don't have mouse scrolling transaction");

  if (Preferences::GetBool("test.mousescroll", false)) {
    
    nsContentUtils::DispatchTrustedEvent(
                      sTargetFrame->GetContent()->OwnerDoc(),
                      sTargetFrame->GetContent(),
                      NS_LITERAL_STRING("MozMouseScrollFailed"),
                      true, true);
  }
  
  
  if (!sTargetFrame) {
    EndTransaction();
  }
}

 void
WheelTransaction::OnTimeout(nsITimer* aTimer, void* aClosure)
{
  if (!sTargetFrame) {
    
    EndTransaction();
    return;
  }
  
  nsIFrame* frame = sTargetFrame;
  
  
  MayEndTransaction();

  if (Preferences::GetBool("test.mousescroll", false)) {
    
    nsContentUtils::DispatchTrustedEvent(
                      frame->GetContent()->OwnerDoc(),
                      frame->GetContent(),
                      NS_LITERAL_STRING("MozMouseScrollTransactionTimeout"),
                      true, true);
  }
}

 void
WheelTransaction::SetTimeout()
{
  if (!sTimer) {
    nsCOMPtr<nsITimer> timer = do_CreateInstance(NS_TIMER_CONTRACTID);
    if (!timer) {
      return;
    }
    timer.swap(sTimer);
  }
  sTimer->Cancel();
  DebugOnly<nsresult> rv =
    sTimer->InitWithFuncCallback(OnTimeout, nullptr, GetTimeoutTime(),
                                 nsITimer::TYPE_ONE_SHOT);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "nsITimer::InitWithFuncCallback failed");
}

 nsIntPoint
WheelTransaction::GetScreenPoint(WidgetGUIEvent* aEvent)
{
  NS_ASSERTION(aEvent, "aEvent is null");
  NS_ASSERTION(aEvent->widget, "aEvent-widget is null");
  return LayoutDeviceIntPoint::ToUntyped(aEvent->refPoint) +
           aEvent->widget->WidgetToScreenOffset();
}

 uint32_t
WheelTransaction::GetTimeoutTime()
{
  return Preferences::GetUint("mousewheel.transaction.timeout", 1500);
}

 uint32_t
WheelTransaction::GetIgnoreMoveDelayTime()
{
  return Preferences::GetUint("mousewheel.transaction.ignoremovedelay", 100);
}

 DeltaValues
WheelTransaction::AccelerateWheelDelta(WidgetWheelEvent* aEvent,
                                       bool aAllowScrollSpeedOverride)
{
  DeltaValues result(aEvent);

  
  if (aEvent->deltaMode != nsIDOMWheelEvent::DOM_DELTA_LINE) {
    return result;
  }

  if (aAllowScrollSpeedOverride) {
    result = OverrideSystemScrollSpeed(aEvent);
  }

  
  int32_t start = GetAccelerationStart();
  if (start >= 0 && sScrollSeriesCounter >= start) {
    int32_t factor = GetAccelerationFactor();
    if (factor > 0) {
      result.deltaX = ComputeAcceleratedWheelDelta(result.deltaX, factor);
      result.deltaY = ComputeAcceleratedWheelDelta(result.deltaY, factor);
    }
  }

  return result;
}

 double
WheelTransaction::ComputeAcceleratedWheelDelta(double aDelta,
                                               int32_t aFactor)
{
  if (aDelta == 0.0) {
    return 0;
  }

  return (aDelta * sScrollSeriesCounter * (double)aFactor / 10);
}

 int32_t
WheelTransaction::GetAccelerationStart()
{
  return Preferences::GetInt("mousewheel.acceleration.start", -1);
}

 int32_t
WheelTransaction::GetAccelerationFactor()
{
  return Preferences::GetInt("mousewheel.acceleration.factor", -1);
}

 DeltaValues
WheelTransaction::OverrideSystemScrollSpeed(WidgetWheelEvent* aEvent)
{
  MOZ_ASSERT(sTargetFrame, "We don't have mouse scrolling transaction");
  MOZ_ASSERT(aEvent->deltaMode == nsIDOMWheelEvent::DOM_DELTA_LINE);

  
  
  if (!aEvent->deltaX && !aEvent->deltaY) {
    return DeltaValues(aEvent);
  }

  
  if (sTargetFrame !=
        sTargetFrame->PresContext()->PresShell()->GetRootScrollFrame()) {
    return DeltaValues(aEvent);
  }

  
  
  
  
  nsCOMPtr<nsIWidget> widget(sTargetFrame->GetNearestWidget());
  NS_ENSURE_TRUE(widget, DeltaValues(aEvent));
  DeltaValues overriddenDeltaValues(0.0, 0.0);
  nsresult rv =
    widget->OverrideSystemMouseScrollSpeed(aEvent->deltaX, aEvent->deltaY,
                                           overriddenDeltaValues.deltaX,
                                           overriddenDeltaValues.deltaY);
  return NS_FAILED(rv) ? DeltaValues(aEvent) : overriddenDeltaValues;
}





const DeltaValues ScrollbarsForWheel::directions[kNumberOfTargets] = {
  DeltaValues(-1, 0), DeltaValues(+1, 0), DeltaValues(0, -1), DeltaValues(0, +1)
};

nsWeakFrame ScrollbarsForWheel::sActiveOwner = nullptr;
nsWeakFrame ScrollbarsForWheel::sActivatedScrollTargets[kNumberOfTargets] = {
  nullptr, nullptr, nullptr, nullptr
};

bool ScrollbarsForWheel::sHadWheelStart = false;
bool ScrollbarsForWheel::sOwnWheelTransaction = false;

 void
ScrollbarsForWheel::PrepareToScrollText(EventStateManager* aESM,
                                        nsIFrame* aTargetFrame,
                                        WidgetWheelEvent* aEvent)
{
  if (aEvent->message == NS_WHEEL_START) {
    WheelTransaction::OwnScrollbars(false);
    if (!IsActive()) {
      TemporarilyActivateAllPossibleScrollTargets(aESM, aTargetFrame, aEvent);
      sHadWheelStart = true;
    }
  } else {
    DeactivateAllTemporarilyActivatedScrollTargets();
  }
}

 void
ScrollbarsForWheel::SetActiveScrollTarget(nsIScrollableFrame* aScrollTarget)
{
  if (!sHadWheelStart) {
    return;
  }
  nsIScrollbarMediator* scrollbarMediator = do_QueryFrame(aScrollTarget);
  if (!scrollbarMediator) {
    return;
  }
  sHadWheelStart = false;
  sActiveOwner = do_QueryFrame(aScrollTarget);
  scrollbarMediator->ScrollbarActivityStarted();
}

 void
ScrollbarsForWheel::MayInactivate()
{
  if (!sOwnWheelTransaction && WheelTransaction::GetTargetFrame()) {
    WheelTransaction::OwnScrollbars(true);
  } else {
    Inactivate();
  }
}

 void
ScrollbarsForWheel::Inactivate()
{
  nsIScrollbarMediator* scrollbarMediator = do_QueryFrame(sActiveOwner);
  if (scrollbarMediator) {
    scrollbarMediator->ScrollbarActivityStopped();
  }
  sActiveOwner = nullptr;
  DeactivateAllTemporarilyActivatedScrollTargets();
  if (sOwnWheelTransaction) {
    sOwnWheelTransaction = false;
    WheelTransaction::OwnScrollbars(false);
    WheelTransaction::EndTransaction();
  }
}

 bool
ScrollbarsForWheel::IsActive()
{
  if (sActiveOwner) {
    return true;
  }
  for (size_t i = 0; i < kNumberOfTargets; ++i) {
    if (sActivatedScrollTargets[i]) {
      return true;
    }
  }
  return false;
}

 void
ScrollbarsForWheel::OwnWheelTransaction(bool aOwn)
{
  sOwnWheelTransaction = aOwn;
}

 void
ScrollbarsForWheel::TemporarilyActivateAllPossibleScrollTargets(
                      EventStateManager* aESM,
                      nsIFrame* aTargetFrame,
                      WidgetWheelEvent* aEvent)
{
  for (size_t i = 0; i < kNumberOfTargets; i++) {
    const DeltaValues *dir = &directions[i];
    nsWeakFrame* scrollTarget = &sActivatedScrollTargets[i];
    MOZ_ASSERT(!*scrollTarget, "scroll target still temporarily activated!");
    nsIScrollableFrame* target =
      aESM->ComputeScrollTarget(aTargetFrame, dir->deltaX, dir->deltaY, aEvent,
              EventStateManager::COMPUTE_DEFAULT_ACTION_TARGET);
    nsIScrollbarMediator* scrollbarMediator = do_QueryFrame(target);
    if (scrollbarMediator) {
      nsIFrame* targetFrame = do_QueryFrame(target);
      *scrollTarget = targetFrame;
      scrollbarMediator->ScrollbarActivityStarted();
    }
  }
}

 void
ScrollbarsForWheel::DeactivateAllTemporarilyActivatedScrollTargets()
{
  for (size_t i = 0; i < kNumberOfTargets; i++) {
    nsWeakFrame* scrollTarget = &sActivatedScrollTargets[i];
    if (*scrollTarget) {
      nsIScrollbarMediator* scrollbarMediator = do_QueryFrame(*scrollTarget);
      if (scrollbarMediator) {
        scrollbarMediator->ScrollbarActivityStopped();
      }
      *scrollTarget = nullptr;
    }
  }
}

} 
