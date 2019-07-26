





#include "Axis.h"
#include <math.h>                       
#include <algorithm>                    
#include "AsyncPanZoomController.h"     
#include "mozilla/layers/APZCTreeManager.h" 
#include "FrameMetrics.h"               
#include "mozilla/Attributes.h"         
#include "mozilla/Preferences.h"        
#include "mozilla/gfx/Rect.h"           
#include "mozilla/mozalloc.h"           
#include "mozilla/FloatingPoint.h"      
#include "nsMathUtils.h"                
#include "nsThreadUtils.h"              
#include "nscore.h"                     
#include "gfxPrefs.h"                   

namespace mozilla {
namespace layers {

Axis::Axis(AsyncPanZoomController* aAsyncPanZoomController)
  : mPos(0),
    mVelocity(0.0f),
    mAxisLocked(false),
    mAsyncPanZoomController(aAsyncPanZoomController),
    mOverscroll(0)
{
}

void Axis::UpdateWithTouchAtDevicePoint(int32_t aPos, const TimeDuration& aTimeDelta) {
  float newVelocity = mAxisLocked ? 0 : (mPos - aPos) / aTimeDelta.ToMilliseconds();
  if (gfxPrefs::APZMaxVelocity() > 0.0f) {
    newVelocity = std::min(newVelocity, gfxPrefs::APZMaxVelocity() * APZCTreeManager::GetDPI());
  }

  mVelocity = newVelocity;
  mPos = aPos;

  
  mVelocityQueue.AppendElement(mVelocity);
  if (mVelocityQueue.Length() > gfxPrefs::APZMaxVelocityQueueSize()) {
    mVelocityQueue.RemoveElementAt(0);
  }
}

void Axis::StartTouch(int32_t aPos) {
  mStartPos = aPos;
  mPos = aPos;
  mAxisLocked = false;
}

float Axis::AdjustDisplacement(float aDisplacement, float& aOverscrollAmountOut) {
  if (mAxisLocked) {
    aOverscrollAmountOut = 0;
    return 0;
  }

  float displacement = aDisplacement;

  
  if (mOverscroll > 0 && aDisplacement < 0) {
    float consumedOverscroll = std::min(mOverscroll, -aDisplacement);
    mOverscroll -= consumedOverscroll;
    displacement += consumedOverscroll;
  } else if (mOverscroll < 0 && aDisplacement > 0) {
    float consumedOverscroll = std::min(-mOverscroll, aDisplacement);
    mOverscroll += consumedOverscroll;
    displacement -= consumedOverscroll;
  }

  
  
  if (DisplacementWillOverscroll(displacement) != OVERSCROLL_NONE) {
    
    
    mVelocity = 0.0f;
    aOverscrollAmountOut = DisplacementWillOverscrollAmount(displacement);
    displacement -= aOverscrollAmountOut;
  }
  return displacement;
}

float Axis::ApplyResistance(float aRequestedOverscroll) const {
  
  
  
  
  
  
  float resistanceFactor = 1 - fabsf(mOverscroll) / GetCompositionLength();
  return resistanceFactor < 0 ? 0 : aRequestedOverscroll * resistanceFactor;
}

void Axis::OverscrollBy(float aOverscroll) {
  MOZ_ASSERT(CanScroll());
  aOverscroll = ApplyResistance(aOverscroll);
  if (aOverscroll > 0) {
    MOZ_ASSERT(FuzzyEqualsAdditive(GetCompositionEnd(), GetPageEnd(), COORDINATE_EPSILON));
    MOZ_ASSERT(mOverscroll >= 0);
  } else if (aOverscroll < 0) {
    MOZ_ASSERT(FuzzyEqualsAdditive(GetOrigin(), GetPageStart(), COORDINATE_EPSILON));
    MOZ_ASSERT(mOverscroll <= 0);
  }
  mOverscroll += aOverscroll;
}

float Axis::GetOverscroll() const {
  return mOverscroll;
}

void Axis::StartSnapBack() {
  float initialSnapBackVelocity = gfxPrefs::APZSnapBackInitialVelocity();
  if (mOverscroll > 0) {
    mVelocity = -initialSnapBackVelocity;
  } else {
    mVelocity = initialSnapBackVelocity;
  }
}

bool Axis::SampleSnapBack(const TimeDuration& aDelta) {
  
  
  
  
  mVelocity *= pow(1.0f + gfxPrefs::APZSnapBackAcceleration(), float(aDelta.ToMilliseconds()));
  float screenDisplacement = mVelocity * aDelta.ToMilliseconds();
  float cssDisplacement = screenDisplacement / GetFrameMetrics().GetZoom().scale;
  if (mOverscroll > 0) {
    if (cssDisplacement > 0) {
      NS_WARNING("Overscroll snap-back animation is moving in the wrong direction!");
      return false;
    }
    mOverscroll = std::max(mOverscroll + cssDisplacement, 0.0f);
    
    if (mOverscroll == 0) {
      mVelocity = 0;
      return false;
    }
    return true;
  } else if (mOverscroll < 0) {
    if (cssDisplacement < 0) {
      NS_WARNING("Overscroll snap-back animation is moving in the wrong direction!");
      return false;
    }
    mOverscroll = std::min(mOverscroll + cssDisplacement, 0.0f);
    
    if (mOverscroll == 0) {
      mVelocity = 0;
      return false;
    }
    return true;
  }
  
  return false;
}

bool Axis::IsOverscrolled() const {
  return mOverscroll != 0;
}

float Axis::PanDistance() {
  return fabsf(mPos - mStartPos);
}

float Axis::PanDistance(float aPos) {
  return fabsf(aPos - mStartPos);
}

void Axis::EndTouch() {
  
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
  while (!mVelocityQueue.IsEmpty()) {
    mVelocityQueue.RemoveElementAt(0);
  }
}

bool Axis::CanScroll() const {
  return GetCompositionLength() < GetPageLength();
}

bool Axis::CanScrollNow() const {
  return !mAxisLocked && CanScroll();
}

bool Axis::FlingApplyFrictionOrCancel(const TimeDuration& aDelta) {
  if (fabsf(mVelocity) <= gfxPrefs::APZFlingStoppedThreshold()) {
    
    
    
    mVelocity = 0.0f;
    return false;
  } else {
    mVelocity *= pow(1.0f - gfxPrefs::APZFlingFriction(), float(aDelta.ToMilliseconds()));
  }
  return true;
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
  return mAxisLocked ? 0 : mVelocity;
}

void Axis::SetVelocity(float aVelocity) {
  mVelocity = aVelocity;
}

float Axis::GetCompositionEnd() const {
  return GetOrigin() + GetCompositionLength();
}

float Axis::GetPageEnd() const {
  return GetPageStart() + GetPageLength();
}

float Axis::GetOrigin() const {
  CSSPoint origin = GetFrameMetrics().GetScrollOffset();
  return GetPointOffset(origin);
}

float Axis::GetCompositionLength() const {
  return GetRectLength(GetFrameMetrics().CalculateCompositedRectInCssPixels());
}

float Axis::GetPageStart() const {
  CSSRect pageRect = GetFrameMetrics().GetExpandedScrollableRect();
  return GetRectOffset(pageRect);
}

float Axis::GetPageLength() const {
  CSSRect pageRect = GetFrameMetrics().GetExpandedScrollableRect();
  return GetRectLength(pageRect);
}

bool Axis::ScaleWillOverscrollBothSides(float aScale) {
  const FrameMetrics& metrics = GetFrameMetrics();

  CSSToParentLayerScale scale(metrics.GetZoomToParent().scale * aScale);
  CSSRect cssCompositionBounds = metrics.mCompositionBounds / scale;

  return GetRectLength(metrics.GetExpandedScrollableRect()) < GetRectLength(cssCompositionBounds);
}

const FrameMetrics& Axis::GetFrameMetrics() const {
  return mAsyncPanZoomController->GetFrameMetrics();
}


AxisX::AxisX(AsyncPanZoomController* aAsyncPanZoomController)
  : Axis(aAsyncPanZoomController)
{

}

float AxisX::GetPointOffset(const CSSPoint& aPoint) const
{
  return aPoint.x;
}

float AxisX::GetRectLength(const CSSRect& aRect) const
{
  return aRect.width;
}

float AxisX::GetRectOffset(const CSSRect& aRect) const
{
  return aRect.x;
}

AxisY::AxisY(AsyncPanZoomController* aAsyncPanZoomController)
  : Axis(aAsyncPanZoomController)
{

}

float AxisY::GetPointOffset(const CSSPoint& aPoint) const
{
  return aPoint.y;
}

float AxisY::GetRectLength(const CSSRect& aRect) const
{
  return aRect.height;
}

float AxisY::GetRectOffset(const CSSRect& aRect) const
{
  return aRect.y;
}

}
}
