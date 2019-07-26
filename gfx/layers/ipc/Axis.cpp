





#include "Axis.h"
#include <math.h>                       
#include <algorithm>                    
#include "AsyncPanZoomController.h"     
#include "FrameMetrics.h"               
#include "mozilla/Attributes.h"         
#include "mozilla/Preferences.h"        
#include "mozilla/gfx/Rect.h"           
#include "mozilla/mozalloc.h"           
#include "nsMathUtils.h"                
#include "nsThreadUtils.h"              
#include "nscore.h"                     

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
  : mPos(0),
    mVelocity(0.0f),
    mAcceleration(0),
    mScrollingDisabled(false),
    mAsyncPanZoomController(aAsyncPanZoomController)
{
  InitAxisPrefs();
}

void Axis::UpdateWithTouchAtDevicePoint(int32_t aPos, const TimeDuration& aTimeDelta) {
  float newVelocity = mScrollingDisabled ? 0 : (mPos - aPos) / aTimeDelta.ToMilliseconds();

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
  mScrollingDisabled = false;
}

float Axis::AdjustDisplacement(float aDisplacement, float& aOverscrollAmountOut) {
  if (mScrollingDisabled) {
    aOverscrollAmountOut = 0;
    return 0;
  }

  if (fabsf(mVelocity) < gVelocityThreshold) {
    mAcceleration = 0;
  }

  float accelerationFactor = GetAccelerationFactor();
  float displacement = aDisplacement * accelerationFactor;
  
  
  if (DisplacementWillOverscroll(displacement) != OVERSCROLL_NONE) {
    
    
    mVelocity = 0.0f;
    mAcceleration = 0;
    aOverscrollAmountOut = DisplacementWillOverscrollAmount(displacement);
    displacement -= aOverscrollAmountOut;
  }
  return displacement;
}

float Axis::PanDistance() {
  return fabsf(mPos - mStartPos);
}

float Axis::PanDistance(float aPos) {
  return fabsf(aPos - mStartPos);
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

bool Axis::Scrollable() {
    if (mScrollingDisabled) {
        return false;
    }
    return GetCompositionLength() < GetPageLength();
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

Axis::Overscroll Axis::DisplacementWillOverscroll(float aDisplacement) {
  
  
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

float Axis::DisplacementWillOverscrollAmount(float aDisplacement) {
  switch (DisplacementWillOverscroll(aDisplacement)) {
  case OVERSCROLL_MINUS: return (GetOrigin() + aDisplacement) - GetPageStart();
  case OVERSCROLL_PLUS: return (GetCompositionEnd() + aDisplacement) - GetPageEnd();
  
  
  default: return 0;
  }
}

float Axis::ScaleWillOverscrollAmount(float aScale, float aFocus) {
  float originAfterScale = (GetOrigin() + aFocus) - (aFocus / aScale);

  bool both = ScaleWillOverscrollBothSides(aScale);
  bool minus = originAfterScale < GetPageStart();
  bool plus = (originAfterScale + (GetCompositionLength() / aScale)) > GetPageEnd();

  if ((minus && plus) || both) {
    
    MOZ_ASSERT(false, "In an OVERSCROLL_BOTH condition in ScaleWillOverscrollAmount");
    return 0;
  }
  if (minus) {
    return originAfterScale - GetPageStart();
  }
  if (plus) {
    return originAfterScale + (GetCompositionLength() / aScale) - GetPageEnd();
  }
  return 0;
}

float Axis::GetVelocity() {
  return mScrollingDisabled ? 0 : mVelocity;
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
  CSSPoint origin = mAsyncPanZoomController->GetFrameMetrics().mScrollOffset;
  return GetPointOffset(origin);
}

float Axis::GetCompositionLength() {
  const FrameMetrics& metrics = mAsyncPanZoomController->GetFrameMetrics();
  CSSRect cssCompositedRect = metrics.CalculateCompositedRectInCssPixels();
  return GetRectLength(cssCompositedRect);
}

float Axis::GetPageStart() {
  CSSRect pageRect = mAsyncPanZoomController->GetFrameMetrics().mScrollableRect;
  return GetRectOffset(pageRect);
}

float Axis::GetPageLength() {
  CSSRect pageRect = mAsyncPanZoomController->GetFrameMetrics().mScrollableRect;
  return GetRectLength(pageRect);
}

bool Axis::ScaleWillOverscrollBothSides(float aScale) {
  const FrameMetrics& metrics = mAsyncPanZoomController->GetFrameMetrics();

  CSSToScreenScale scale(metrics.mZoom.scale * aScale);
  CSSRect cssCompositionBounds = metrics.mCompositionBounds / scale;

  return GetRectLength(metrics.mScrollableRect) < GetRectLength(cssCompositionBounds);
}

AxisX::AxisX(AsyncPanZoomController* aAsyncPanZoomController)
  : Axis(aAsyncPanZoomController)
{

}

float AxisX::GetPointOffset(const CSSPoint& aPoint)
{
  return aPoint.x;
}

float AxisX::GetRectLength(const CSSRect& aRect)
{
  return aRect.width;
}

float AxisX::GetRectOffset(const CSSRect& aRect)
{
  return aRect.x;
}

AxisY::AxisY(AsyncPanZoomController* aAsyncPanZoomController)
  : Axis(aAsyncPanZoomController)
{

}

float AxisY::GetPointOffset(const CSSPoint& aPoint)
{
  return aPoint.y;
}

float AxisY::GetRectLength(const CSSRect& aRect)
{
  return aRect.height;
}

float AxisY::GetRectOffset(const CSSRect& aRect)
{
  return aRect.y;
}

}
}
