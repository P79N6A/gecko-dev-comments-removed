





#include "Axis.h"
#include "AsyncPanZoomController.h"

namespace mozilla {
namespace layers {

static const float EPSILON = 0.0001f;







static const float MAX_EVENT_ACCELERATION = 0.5f;




static const float FLING_FRICTION = 0.007f;





static const float VELOCITY_THRESHOLD = 0.14f;









static const float ACCELERATION_MULTIPLIER = 1.125f;






static const float FLING_STOPPED_THRESHOLD = 0.01f;

Axis::Axis(AsyncPanZoomController* aAsyncPanZoomController)
  : mPos(0.0f),
    mVelocity(0.0f),
    mAcceleration(0),
    mAsyncPanZoomController(aAsyncPanZoomController),
    mLockPanning(false)
{

}

void Axis::UpdateWithTouchAtDevicePoint(PRInt32 aPos, const TimeDuration& aTimeDelta) {
  if (mLockPanning) {
    return;
  }

  float newVelocity = (mPos - aPos) / aTimeDelta.ToMilliseconds();

  bool curVelocityIsLow = fabsf(newVelocity) < 0.01f;
  bool curVelocityBelowThreshold = fabsf(newVelocity) < VELOCITY_THRESHOLD;
  bool directionChange = (mVelocity > 0) != (newVelocity > 0);

  
  
  if (directionChange || curVelocityBelowThreshold) {
    mAcceleration = 0;
  }

  
  
  if (curVelocityIsLow || (directionChange && fabs(newVelocity) - EPSILON <= 0.0f)) {
    mVelocity = newVelocity;
  } else {
    float maxChange = fabsf(mVelocity * aTimeDelta.ToMilliseconds() * MAX_EVENT_ACCELERATION);
    mVelocity = NS_MIN(mVelocity + maxChange, NS_MAX(mVelocity - maxChange, newVelocity));
  }

  mVelocity = newVelocity;
  mPos = aPos;
}

void Axis::StartTouch(PRInt32 aPos) {
  mStartPos = aPos;
  mPos = aPos;
  mLockPanning = false;
}

float Axis::GetDisplacementForDuration(float aScale, const TimeDuration& aDelta) {
  float velocityFactor = powf(ACCELERATION_MULTIPLIER,
                              NS_MAX(0, (mAcceleration - 4) * 3));
  float displacement = mVelocity * aScale * aDelta.ToMilliseconds() * velocityFactor;
  
  
  if (DisplacementWillOverscroll(displacement) != OVERSCROLL_NONE) {
    
    
    mVelocity = 0.0f;
    displacement -= DisplacementWillOverscrollAmount(displacement);
  }
  return displacement;
}

float Axis::PanDistance() {
  return fabsf(mPos - mStartPos);
}

void Axis::EndTouch() {
  mAcceleration++;
}

void Axis::CancelTouch() {
  mVelocity = 0.0f;
  mAcceleration = 0;
}

void Axis::LockPanning() {
  mLockPanning = true;
}

bool Axis::FlingApplyFrictionOrCancel(const TimeDuration& aDelta) {
  if (fabsf(mVelocity) <= FLING_STOPPED_THRESHOLD) {
    
    
    
    mVelocity = 0.0f;
    return false;
  } else {
    mVelocity *= NS_MAX(1.0f - FLING_FRICTION * aDelta.ToMilliseconds(), 0.0);
  }
  return true;
}

Axis::Overscroll Axis::GetOverscroll() {
  
  
  bool minus = GetOrigin() < GetPageStart();
  
  
  bool plus = GetViewportEnd() > GetPageEnd();
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
  case OVERSCROLL_PLUS: return GetViewportEnd() - GetPageEnd();
  case OVERSCROLL_BOTH: return (GetViewportEnd() - GetPageEnd()) + (GetPageStart() - GetOrigin());
  default: return 0;
  }
}

Axis::Overscroll Axis::DisplacementWillOverscroll(PRInt32 aDisplacement) {
  
  
  bool minus = GetOrigin() + aDisplacement < GetPageStart();
  
  
  bool plus = GetViewportEnd() + aDisplacement > GetPageEnd();
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

float Axis::DisplacementWillOverscrollAmount(PRInt32 aDisplacement) {
  switch (DisplacementWillOverscroll(aDisplacement)) {
  case OVERSCROLL_MINUS: return (GetOrigin() + aDisplacement) - GetPageStart();
  case OVERSCROLL_PLUS: return (GetViewportEnd() + aDisplacement) - GetPageEnd();
  
  
  default: return 0;
  }
}

Axis::Overscroll Axis::ScaleWillOverscroll(float aScale, PRInt32 aFocus) {
  float originAfterScale = (GetOrigin() + aFocus) * aScale - aFocus;

  bool both = ScaleWillOverscrollBothSides(aScale);
  bool minus = originAfterScale < GetPageStart() * aScale;
  bool plus = (originAfterScale + GetViewportLength()) > GetPageEnd() * aScale;

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

float Axis::ScaleWillOverscrollAmount(float aScale, PRInt32 aFocus) {
  float originAfterScale = (GetOrigin() + aFocus) * aScale - aFocus;
  switch (ScaleWillOverscroll(aScale, aFocus)) {
  case OVERSCROLL_MINUS: return originAfterScale - GetPageStart() * aScale;
  case OVERSCROLL_PLUS: return (originAfterScale + GetViewportLength()) - GetPageEnd() * aScale;
  
  default: return 0;
  }
}

float Axis::GetVelocity() {
  return mVelocity;
}

float Axis::GetViewportEnd() {
  return GetOrigin() + GetViewportLength();
}

float Axis::GetPageEnd() {
  return GetPageStart() + GetPageLength();
}

float Axis::GetOrigin() {
  gfx::Point origin = mAsyncPanZoomController->GetFrameMetrics().mViewportScrollOffset;
  return GetPointOffset(origin);
}

float Axis::GetViewportLength() {
  nsIntRect viewport = mAsyncPanZoomController->GetFrameMetrics().mViewport;
  gfx::Rect scaledViewport = gfx::Rect(viewport.x, viewport.y, viewport.width, viewport.height);
  scaledViewport.ScaleRoundIn(1 / mAsyncPanZoomController->GetFrameMetrics().mResolution.width);
  return GetRectLength(scaledViewport);
}

float Axis::GetPageStart() {
  gfx::Rect pageRect = mAsyncPanZoomController->GetFrameMetrics().mCSSContentRect;
  return GetRectOffset(pageRect);
}

float Axis::GetPageLength() {
  gfx::Rect pageRect = mAsyncPanZoomController->GetFrameMetrics().mCSSContentRect;
  return GetRectLength(pageRect);
}

bool Axis::ScaleWillOverscrollBothSides(float aScale) {
  const FrameMetrics& metrics = mAsyncPanZoomController->GetFrameMetrics();

  gfx::Rect cssContentRect = metrics.mCSSContentRect;

  float currentScale = metrics.mResolution.width;
  gfx::Rect viewport = gfx::Rect(metrics.mViewport.x,
                                 metrics.mViewport.y,
                                 metrics.mViewport.width,
                                 metrics.mViewport.height);
  viewport.ScaleRoundIn(1 / (currentScale * aScale));

  return GetRectLength(cssContentRect) < GetRectLength(viewport);
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
