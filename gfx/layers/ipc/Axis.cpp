





#include "Axis.h"
#include "AsyncPanZoomController.h"
#include "mozilla/Preferences.h"
#include "nsThreadUtils.h"
#include <algorithm>

namespace mozilla {
namespace layers {

static const float EPSILON = 0.0001f;







static float gMaxEventAcceleration = 999.0f;




static float gFlingFriction = 0.006f;





static float gVelocityThreshold = 0.14f;









static float gAccelerationMultiplier = 1.125f;






static float gFlingStoppedThreshold = 0.01f;






static int gMaxVelocityQueueSize = 5;

static void ReadAxisPrefs()
{
  Preferences::AddFloatVarCache(&gMaxEventAcceleration, "gfx.axis.max_event_acceleration", gMaxEventAcceleration);
  Preferences::AddFloatVarCache(&gFlingFriction, "gfx.axis.fling_friction", gFlingFriction);
  Preferences::AddFloatVarCache(&gVelocityThreshold, "gfx.axis.velocity_threshold", gVelocityThreshold);
  Preferences::AddFloatVarCache(&gAccelerationMultiplier, "gfx.axis.acceleration_multiplier", gAccelerationMultiplier);
  Preferences::AddFloatVarCache(&gFlingStoppedThreshold, "gfx.axis.fling_stopped_threshold", gFlingStoppedThreshold);
  Preferences::AddIntVarCache(&gMaxVelocityQueueSize, "gfx.axis.max_velocity_queue_size", gMaxVelocityQueueSize);
}

class ReadAxisPref MOZ_FINAL : public nsRunnable {
public:
  NS_IMETHOD Run()
  {
    ReadAxisPrefs();
    return NS_OK;
  }
};

static void InitAxisPrefs()
{
  static bool sInitialized = false;
  if (sInitialized)
    return;

  sInitialized = true;
  if (NS_IsMainThread()) {
    ReadAxisPrefs();
  } else {
    
    NS_DispatchToMainThread(new ReadAxisPref());
  }
}

Axis::Axis(AsyncPanZoomController* aAsyncPanZoomController)
  : mPos(0.0f),
    mVelocity(0.0f),
    mAcceleration(0),
    mAsyncPanZoomController(aAsyncPanZoomController)
{
  InitAxisPrefs();
}

void Axis::UpdateWithTouchAtDevicePoint(int32_t aPos, const TimeDuration& aTimeDelta) {
  if (mPos == aPos) {
    
    return;
  }

  float newVelocity = (mPos - aPos) / aTimeDelta.ToMilliseconds();

  bool curVelocityBelowThreshold = fabsf(newVelocity) < gVelocityThreshold;
  bool directionChange = (mVelocity > 0) != (newVelocity > 0);

  
  
  if (directionChange || curVelocityBelowThreshold) {
    mAcceleration = 0;
  }

  mVelocity = newVelocity;
  mPos = aPos;

  
  mVelocityQueue.AppendElement(mVelocity);
  if (mVelocityQueue.Length() > gMaxVelocityQueueSize) {
    mVelocityQueue.RemoveElementAt(0);
  }
}

void Axis::StartTouch(int32_t aPos) {
  mStartPos = aPos;
  mPos = aPos;
}

float Axis::GetDisplacementForDuration(float aScale, const TimeDuration& aDelta) {
  if (fabsf(mVelocity) < gVelocityThreshold) {
    mAcceleration = 0;
  }

  float accelerationFactor = GetAccelerationFactor();
  float displacement = mVelocity * aScale * aDelta.ToMilliseconds() * accelerationFactor;
  
  
  if (DisplacementWillOverscroll(displacement) != OVERSCROLL_NONE) {
    
    
    mVelocity = 0.0f;
    mAcceleration = 0;
    displacement -= DisplacementWillOverscrollAmount(displacement);
  }
  return displacement;
}

float Axis::PanDistance() {
  return fabsf(mPos - mStartPos);
}

void Axis::EndTouch() {
  mAcceleration++;

  
  int count = mVelocityQueue.Length();
  if (count) {
    mVelocity = 0;
    while (!mVelocityQueue.IsEmpty()) {
      mVelocity += mVelocityQueue[0];
      mVelocityQueue.RemoveElementAt(0);
    }
    mVelocity /= count;
  }
}

void Axis::CancelTouch() {
  mVelocity = 0.0f;
  mAcceleration = 0;
  while (!mVelocityQueue.IsEmpty()) {
    mVelocityQueue.RemoveElementAt(0);
  }
}

bool Axis::FlingApplyFrictionOrCancel(const TimeDuration& aDelta) {
  if (fabsf(mVelocity) <= gFlingStoppedThreshold) {
    
    
    
    mVelocity = 0.0f;
    return false;
  } else {
    mVelocity *= pow(1.0f - gFlingFriction, float(aDelta.ToMilliseconds()));
  }
  return true;
}

Axis::Overscroll Axis::GetOverscroll() {
  
  
  bool minus = GetOrigin() < GetPageStart();
  
  
  bool plus = GetCompositionEnd() > GetPageEnd();
  if (minus && plus) {
    return OVERSCROLL_BOTH;
  }
  if (minus) {
    return OVERSCROLL_MINUS;
  }
  if (plus) {
    return OVERSCROLL_PLUS;
  }
  return OVERSCROLL_NONE;
}

float Axis::GetExcess() {
  switch (GetOverscroll()) {
  case OVERSCROLL_MINUS: return GetOrigin() - GetPageStart();
  case OVERSCROLL_PLUS: return GetCompositionEnd() - GetPageEnd();
  case OVERSCROLL_BOTH: return (GetCompositionEnd() - GetPageEnd()) +
                               (GetPageStart() - GetOrigin());
  default: return 0;
  }
}

Axis::Overscroll Axis::DisplacementWillOverscroll(int32_t aDisplacement) {
  
  
  bool minus = GetOrigin() + aDisplacement < GetPageStart();
  
  
  bool plus = GetCompositionEnd() + aDisplacement > GetPageEnd();
  if (minus && plus) {
    return OVERSCROLL_BOTH;
  }
  if (minus) {
    return OVERSCROLL_MINUS;
  }
  if (plus) {
    return OVERSCROLL_PLUS;
  }
  return OVERSCROLL_NONE;
}

float Axis::DisplacementWillOverscrollAmount(int32_t aDisplacement) {
  switch (DisplacementWillOverscroll(aDisplacement)) {
  case OVERSCROLL_MINUS: return (GetOrigin() + aDisplacement) - GetPageStart();
  case OVERSCROLL_PLUS: return (GetCompositionEnd() + aDisplacement) - GetPageEnd();
  
  
  default: return 0;
  }
}

Axis::Overscroll Axis::ScaleWillOverscroll(float aScale, int32_t aFocus) {
  float originAfterScale = (GetOrigin() + aFocus) * aScale - aFocus;

  bool both = ScaleWillOverscrollBothSides(aScale);
  bool minus = originAfterScale < GetPageStart() * aScale;
  bool plus = (originAfterScale + GetCompositionLength()) > GetPageEnd() * aScale;

  if ((minus && plus) || both) {
    return OVERSCROLL_BOTH;
  }
  if (minus) {
    return OVERSCROLL_MINUS;
  }
  if (plus) {
    return OVERSCROLL_PLUS;
  }
  return OVERSCROLL_NONE;
}

float Axis::ScaleWillOverscrollAmount(float aScale, int32_t aFocus) {
  float originAfterScale = (GetOrigin() + aFocus) * aScale - aFocus;
  switch (ScaleWillOverscroll(aScale, aFocus)) {
  case OVERSCROLL_MINUS: return originAfterScale - GetPageStart() * aScale;
  case OVERSCROLL_PLUS: return (originAfterScale + GetCompositionLength()) -
                               NS_lround(GetPageEnd() * aScale);
  
  default: return 0;
  }
}

float Axis::GetVelocity() {
  return mVelocity;
}

float Axis::GetAccelerationFactor() {
  return powf(gAccelerationMultiplier, std::max(0, (mAcceleration - 4) * 3));
}

float Axis::GetCompositionEnd() {
  return GetOrigin() + GetCompositionLength();
}

float Axis::GetPageEnd() {
  return GetPageStart() + GetPageLength();
}

float Axis::GetOrigin() {
  gfx::Point origin = mAsyncPanZoomController->GetFrameMetrics().mScrollOffset;
  return GetPointOffset(origin);
}

float Axis::GetCompositionLength() {
  const FrameMetrics& metrics = mAsyncPanZoomController->GetFrameMetrics();
  gfx::Rect cssCompositedRect =
    AsyncPanZoomController::CalculateCompositedRectInCssPixels(metrics);
  return GetRectLength(cssCompositedRect);
}

float Axis::GetPageStart() {
  gfx::Rect pageRect = mAsyncPanZoomController->GetFrameMetrics().mScrollableRect;
  return GetRectOffset(pageRect);
}

float Axis::GetPageLength() {
  gfx::Rect pageRect = mAsyncPanZoomController->GetFrameMetrics().mScrollableRect;
  return GetRectLength(pageRect);
}

bool Axis::ScaleWillOverscrollBothSides(float aScale) {
  const FrameMetrics& metrics = mAsyncPanZoomController->GetFrameMetrics();

  gfx::Rect cssContentRect = metrics.mScrollableRect;

  float currentScale = metrics.mZoom.width;
  gfx::Rect scaledCompositionBounds(metrics.mCompositionBounds);
  scaledCompositionBounds.ScaleInverseRoundIn(currentScale * aScale);

  return GetRectLength(cssContentRect) < GetRectLength(scaledCompositionBounds);
}

AxisX::AxisX(AsyncPanZoomController* aAsyncPanZoomController)
  : Axis(aAsyncPanZoomController)
{

}

float AxisX::GetPointOffset(const gfx::Point& aPoint)
{
  return aPoint.x;
}

float AxisX::GetRectLength(const gfx::Rect& aRect)
{
  return aRect.width;
}

float AxisX::GetRectOffset(const gfx::Rect& aRect)
{
  return aRect.x;
}

AxisY::AxisY(AsyncPanZoomController* aAsyncPanZoomController)
  : Axis(aAsyncPanZoomController)
{

}

float AxisY::GetPointOffset(const gfx::Point& aPoint)
{
  return aPoint.y;
}

float AxisY::GetRectLength(const gfx::Rect& aRect)
{
  return aRect.height;
}

float AxisY::GetRectOffset(const gfx::Rect& aRect)
{
  return aRect.y;
}

}
}
