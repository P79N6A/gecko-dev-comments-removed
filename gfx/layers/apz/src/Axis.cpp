





#include "Axis.h"
#include <math.h>                       
#include <algorithm>                    
#include "AsyncPanZoomController.h"     
#include "mozilla/dom/AnimationPlayer.h" 
#include "mozilla/layers/APZCTreeManager.h" 
#include "mozilla/layers/APZThreadUtils.h" 
#include "FrameMetrics.h"               
#include "mozilla/Attributes.h"         
#include "mozilla/Preferences.h"        
#include "mozilla/gfx/Rect.h"           
#include "mozilla/mozalloc.h"           
#include "mozilla/FloatingPoint.h"      
#include "mozilla/StaticPtr.h"          
#include "nsMathUtils.h"                
#include "nsPrintfCString.h"            
#include "nsThreadUtils.h"              
#include "nscore.h"                     
#include "gfxPrefs.h"                   

#define AXIS_LOG(...)


namespace mozilla {
namespace layers {

bool FuzzyEqualsCoordinate(float aValue1, float aValue2)
{
  return FuzzyEqualsAdditive(aValue1, aValue2, COORDINATE_EPSILON)
      || FuzzyEqualsMultiplicative(aValue1, aValue2);
}

extern StaticAutoPtr<ComputedTimingFunction> gVelocityCurveFunction;

Axis::Axis(AsyncPanZoomController* aAsyncPanZoomController)
  : mPos(0),
    mPosTimeMs(0),
    mVelocity(0.0f),
    mAxisLocked(false),
    mAsyncPanZoomController(aAsyncPanZoomController),
    mOverscroll(0),
    mFirstOverscrollAnimationSample(0),
    mLastOverscrollPeak(0),
    mOverscrollScale(1.0f)
{
}

float Axis::ToLocalVelocity(float aVelocityInchesPerMs) const {
  ScreenPoint velocity = MakePoint(aVelocityInchesPerMs * APZCTreeManager::GetDPI());
  
  
  ScreenPoint panStart = mAsyncPanZoomController->ToScreenCoordinates(
      mAsyncPanZoomController->PanStart(),
      ParentLayerPoint());
  ParentLayerPoint localVelocity =
      mAsyncPanZoomController->ToParentLayerCoordinates(velocity, panStart);
  return localVelocity.Length();
}

void Axis::UpdateWithTouchAtDevicePoint(ParentLayerCoord aPos, uint32_t aTimestampMs) {
  
  APZThreadUtils::AssertOnControllerThread();

  if (aTimestampMs == mPosTimeMs) {
    
    
    
    
    mPos = aPos;
    return;
  }

  float newVelocity = mAxisLocked ? 0.0f : (float)(mPos - aPos) / (float)(aTimestampMs - mPosTimeMs);
  if (gfxPrefs::APZMaxVelocity() > 0.0f) {
    bool velocityIsNegative = (newVelocity < 0);
    newVelocity = fabs(newVelocity);

    float maxVelocity = ToLocalVelocity(gfxPrefs::APZMaxVelocity());
    newVelocity = std::min(newVelocity, maxVelocity);

    if (gfxPrefs::APZCurveThreshold() > 0.0f && gfxPrefs::APZCurveThreshold() < gfxPrefs::APZMaxVelocity()) {
      float curveThreshold = ToLocalVelocity(gfxPrefs::APZCurveThreshold());
      if (newVelocity > curveThreshold) {
        
        float scale = maxVelocity - curveThreshold;
        float funcInput = (newVelocity - curveThreshold) / scale;
        float funcOutput = gVelocityCurveFunction->GetValue(funcInput);
        float curvedVelocity = (funcOutput * scale) + curveThreshold;
        AXIS_LOG("%p|%s curving up velocity from %f to %f\n",
          mAsyncPanZoomController, Name(), newVelocity, curvedVelocity);
        newVelocity = curvedVelocity;
      }
    }

    if (velocityIsNegative) {
      newVelocity = -newVelocity;
    }
  }

  AXIS_LOG("%p|%s updating velocity to %f with touch\n",
    mAsyncPanZoomController, Name(), newVelocity);
  mVelocity = newVelocity;
  mPos = aPos;
  mPosTimeMs = aTimestampMs;

  
  mVelocityQueue.AppendElement(std::make_pair(aTimestampMs, mVelocity));
  if (mVelocityQueue.Length() > gfxPrefs::APZMaxVelocityQueueSize()) {
    mVelocityQueue.RemoveElementAt(0);
  }
}

void Axis::StartTouch(ParentLayerCoord aPos, uint32_t aTimestampMs) {
  mStartPos = aPos;
  mPos = aPos;
  mPosTimeMs = aTimestampMs;
  mAxisLocked = false;
}

bool Axis::AdjustDisplacement(ParentLayerCoord aDisplacement,
                               float& aDisplacementOut,
                               float&
                              aOverscrollAmountOut,
                              bool forceOverscroll )
{
  if (mAxisLocked) {
    aOverscrollAmountOut = 0;
    aDisplacementOut = 0;
    return false;
  }
  if (forceOverscroll) {
    aOverscrollAmountOut = aDisplacement;
    aDisplacementOut = 0;
    return false;
  }

  StopSamplingOverscrollAnimation();

  ParentLayerCoord displacement = aDisplacement;

  
  ParentLayerCoord consumedOverscroll = 0;
  if (mOverscroll > 0 && aDisplacement < 0) {
    consumedOverscroll = std::min(mOverscroll, -aDisplacement);
  } else if (mOverscroll < 0 && aDisplacement > 0) {
    consumedOverscroll = 0.f - std::min(-mOverscroll, aDisplacement);
  }
  mOverscroll -= consumedOverscroll;
  displacement += consumedOverscroll;

  
  
  aOverscrollAmountOut = DisplacementWillOverscrollAmount(displacement);
  if (aOverscrollAmountOut != 0.0f) {
    
    
    AXIS_LOG("%p|%s has overscrolled, clearing velocity\n",
      mAsyncPanZoomController, Name());
    mVelocity = 0.0f;
    displacement -= aOverscrollAmountOut;
  }
  aDisplacementOut = displacement;
  return fabsf(consumedOverscroll) > EPSILON;
}

ParentLayerCoord Axis::ApplyResistance(ParentLayerCoord aRequestedOverscroll) const {
  
  
  
  
  
  
  float resistanceFactor = 1 - fabsf(GetOverscroll()) / GetCompositionLength();
  return resistanceFactor < 0 ? ParentLayerCoord(0) : aRequestedOverscroll * resistanceFactor;
}

void Axis::OverscrollBy(ParentLayerCoord aOverscroll) {
  MOZ_ASSERT(CanScroll());
  StopSamplingOverscrollAnimation();
  aOverscroll = ApplyResistance(aOverscroll);
  if (aOverscroll > 0) {
#ifdef DEBUG
    if (!FuzzyEqualsCoordinate(GetCompositionEnd().value, GetPageEnd().value)) {
      nsPrintfCString message("composition end (%f) is not equal (within error) to page end (%f)\n",
                              GetCompositionEnd().value, GetPageEnd().value);
      NS_ASSERTION(false, message.get());
      MOZ_CRASH();
    }
#endif
    MOZ_ASSERT(mOverscroll >= 0);
  } else if (aOverscroll < 0) {
#ifdef DEBUG
    if (!FuzzyEqualsCoordinate(GetOrigin().value, GetPageStart().value)) {
      nsPrintfCString message("composition origin (%f) is not equal (within error) to page origin (%f)\n",
                              GetOrigin().value, GetPageStart().value);
      NS_ASSERTION(false, message.get());
      MOZ_CRASH();
    }
#endif
    MOZ_ASSERT(mOverscroll <= 0);
  }
  mOverscroll += aOverscroll;
}

ParentLayerCoord Axis::GetOverscroll() const {
  ParentLayerCoord result = (mOverscroll - mLastOverscrollPeak) / mOverscrollScale;

  
#ifdef DEBUG
  if ((result.value * mFirstOverscrollAnimationSample.value) < 0.0f) {
    nsPrintfCString message("GetOverscroll() (%f) and first overscroll animation sample (%f) have different signs\n",
                            result.value, mFirstOverscrollAnimationSample.value);
    NS_ASSERTION(false, message.get());
    MOZ_CRASH();
  }
#endif

  return result;
}

void Axis::StopSamplingOverscrollAnimation() {
  ParentLayerCoord overscroll = GetOverscroll();
  ClearOverscroll();
  mOverscroll = overscroll;
}

void Axis::StepOverscrollAnimation(double aStepDurationMilliseconds) {
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  const float kSpringStiffness = gfxPrefs::APZOverscrollSpringStiffness();
  const float kSpringFriction = gfxPrefs::APZOverscrollSpringFriction();

  
  float springForce = -1 * kSpringStiffness * mOverscroll;
  
  float oldVelocity = mVelocity;
  mVelocity += springForce * aStepDurationMilliseconds;

  
  mVelocity *= pow(double(1 - kSpringFriction), aStepDurationMilliseconds);
  AXIS_LOG("%p|%s sampled overscroll animation, leaving velocity at %f\n",
    mAsyncPanZoomController, Name(), mVelocity);

  
  
  
  
  
  
  bool velocitySignChange = (oldVelocity * mVelocity) < 0 || mVelocity == 0;
  if (mFirstOverscrollAnimationSample == 0.0f) {
    mFirstOverscrollAnimationSample = mOverscroll;

    
    
    
    if (mOverscroll != 0 && ((mOverscroll > 0 ? oldVelocity : -oldVelocity) <= 0.0f)) {
      velocitySignChange = true;
    }
  }
  if (velocitySignChange) {
    bool oddOscillation = (mOverscroll.value * mFirstOverscrollAnimationSample.value) < 0.0f;
    mLastOverscrollPeak = oddOscillation ? mOverscroll : -mOverscroll;
    mOverscrollScale = 2.0f;
  }

  
  
  mOverscroll += (mVelocity * aStepDurationMilliseconds);

  
  
  
  
  
  
  
  
  
  if (mLastOverscrollPeak != 0 && fabs(mOverscroll) > fabs(mLastOverscrollPeak)) {
    mOverscroll = (mOverscroll >= 0) ? fabs(mLastOverscrollPeak) : -fabs(mLastOverscrollPeak);
  }
}

bool Axis::SampleOverscrollAnimation(const TimeDuration& aDelta) {
  
  if (mVelocity == 0.0f && mOverscroll == 0.0f) {
    return false;
  }

  
  
  
  
  
  
  
  
  
  
  double milliseconds = aDelta.ToMilliseconds();
  int wholeMilliseconds = (int) aDelta.ToMilliseconds();
  double fractionalMilliseconds = milliseconds - wholeMilliseconds;
  for (int i = 0; i < wholeMilliseconds; ++i) {
    StepOverscrollAnimation(1);
  }
  StepOverscrollAnimation(fractionalMilliseconds);

  
  
  
  if (fabs(mOverscroll) < gfxPrefs::APZOverscrollStopDistanceThreshold() &&
      fabs(mVelocity) < gfxPrefs::APZOverscrollStopVelocityThreshold()) {
    
    
    AXIS_LOG("%p|%s oscillation dropped below threshold, going to rest\n",
      mAsyncPanZoomController, Name());
    ClearOverscroll();
    mVelocity = 0;
    return false;
  }

  
  return true;
}

bool Axis::IsOverscrolled() const {
  return mOverscroll != 0.f;
}

void Axis::ClearOverscroll() {
  mOverscroll = 0;
  mFirstOverscrollAnimationSample = 0;
  mLastOverscrollPeak = 0;
  mOverscrollScale = 1.0f;
}

ParentLayerCoord Axis::PanStart() const {
  return mStartPos;
}

ParentLayerCoord Axis::PanDistance() const {
  return fabs(mPos - mStartPos);
}

ParentLayerCoord Axis::PanDistance(ParentLayerCoord aPos) const {
  return fabs(aPos - mStartPos);
}

void Axis::EndTouch(uint32_t aTimestampMs) {
  
  APZThreadUtils::AssertOnControllerThread();

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
  AXIS_LOG("%p|%s ending touch, computed velocity %f\n",
    mAsyncPanZoomController, Name(), mVelocity);
}

void Axis::CancelTouch() {
  
  APZThreadUtils::AssertOnControllerThread();

  AXIS_LOG("%p|%s cancelling touch, clearing velocity queue\n",
    mAsyncPanZoomController, Name());
  mVelocity = 0.0f;
  while (!mVelocityQueue.IsEmpty()) {
    mVelocityQueue.RemoveElementAt(0);
  }
}

bool Axis::CanScroll() const {
  return GetPageLength() - GetCompositionLength() > COORDINATE_EPSILON;
}

bool Axis::CanScroll(double aDelta) const
{
  if (!CanScroll() || mAxisLocked) {
    return false;
  }

  ParentLayerCoord delta = aDelta;
  return DisplacementWillOverscrollAmount(delta) != delta;
}

CSSCoord Axis::ClampOriginToScrollableRect(CSSCoord aOrigin) const
{
  CSSToParentLayerScale zoom = GetScaleForAxis(GetFrameMetrics().GetZoom());
  ParentLayerCoord origin = aOrigin * zoom;

  ParentLayerCoord result;
  if (origin < GetPageStart()) {
    result = GetPageStart();
  } else if (origin + GetCompositionLength() > GetPageEnd()) {
    result = GetPageEnd() - GetCompositionLength();
  } else {
    result = origin;
  }

  return result / zoom;
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
  AXIS_LOG("%p|%s reduced velocity to %f due to friction\n",
    mAsyncPanZoomController, Name(), mVelocity);
  return true;
}

ParentLayerCoord Axis::DisplacementWillOverscrollAmount(ParentLayerCoord aDisplacement) const {
  ParentLayerCoord newOrigin = GetOrigin() + aDisplacement;
  ParentLayerCoord newCompositionEnd = GetCompositionEnd() + aDisplacement;
  
  
  bool minus = newOrigin < GetPageStart();
  
  
  bool plus = newCompositionEnd > GetPageEnd();
  if (minus && plus) {
    
    
    return 0;
  }
  if (minus) {
    return newOrigin - GetPageStart();
  }
  if (plus) {
    return newCompositionEnd - GetPageEnd();
  }
  return 0;
}

CSSCoord Axis::ScaleWillOverscrollAmount(float aScale, CSSCoord aFocus) const {
  
  
  CSSToParentLayerScale zoom = GetFrameMetrics().GetZoom().ToScaleFactor();
  ParentLayerCoord focus = aFocus * zoom;
  ParentLayerCoord originAfterScale = (GetOrigin() + focus) - (focus / aScale);

  bool both = ScaleWillOverscrollBothSides(aScale);
  bool minus = GetPageStart() - originAfterScale > COORDINATE_EPSILON;
  bool plus = (originAfterScale + (GetCompositionLength() / aScale)) - GetPageEnd() > COORDINATE_EPSILON;

  if ((minus && plus) || both) {
    
    MOZ_ASSERT(false, "In an OVERSCROLL_BOTH condition in ScaleWillOverscrollAmount");
    return 0;
  }
  if (minus) {
    return (originAfterScale - GetPageStart()) / zoom;
  }
  if (plus) {
    return (originAfterScale + (GetCompositionLength() / aScale) - GetPageEnd()) / zoom;
  }
  return 0;
}

float Axis::GetVelocity() const {
  return mAxisLocked ? 0 : mVelocity;
}

void Axis::SetVelocity(float aVelocity) {
  AXIS_LOG("%p|%s direct-setting velocity to %f\n",
    mAsyncPanZoomController, Name(), aVelocity);
  mVelocity = aVelocity;
}

ParentLayerCoord Axis::GetCompositionEnd() const {
  return GetOrigin() + GetCompositionLength();
}

ParentLayerCoord Axis::GetPageEnd() const {
  return GetPageStart() + GetPageLength();
}

ParentLayerCoord Axis::GetOrigin() const {
  ParentLayerPoint origin = GetFrameMetrics().GetScrollOffset() * GetFrameMetrics().GetZoom();
  return GetPointOffset(origin);
}

ParentLayerCoord Axis::GetCompositionLength() const {
  return GetRectLength(GetFrameMetrics().mCompositionBounds);
}

ParentLayerCoord Axis::GetPageStart() const {
  ParentLayerRect pageRect = GetFrameMetrics().GetExpandedScrollableRect() * GetFrameMetrics().GetZoom();
  return GetRectOffset(pageRect);
}

ParentLayerCoord Axis::GetPageLength() const {
  ParentLayerRect pageRect = GetFrameMetrics().GetExpandedScrollableRect() * GetFrameMetrics().GetZoom();
  return GetRectLength(pageRect);
}

bool Axis::ScaleWillOverscrollBothSides(float aScale) const {
  const FrameMetrics& metrics = GetFrameMetrics();
  ParentLayerRect screenCompositionBounds = metrics.mCompositionBounds
                                          / ParentLayerToParentLayerScale(aScale);
  return GetRectLength(screenCompositionBounds) - GetPageLength() > COORDINATE_EPSILON;
}

const FrameMetrics& Axis::GetFrameMetrics() const {
  return mAsyncPanZoomController->GetFrameMetrics();
}


AxisX::AxisX(AsyncPanZoomController* aAsyncPanZoomController)
  : Axis(aAsyncPanZoomController)
{

}

ParentLayerCoord AxisX::GetPointOffset(const ParentLayerPoint& aPoint) const
{
  return aPoint.x;
}

ParentLayerCoord AxisX::GetRectLength(const ParentLayerRect& aRect) const
{
  return aRect.width;
}

ParentLayerCoord AxisX::GetRectOffset(const ParentLayerRect& aRect) const
{
  return aRect.x;
}

CSSToParentLayerScale AxisX::GetScaleForAxis(const CSSToParentLayerScale2D& aScale) const
{
  return CSSToParentLayerScale(aScale.xScale);
}

ScreenPoint AxisX::MakePoint(ScreenCoord aCoord) const
{
  return ScreenPoint(aCoord, 0);
}

const char* AxisX::Name() const
{
  return "X";
}

AxisY::AxisY(AsyncPanZoomController* aAsyncPanZoomController)
  : Axis(aAsyncPanZoomController)
{

}

ParentLayerCoord AxisY::GetPointOffset(const ParentLayerPoint& aPoint) const
{
  return aPoint.y;
}

ParentLayerCoord AxisY::GetRectLength(const ParentLayerRect& aRect) const
{
  return aRect.height;
}

ParentLayerCoord AxisY::GetRectOffset(const ParentLayerRect& aRect) const
{
  return aRect.y;
}

CSSToParentLayerScale AxisY::GetScaleForAxis(const CSSToParentLayerScale2D& aScale) const
{
  return CSSToParentLayerScale(aScale.yScale);
}

ScreenPoint AxisY::MakePoint(ScreenCoord aCoord) const
{
  return ScreenPoint(0, aCoord);
}

const char* AxisY::Name() const
{
  return "Y";
}

}
}
