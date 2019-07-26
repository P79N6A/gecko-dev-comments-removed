





#include "WheelHandlingHelper.h"

#include "mozilla/EventDispatcher.h"
#include "mozilla/MouseEvents.h"
#include "mozilla/Preferences.h"
#include "nsCOMPtr.h"
#include "nsContentUtils.h"
#include "nsEventStateManager.h"
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

} 

using namespace mozilla;





nsWeakFrame nsMouseWheelTransaction::sTargetFrame(nullptr);
uint32_t nsMouseWheelTransaction::sTime = 0;
uint32_t nsMouseWheelTransaction::sMouseMoved = 0;
nsITimer* nsMouseWheelTransaction::sTimer = nullptr;
int32_t nsMouseWheelTransaction::sScrollSeriesCounter = 0;
bool nsMouseWheelTransaction::sOwnScrollbars = false;

bool
nsMouseWheelTransaction::OutOfTime(uint32_t aBaseTime, uint32_t aThreshold)
{
  uint32_t now = PR_IntervalToMilliseconds(PR_IntervalNow());
  return (now - aBaseTime > aThreshold);
}

void
nsMouseWheelTransaction::OwnScrollbars(bool aOwn)
{
  sOwnScrollbars = aOwn;
}

void
nsMouseWheelTransaction::BeginTransaction(nsIFrame* aTargetFrame,
                                          WidgetWheelEvent* aEvent)
{
  NS_ASSERTION(!sTargetFrame, "previous transaction is not finished!");
  MOZ_ASSERT(aEvent->message == NS_WHEEL_WHEEL,
             "Transaction must be started with a wheel event");
  nsScrollbarsForWheel::OwnWheelTransaction(false);
  sTargetFrame = aTargetFrame;
  sScrollSeriesCounter = 0;
  if (!UpdateTransaction(aEvent)) {
    NS_ERROR("BeginTransaction is called even cannot scroll the frame");
    EndTransaction();
  }
}

bool
nsMouseWheelTransaction::UpdateTransaction(WidgetWheelEvent* aEvent)
{
  nsIScrollableFrame* sf = GetTargetFrame()->GetScrollTargetFrame();
  NS_ENSURE_TRUE(sf, false);

  if (!WheelHandlingUtils::CanScrollOn(sf, aEvent->deltaX, aEvent->deltaY)) {
    OnFailToScrollTarget();
    
    
    return false;
  }

  SetTimeout();

  if (sScrollSeriesCounter != 0 && OutOfTime(sTime, kScrollSeriesTimeout))
    sScrollSeriesCounter = 0;
  sScrollSeriesCounter++;

  
  
  
  
  sTime = PR_IntervalToMilliseconds(PR_IntervalNow());
  sMouseMoved = 0;
  return true;
}

void
nsMouseWheelTransaction::MayEndTransaction()
{
  if (!sOwnScrollbars && nsScrollbarsForWheel::IsActive()) {
    nsScrollbarsForWheel::OwnWheelTransaction(true);
  } else {
    EndTransaction();
  }
}

void
nsMouseWheelTransaction::EndTransaction()
{
  if (sTimer)
    sTimer->Cancel();
  sTargetFrame = nullptr;
  sScrollSeriesCounter = 0;
  if (sOwnScrollbars) {
    sOwnScrollbars = false;
    nsScrollbarsForWheel::OwnWheelTransaction(false);
    nsScrollbarsForWheel::Inactivate();
  }
}

void
nsMouseWheelTransaction::OnEvent(WidgetEvent* aEvent)
{
  if (!sTargetFrame)
    return;

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

        
        
        
        
        if (OutOfTime(sTime, GetIgnoreMoveDelayTime())) {
          if (sMouseMoved == 0)
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
nsMouseWheelTransaction::Shutdown()
{
  NS_IF_RELEASE(sTimer);
}

void
nsMouseWheelTransaction::OnFailToScrollTarget()
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
nsMouseWheelTransaction::OnTimeout(nsITimer* aTimer, void* aClosure)
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
nsMouseWheelTransaction::SetTimeout()
{
  if (!sTimer) {
    nsCOMPtr<nsITimer> timer = do_CreateInstance(NS_TIMER_CONTRACTID);
    if (!timer)
      return;
    timer.swap(sTimer);
  }
  sTimer->Cancel();
#ifdef DEBUG
  nsresult rv =
#endif
  sTimer->InitWithFuncCallback(OnTimeout, nullptr, GetTimeoutTime(),
                               nsITimer::TYPE_ONE_SHOT);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "nsITimer::InitWithFuncCallback failed");
}

nsIntPoint
nsMouseWheelTransaction::GetScreenPoint(WidgetGUIEvent* aEvent)
{
  NS_ASSERTION(aEvent, "aEvent is null");
  NS_ASSERTION(aEvent->widget, "aEvent-widget is null");
  return LayoutDeviceIntPoint::ToUntyped(aEvent->refPoint) +
         aEvent->widget->WidgetToScreenOffset();
}

uint32_t
nsMouseWheelTransaction::GetTimeoutTime()
{
  return Preferences::GetUint("mousewheel.transaction.timeout", 1500);
}

uint32_t
nsMouseWheelTransaction::GetIgnoreMoveDelayTime()
{
  return Preferences::GetUint("mousewheel.transaction.ignoremovedelay", 100);
}

DeltaValues
nsMouseWheelTransaction::AccelerateWheelDelta(WidgetWheelEvent* aEvent,
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
nsMouseWheelTransaction::ComputeAcceleratedWheelDelta(double aDelta,
                                                      int32_t aFactor)
{
  if (aDelta == 0.0) {
    return 0;
  }

  return (aDelta * sScrollSeriesCounter * (double)aFactor / 10);
}

int32_t
nsMouseWheelTransaction::GetAccelerationStart()
{
  return Preferences::GetInt("mousewheel.acceleration.start", -1);
}

int32_t
nsMouseWheelTransaction::GetAccelerationFactor()
{
  return Preferences::GetInt("mousewheel.acceleration.factor", -1);
}

DeltaValues
nsMouseWheelTransaction::OverrideSystemScrollSpeed(WidgetWheelEvent* aEvent)
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





const DeltaValues nsScrollbarsForWheel::directions[kNumberOfTargets] = {
  DeltaValues(-1, 0), DeltaValues(+1, 0), DeltaValues(0, -1), DeltaValues(0, +1)
};

nsWeakFrame nsScrollbarsForWheel::sActiveOwner = nullptr;
nsWeakFrame nsScrollbarsForWheel::sActivatedScrollTargets[kNumberOfTargets] = {
  nullptr, nullptr, nullptr, nullptr
};

bool nsScrollbarsForWheel::sHadWheelStart = false;
bool nsScrollbarsForWheel::sOwnWheelTransaction = false;

void
nsScrollbarsForWheel::PrepareToScrollText(nsEventStateManager* aESM,
                                          nsIFrame* aTargetFrame,
                                          WidgetWheelEvent* aEvent)
{
  if (aEvent->message == NS_WHEEL_START) {
    nsMouseWheelTransaction::OwnScrollbars(false);
    if (!IsActive()) {
      TemporarilyActivateAllPossibleScrollTargets(aESM, aTargetFrame, aEvent);
      sHadWheelStart = true;
    }
  } else {
    DeactivateAllTemporarilyActivatedScrollTargets();
  }
}

void
nsScrollbarsForWheel::SetActiveScrollTarget(nsIScrollableFrame* aScrollTarget)
{
  if (!sHadWheelStart) {
    return;
  }
  nsIScrollbarOwner* scrollbarOwner = do_QueryFrame(aScrollTarget);
  if (!scrollbarOwner) {
    return;
  }
  sHadWheelStart = false;
  sActiveOwner = do_QueryFrame(aScrollTarget);
  scrollbarOwner->ScrollbarActivityStarted();
}

void
nsScrollbarsForWheel::MayInactivate()
{
  if (!sOwnWheelTransaction && nsMouseWheelTransaction::GetTargetFrame()) {
    nsMouseWheelTransaction::OwnScrollbars(true);
  } else {
    Inactivate();
  }
}

void
nsScrollbarsForWheel::Inactivate()
{
  nsIScrollbarOwner* scrollbarOwner = do_QueryFrame(sActiveOwner);
  if (scrollbarOwner) {
    scrollbarOwner->ScrollbarActivityStopped();
  }
  sActiveOwner = nullptr;
  DeactivateAllTemporarilyActivatedScrollTargets();
  if (sOwnWheelTransaction) {
    sOwnWheelTransaction = false;
    nsMouseWheelTransaction::OwnScrollbars(false);
    nsMouseWheelTransaction::EndTransaction();
  }
}

bool
nsScrollbarsForWheel::IsActive()
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
nsScrollbarsForWheel::OwnWheelTransaction(bool aOwn)
{
  sOwnWheelTransaction = aOwn;
}

void
nsScrollbarsForWheel::TemporarilyActivateAllPossibleScrollTargets(
                        nsEventStateManager* aESM,
                        nsIFrame* aTargetFrame,
                        WidgetWheelEvent* aEvent)
{
  for (size_t i = 0; i < kNumberOfTargets; i++) {
    const DeltaValues *dir = &directions[i];
    nsWeakFrame* scrollTarget = &sActivatedScrollTargets[i];
    MOZ_ASSERT(!*scrollTarget, "scroll target still temporarily activated!");
    nsIScrollableFrame* target =
      aESM->ComputeScrollTarget(aTargetFrame, dir->deltaX, dir->deltaY, aEvent, 
                                nsEventStateManager::COMPUTE_DEFAULT_ACTION_TARGET);
    nsIScrollbarOwner* scrollbarOwner = do_QueryFrame(target);
    if (scrollbarOwner) {
      nsIFrame* targetFrame = do_QueryFrame(target);
      *scrollTarget = targetFrame;
      scrollbarOwner->ScrollbarActivityStarted();
    }
  }
}

void
nsScrollbarsForWheel::DeactivateAllTemporarilyActivatedScrollTargets()
{
  for (size_t i = 0; i < kNumberOfTargets; i++) {
    nsWeakFrame* scrollTarget = &sActivatedScrollTargets[i];
    if (*scrollTarget) {
      nsIScrollbarOwner* scrollbarOwner = do_QueryFrame(*scrollTarget);
      if (scrollbarOwner) {
        scrollbarOwner->ScrollbarActivityStopped();
      }
      *scrollTarget = nullptr;
    }
  }
}
