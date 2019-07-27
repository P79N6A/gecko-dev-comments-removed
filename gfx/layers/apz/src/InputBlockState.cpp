





#include "InputBlockState.h"
#include "AsyncPanZoomController.h"         
#include "gfxPrefs.h"                       
#include "mozilla/SizePrintfMacros.h"       
#include "mozilla/layers/APZCTreeManager.h" 
#include "OverscrollHandoffState.h"

#define TBS_LOG(...)


namespace mozilla {
namespace layers {

static uint64_t sBlockCounter = InputBlockState::NO_BLOCK_ID + 1;

InputBlockState::InputBlockState(const nsRefPtr<AsyncPanZoomController>& aTargetApzc,
                                 bool aTargetConfirmed)
  : mTargetApzc(aTargetApzc)
  , mTargetConfirmed(aTargetConfirmed)
  , mBlockId(sBlockCounter++)
  , mTransformToApzc(aTargetApzc->GetTransformToThis())
{
  
  MOZ_ASSERT(mTargetApzc);
  mOverscrollHandoffChain = mTargetApzc->BuildOverscrollHandoffChain();
}

bool
InputBlockState::SetConfirmedTargetApzc(const nsRefPtr<AsyncPanZoomController>& aTargetApzc)
{
  if (mTargetConfirmed) {
    return false;
  }
  mTargetConfirmed = true;

  TBS_LOG("%p got confirmed target APZC %p\n", this, mTargetApzc.get());
  if (mTargetApzc == aTargetApzc) {
    
    return true;
  }

  
  
  printf_stderr("%p replacing unconfirmed target %p with real target %p\n",
      this, mTargetApzc.get(), aTargetApzc.get());

  UpdateTargetApzc(aTargetApzc);
  return true;
}

void
InputBlockState::UpdateTargetApzc(const nsRefPtr<AsyncPanZoomController>& aTargetApzc)
{
  
  mTargetApzc = aTargetApzc;
  mTransformToApzc = aTargetApzc ? aTargetApzc->GetTransformToThis() : gfx::Matrix4x4();
  mOverscrollHandoffChain = (mTargetApzc ? mTargetApzc->BuildOverscrollHandoffChain() : nullptr);
}

const nsRefPtr<AsyncPanZoomController>&
InputBlockState::GetTargetApzc() const
{
  return mTargetApzc;
}

const nsRefPtr<const OverscrollHandoffChain>&
InputBlockState::GetOverscrollHandoffChain() const
{
  return mOverscrollHandoffChain;
}

uint64_t
InputBlockState::GetBlockId() const
{
  return mBlockId;
}

bool
InputBlockState::IsTargetConfirmed() const
{
  return mTargetConfirmed;
}

CancelableBlockState::CancelableBlockState(const nsRefPtr<AsyncPanZoomController>& aTargetApzc,
                                           bool aTargetConfirmed)
  : InputBlockState(aTargetApzc, aTargetConfirmed)
  , mPreventDefault(false)
  , mContentResponded(false)
  , mContentResponseTimerExpired(false)
{
}

bool
CancelableBlockState::SetContentResponse(bool aPreventDefault)
{
  if (mContentResponded) {
    return false;
  }
  TBS_LOG("%p got content response %d with timer expired %d\n",
    this, aPreventDefault, mContentResponseTimerExpired);
  if (!mContentResponseTimerExpired) {
    mPreventDefault = aPreventDefault;
  }
  mContentResponded = true;
  return true;
}

bool
CancelableBlockState::TimeoutContentResponse()
{
  if (mContentResponseTimerExpired) {
    return false;
  }
  TBS_LOG("%p got content timer expired with response received %d\n",
    this, mContentResponded);
  if (!mContentResponded) {
    mPreventDefault = false;
  }
  mContentResponseTimerExpired = true;
  return true;
}

bool
CancelableBlockState::IsDefaultPrevented() const
{
  MOZ_ASSERT(mContentResponded || mContentResponseTimerExpired);
  return mPreventDefault;
}

bool
CancelableBlockState::IsReadyForHandling() const
{
  if (!IsTargetConfirmed())
    return false;
  return mContentResponded || mContentResponseTimerExpired;
}

void
CancelableBlockState::DispatchImmediate(const InputData& aEvent) const
{
  MOZ_ASSERT(!HasEvents());
  MOZ_ASSERT(GetTargetApzc());
  GetTargetApzc()->HandleInputEvent(aEvent, mTransformToApzc);
}


static uint64_t sLastWheelBlockId = InputBlockState::NO_BLOCK_ID;

WheelBlockState::WheelBlockState(const nsRefPtr<AsyncPanZoomController>& aTargetApzc,
                                 bool aTargetConfirmed,
                                 const ScrollWheelInput& aInitialEvent)
  : CancelableBlockState(aTargetApzc, aTargetConfirmed)
  , mTransactionEnded(false)
{
  sLastWheelBlockId = GetBlockId();

  if (aTargetConfirmed) {
    
    
    
    
    nsRefPtr<AsyncPanZoomController> apzc =
      mOverscrollHandoffChain->FindFirstScrollable(aInitialEvent);

    
    
    if (!apzc) {
      EndTransaction();
      return;
    }

    if (apzc != GetTargetApzc()) {
      UpdateTargetApzc(apzc);
    }
  }
}

bool
WheelBlockState::SetConfirmedTargetApzc(const nsRefPtr<AsyncPanZoomController>& aTargetApzc)
{
  
  
  
  nsRefPtr<AsyncPanZoomController> apzc = aTargetApzc;
  if (apzc && mEvents.Length() > 0) {
    const ScrollWheelInput& event = mEvents.ElementAt(0);
    apzc = apzc->BuildOverscrollHandoffChain()->FindFirstScrollable(event);
  }

  InputBlockState::SetConfirmedTargetApzc(apzc);
  return true;
}

void
WheelBlockState::Update(const ScrollWheelInput& aEvent)
{
  
  
  if (!InTransaction()) {
    return;
  }

  
  
  
  
  
  
  nsRefPtr<AsyncPanZoomController> apzc = GetTargetApzc();
  if (IsTargetConfirmed() && !apzc->CanScroll(aEvent)) {
    return;
  }

  
  
  
  mLastEventTime = aEvent.mTimeStamp;
  mLastMouseMove = TimeStamp();
}

void
WheelBlockState::AddEvent(const ScrollWheelInput& aEvent)
{
  mEvents.AppendElement(aEvent);
}

bool
WheelBlockState::IsReadyForHandling() const
{
  if (!CancelableBlockState::IsReadyForHandling()) {
    return false;
  }
  return true;
}

bool
WheelBlockState::HasEvents() const
{
  return !mEvents.IsEmpty();
}

void
WheelBlockState::DropEvents()
{
  TBS_LOG("%p dropping %" PRIuSIZE " events\n", this, mEvents.Length());
  mEvents.Clear();
}

void
WheelBlockState::HandleEvents()
{
  while (HasEvents()) {
    TBS_LOG("%p returning first of %" PRIuSIZE " events\n", this, mEvents.Length());
    ScrollWheelInput event = mEvents[0];
    mEvents.RemoveElementAt(0);
    GetTargetApzc()->HandleInputEvent(event, mTransformToApzc);
  }
}

bool
WheelBlockState::MustStayActive()
{
  return !mTransactionEnded;
}

const char*
WheelBlockState::Type()
{
  return "scroll wheel";
}

bool
WheelBlockState::ShouldAcceptNewEvent() const
{
  if (!InTransaction()) {
    
    return false;
  }

  nsRefPtr<AsyncPanZoomController> apzc = GetTargetApzc();
  if (apzc->IsDestroyed()) {
    return false;
  }

  return true;
}

bool
WheelBlockState::MaybeTimeout(const ScrollWheelInput& aEvent)
{
  MOZ_ASSERT(InTransaction());

  if (MaybeTimeout(aEvent.mTimeStamp)) {
    return true;
  }

  if (!mLastMouseMove.IsNull()) {
    
    TimeDuration duration = TimeStamp::Now() - mLastMouseMove;
    if (duration.ToMilliseconds() >= gfxPrefs::MouseWheelIgnoreMoveDelayMs()) {
      TBS_LOG("%p wheel transaction timed out after mouse move\n", this);
      EndTransaction();
      return true;
    }
  }

  return false;
}

bool
WheelBlockState::MaybeTimeout(const TimeStamp& aTimeStamp)
{
  MOZ_ASSERT(InTransaction());

  
  
  TimeDuration duration = aTimeStamp - mLastEventTime;
  if (duration.ToMilliseconds() < gfxPrefs::MouseWheelTransactionTimeoutMs()) {
    return false;
  }

  TBS_LOG("%p wheel transaction timed out\n", this);

  if (gfxPrefs::MouseScrollTestingEnabled()) {
    nsRefPtr<AsyncPanZoomController> apzc = GetTargetApzc();
    apzc->NotifyMozMouseScrollEvent(NS_LITERAL_STRING("MozMouseScrollTransactionTimeout"));
  }

  EndTransaction();
  return true;
}

void
WheelBlockState::OnMouseMove(const ScreenIntPoint& aPoint)
{
  MOZ_ASSERT(InTransaction());

  if (!GetTargetApzc()->Contains(aPoint)) {
    EndTransaction();
    return;
  }

  if (mLastMouseMove.IsNull()) {
    
    
    
    TimeStamp now = TimeStamp::Now();
    TimeDuration duration = now - mLastEventTime;
    if (duration.ToMilliseconds() >= gfxPrefs::MouseWheelIgnoreMoveDelayMs()) {
      mLastMouseMove = now;
    }
  }
}

void
WheelBlockState::UpdateTargetApzc(const nsRefPtr<AsyncPanZoomController>& aTargetApzc)
{
  InputBlockState::UpdateTargetApzc(aTargetApzc);

  
  if (!GetTargetApzc()) {
    EndTransaction();
  }
}

bool
WheelBlockState::InTransaction() const
{
  
  
  if (GetBlockId() != sLastWheelBlockId) {
    return false;
  }

  if (mTransactionEnded) {
    return false;
  }

  MOZ_ASSERT(GetTargetApzc());
  return true;
}

bool
WheelBlockState::AllowScrollHandoff() const
{
  
  
  return !IsTargetConfirmed() || !InTransaction();
}

void
WheelBlockState::EndTransaction()
{
  TBS_LOG("%p ending wheel transaction\n", this);
  mTransactionEnded = true;
}

TouchBlockState::TouchBlockState(const nsRefPtr<AsyncPanZoomController>& aTargetApzc,
                                 bool aTargetConfirmed)
  : CancelableBlockState(aTargetApzc, aTargetConfirmed)
  , mAllowedTouchBehaviorSet(false)
  , mDuringFastMotion(false)
  , mSingleTapOccurred(false)
{
  TBS_LOG("Creating %p\n", this);
}

bool
TouchBlockState::SetAllowedTouchBehaviors(const nsTArray<TouchBehaviorFlags>& aBehaviors)
{
  if (mAllowedTouchBehaviorSet) {
    return false;
  }
  TBS_LOG("%p got allowed touch behaviours for %" PRIuSIZE " points\n", this, aBehaviors.Length());
  mAllowedTouchBehaviors.AppendElements(aBehaviors);
  mAllowedTouchBehaviorSet = true;
  return true;
}

bool
TouchBlockState::GetAllowedTouchBehaviors(nsTArray<TouchBehaviorFlags>& aOutBehaviors) const
{
  if (!mAllowedTouchBehaviorSet) {
    return false;
  }
  aOutBehaviors.AppendElements(mAllowedTouchBehaviors);
  return true;
}

void
TouchBlockState::CopyPropertiesFrom(const TouchBlockState& aOther)
{
  TBS_LOG("%p copying properties from %p\n", this, &aOther);
  if (gfxPrefs::TouchActionEnabled()) {
    MOZ_ASSERT(aOther.mAllowedTouchBehaviorSet);
    SetAllowedTouchBehaviors(aOther.mAllowedTouchBehaviors);
  }
  mTransformToApzc = aOther.mTransformToApzc;
}

bool
TouchBlockState::IsReadyForHandling() const
{
  if (!CancelableBlockState::IsReadyForHandling()) {
    return false;
  }

  
  if (gfxPrefs::TouchActionEnabled() && !mAllowedTouchBehaviorSet) {
    return false;
  }
  return true;
}

void
TouchBlockState::SetDuringFastMotion()
{
  TBS_LOG("%p setting fast-motion flag\n", this);
  mDuringFastMotion = true;
}

bool
TouchBlockState::IsDuringFastMotion() const
{
  return mDuringFastMotion;
}

bool
TouchBlockState::SetSingleTapOccurred()
{
  TBS_LOG("%p attempting to set single-tap occurred; disallowed=%d\n",
    this, mDuringFastMotion);
  if (!mDuringFastMotion) {
    mSingleTapOccurred = true;
    return true;
  }
  return false;
}

bool
TouchBlockState::SingleTapOccurred() const
{
  return mSingleTapOccurred;
}

bool
TouchBlockState::HasEvents() const
{
  return !mEvents.IsEmpty();
}

void
TouchBlockState::AddEvent(const MultiTouchInput& aEvent)
{
  TBS_LOG("%p adding event of type %d\n", this, aEvent.mType);
  mEvents.AppendElement(aEvent);
}

bool
TouchBlockState::MustStayActive()
{
  return true;
}

const char*
TouchBlockState::Type()
{
  return "touch";
}

void
TouchBlockState::DropEvents()
{
  TBS_LOG("%p dropping %" PRIuSIZE " events\n", this, mEvents.Length());
  mEvents.Clear();
}

void
TouchBlockState::HandleEvents()
{
  while (HasEvents()) {
    TBS_LOG("%p returning first of %" PRIuSIZE " events\n", this, mEvents.Length());
    MultiTouchInput event = mEvents[0];
    mEvents.RemoveElementAt(0);
    GetTargetApzc()->HandleInputEvent(event, mTransformToApzc);
  }
}

bool
TouchBlockState::TouchActionAllowsPinchZoom() const
{
  if (!gfxPrefs::TouchActionEnabled()) {
    return true;
  }
  
  for (size_t i = 0; i < mAllowedTouchBehaviors.Length(); i++) {
    if (!(mAllowedTouchBehaviors[i] & AllowedTouchBehavior::PINCH_ZOOM)) {
      return false;
    }
  }
  return true;
}

bool
TouchBlockState::TouchActionAllowsDoubleTapZoom() const
{
  if (!gfxPrefs::TouchActionEnabled()) {
    return true;
  }
  for (size_t i = 0; i < mAllowedTouchBehaviors.Length(); i++) {
    if (!(mAllowedTouchBehaviors[i] & AllowedTouchBehavior::DOUBLE_TAP_ZOOM)) {
      return false;
    }
  }
  return true;
}

bool
TouchBlockState::TouchActionAllowsPanningX() const
{
  if (!gfxPrefs::TouchActionEnabled()) {
    return true;
  }
  if (mAllowedTouchBehaviors.IsEmpty()) {
    
    return true;
  }
  TouchBehaviorFlags flags = mAllowedTouchBehaviors[0];
  return (flags & AllowedTouchBehavior::HORIZONTAL_PAN);
}

bool
TouchBlockState::TouchActionAllowsPanningY() const
{
  if (!gfxPrefs::TouchActionEnabled()) {
    return true;
  }
  if (mAllowedTouchBehaviors.IsEmpty()) {
    
    return true;
  }
  TouchBehaviorFlags flags = mAllowedTouchBehaviors[0];
  return (flags & AllowedTouchBehavior::VERTICAL_PAN);
}

bool
TouchBlockState::TouchActionAllowsPanningXY() const
{
  if (!gfxPrefs::TouchActionEnabled()) {
    return true;
  }
  if (mAllowedTouchBehaviors.IsEmpty()) {
    
    return true;
  }
  TouchBehaviorFlags flags = mAllowedTouchBehaviors[0];
  return (flags & AllowedTouchBehavior::HORIZONTAL_PAN)
      && (flags & AllowedTouchBehavior::VERTICAL_PAN);
}

} 
} 
