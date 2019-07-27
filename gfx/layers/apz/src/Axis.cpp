





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
#include "nsPrintfCString.h"            
#include "nsThreadUtils.h"              
#include "nscore.h"                     
#include "gfxPrefs.h"                   

namespace mozilla {
namespace layers {

Axis::Axis(AsyncPanZoomController* aAsyncPanZoomController)
  : mPos(0),
    mPosTimeMs(0),
    mVelocity(0.0f),
    mAxisLocked(false),
    mAsyncPanZoomController(aAsyncPanZoomController),
    mOverscroll(0)
{
}

void Axis::UpdateWithTouchAtDevicePoint(ScreenCoord aPos, uint32_t aTimestampMs) {
  
  AsyncPanZoomController::AssertOnControllerThread();

  if (aTimestampMs == mPosTimeMs) {
    
    
    
    
    mPos = aPos;
    return;
  }

  float newVelocity = mAxisLocked ? 0.0f : (float)(mPos - aPos) / (float)(aTimestampMs - mPosTimeMs);
  if (gfxPrefs::APZMaxVelocity() > 0.0f) {
    newVelocity = std::min(newVelocity, gfxPrefs::APZMaxVelocity() * APZCTreeManager::GetDPI());
  }

  mVelocity = newVelocity;
  mPos = aPos;
  mPosTimeMs = aTimestampMs;

  
  mVelocityQueue.AppendElement(std::make_pair(aTimestampMs, mVelocity));
  if (mVelocityQueue.Length() > gfxPrefs::APZMaxVelocityQueueSize()) {
    mVelocityQueue.RemoveElementAt(0);
  }
}

void Axis::StartTouch(ScreenCoord aPos, uint32_t aTimestampMs) {
  mStartPos = aPos;
  mPos = aPos;
  mPosTimeMs = aTimestampMs;
  mAxisLocked = false;
}

bool Axis::AdjustDisplacement(CSSCoord aDisplacement,
                               float& aDisplacementOut,
                               float& aOverscrollAmountOut)
{
  if (mAxisLocked) {
    aOverscrollAmountOut = 0;
    aDisplacementOut = 0;
    return false;
  }

  CSSCoord displacement = aDisplacement;

  
  CSSCoord consumedOverscroll = 0;
  if (mOverscroll > 0 && aDisplacement < 0) {
    consumedOverscroll = std::min(mOverscroll, -aDisplacement);
  } else if (mOverscroll < 0 && aDisplacement > 0) {
    consumedOverscroll = 0.f - std::min(-mOverscroll, aDisplacement);
  }
  mOverscroll -= consumedOverscroll;
  displacement += consumedOverscroll;

  
  
  if (DisplacementWillOverscroll(displacement) != OVERSCROLL_NONE) {
    
    
    mVelocity = 0.0f;
    aOverscrollAmountOut = DisplacementWillOverscrollAmount(displacement);
    displacement -= aOverscrollAmountOut;
  }
  aDisplacementOut = displacement;
  return fabsf(consumedOverscroll) > EPSILON;
}

CSSCoord Axis::ApplyResistance(CSSCoord aRequestedOverscroll) const {
  
  
  
  
  
  
  float resistanceFactor = 1 - fabsf(mOverscroll) / GetCompositionLength();
  return resistanceFactor < 0 ? CSSCoord(0) : aRequestedOverscroll * resistanceFactor;
}

void Axis::OverscrollBy(CSSCoord aOverscroll) {
  MOZ_ASSERT(CanScroll());
  aOverscroll = ApplyResistance(aOverscroll);
  if (aOverscroll > 0) {
#ifdef DEBUG
    if (!FuzzyEqualsAdditive(GetCompositionEnd().value, GetPageEnd().value, COORDINATE_EPSILON)) {
      nsPrintfCString message("composition end (%f) is not within COORDINATE_EPISLON of page end (%f)\n",
                              GetCompositionEnd().value, GetPageEnd().value);
      NS_ASSERTION(false, message.get());
      MOZ_CRASH();
    }
#endif
    MOZ_ASSERT(mOverscroll >= 0);
  } else if (aOverscroll < 0) {
#ifdef DEBUG
    if (!FuzzyEqualsAdditive(GetOrigin().value, GetPageStart().value, COORDINATE_EPSILON)) {
      nsPrintfCString message("composition origin (%f) is not within COORDINATE_EPISLON of page origin (%f)\n",
                              GetOrigin().value, GetPageStart().value);
      NS_ASSERTION(false, message.get());
      MOZ_CRASH();
    }
#endif
    MOZ_ASSERT(mOverscroll <= 0);
  }
  mOverscroll += aOverscroll;
}

CSSCoord Axis::GetOverscroll() const {
  return mOverscroll;
}

bool Axis::SampleSnapBack(const TimeDuration& aDelta) {
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  const float kSpringStiffness = gfxPrefs::APZOverscrollSnapBackSpringStiffness();
  const float kSpringFriction = gfxPrefs::APZOverscrollSnapBackSpringFriction();
  const float kMass = gfxPrefs::APZOverscrollSnapBackMass();
  float force = -1 * kSpringStiffness * mOverscroll - kSpringFriction * mVelocity;
  float acceleration = force / kMass;
  mVelocity += acceleration * aDelta.ToMilliseconds();
  float screenDisplacement = mVelocity * aDelta.ToMilliseconds();
  float cssDisplacement = screenDisplacement / GetFrameMetrics().GetZoom().scale;
  if (mOverscroll > 0) {
    if (cssDisplacement > 0) {
      NS_WARNING("Overscroll snap-back animation is moving in the wrong direction!");
      return false;
    }
    mOverscroll = std::max(mOverscroll + cssDisplacement, 0.0f);
    
    if (mOverscroll == 0.f) {
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
    
    if (mOverscroll == 0.f) {
      mVelocity = 0;
      return false;
    }
    return true;
  }
  
  return false;
}

bool Axis::IsOverscrolled() const {
  return mOverscroll != 0.f;
}

void Axis::ClearOverscroll() {
  mOverscroll = 0;
}

ScreenCoord Axis::PanDistance() const {
  return fabs(mPos - mStartPos);
}

ScreenCoord Axis::PanDistance(ScreenCoord aPos) const {
  return fabs(aPos - mStartPos);
}

void Axis::EndTouch(uint32_t aTimestampMs) {
  
  AsyncPanZoomController::AssertOnControllerThread();

  mVelocity = 0;
  int count = 0;
  while (!mVelocityQueue.IsEmpty()) {
    uint32_t timeDelta = (aTimestampMs - mVelocityQueue[0].first);
    if (timeDelta < gfxPrefs::APZVelocityRelevanceTime()) {
      count++;
      mVelocity += mVelocityQueue[0].second;
    }
    mVelocityQueue.RemoveElementAt(0);
  }
  if (count > 1) {
    mVelocity /= count;
  }
}

void Axis::CancelTouch() {
  
  AsyncPanZoomController::AssertOnControllerThread();

  mVelocity = 0.0f;
  while (!mVelocityQueue.IsEmpty()) {
    mVelocityQueue.RemoveElementAt(0);
  }
}

bool Axis::CanScroll() const {
  return GetPageLength() - GetCompositionLength() > COORDINATE_EPSILON;
}

bool Axis::CanScrollNow() const {
  return !mAxisLocked && CanScroll();
}

bool Axis::FlingApplyFrictionOrCancel(const TimeDuration& aDelta,
                                      float aFriction,
                                      float aThreshold) {
  if (fabsf(mVelocity) <= aThreshold) {
    
    
    
    mVelocity = 0.0f;
    return false;
  } else {
    mVelocity *= pow(1.0f - aFriction, float(aDelta.ToMilliseconds()));
  }
  return true;
}

Axis::Overscroll Axis::DisplacementWillOverscroll(CSSCoord aDisplacement) {
  
  
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

CSSCoord Axis::DisplacementWillOverscrollAmount(CSSCoord aDisplacement) {
  switch (DisplacementWillOverscroll(aDisplacement)) {
  case OVERSCROLL_MINUS: return (GetOrigin() + aDisplacement) - GetPageStart();
  case OVERSCROLL_PLUS: return (GetCompositionEnd() + aDisplacement) - GetPageEnd();
  
  
  default: return 0;
  }
}

CSSCoord Axis::ScaleWillOverscrollAmount(float aScale, CSSCoord aFocus) {
  CSSCoord originAfterScale = (GetOrigin() + aFocus) - (aFocus / aScale);

  bool both = ScaleWillOverscrollBothSides(aScale);
  bool minus = GetPageStart() - originAfterScale > COORDINATE_EPSILON;
  bool plus = (originAfterScale + (GetCompositionLength() / aScale)) - GetPageEnd() > COORDINATE_EPSILON;

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

CSSCoord Axis::GetCompositionEnd() const {
  return GetOrigin() + GetCompositionLength();
}

CSSCoord Axis::GetPageEnd() const {
  return GetPageStart() + GetPageLength();
}

CSSCoord Axis::GetOrigin() const {
  CSSPoint origin = GetFrameMetrics().GetScrollOffset();
  return GetPointOffset(origin);
}

CSSCoord Axis::GetCompositionLength() const {
  return GetRectLength(GetFrameMetrics().CalculateCompositedRectInCssPixels());
}

CSSCoord Axis::GetPageStart() const {
  CSSRect pageRect = GetFrameMetrics().GetExpandedScrollableRect();
  return GetRectOffset(pageRect);
}

CSSCoord Axis::GetPageLength() const {
  CSSRect pageRect = GetFrameMetrics().GetExpandedScrollableRect();
  return GetRectLength(pageRect);
}

bool Axis::ScaleWillOverscrollBothSides(float aScale) {
  const FrameMetrics& metrics = GetFrameMetrics();

  CSSToParentLayerScale scale(metrics.GetZoomToParent().scale * aScale);
  CSSRect cssCompositionBounds = metrics.mCompositionBounds / scale;

  return GetRectLength(cssCompositionBounds) - GetRectLength(metrics.GetExpandedScrollableRect()) > COORDINATE_EPSILON;
}

const FrameMetrics& Axis::GetFrameMetrics() const {
  return mAsyncPanZoomController->GetFrameMetrics();
}


AxisX::AxisX(AsyncPanZoomController* aAsyncPanZoomController)
  : Axis(aAsyncPanZoomController)
{

}

CSSCoord AxisX::GetPointOffset(const CSSPoint& aPoint) const
{
  return aPoint.x;
}

CSSCoord AxisX::GetRectLength(const CSSRect& aRect) const
{
  return aRect.width;
}

CSSCoord AxisX::GetRectOffset(const CSSRect& aRect) const
{
  return aRect.x;
}

AxisY::AxisY(AsyncPanZoomController* aAsyncPanZoomController)
  : Axis(aAsyncPanZoomController)
{

}

CSSCoord AxisY::GetPointOffset(const CSSPoint& aPoint) const
{
  return aPoint.y;
}

CSSCoord AxisY::GetRectLength(const CSSRect& aRect) const
{
  return aRect.height;
}

CSSCoord AxisY::GetRectOffset(const CSSRect& aRect) const
{
  return aRect.y;
}

}
}
