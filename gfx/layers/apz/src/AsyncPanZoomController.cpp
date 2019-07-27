





#include <math.h>                       
#include <stdint.h>                     
#include <sys/types.h>                  
#include <algorithm>                    
#include "AsyncPanZoomController.h"     
#include "Axis.h"                       
#include "Compositor.h"                 
#include "FrameMetrics.h"               
#include "GestureEventListener.h"       
#include "InputData.h"                  
#include "InputBlockState.h"            
#include "InputQueue.h"                 
#include "OverscrollHandoffState.h"     
#include "TaskThrottler.h"              
#include "Units.h"                      
#include "UnitTransforms.h"             
#include "base/message_loop.h"          
#include "base/task.h"                  
#include "base/tracked.h"               
#include "gfxPrefs.h"                   
#include "gfxTypes.h"                   
#include "mozilla/Assertions.h"         
#include "mozilla/BasicEvents.h"        
#include "mozilla/ClearOnShutdown.h"    
#include "mozilla/Constants.h"          
#include "mozilla/EventForwards.h"      
#include "mozilla/Preferences.h"        
#include "mozilla/ReentrantMonitor.h"   
#include "mozilla/StaticPtr.h"          
#include "mozilla/TimeStamp.h"          
#include "mozilla/dom/AnimationPlayer.h" 
#include "mozilla/dom/Touch.h"          
#include "mozilla/gfx/BasePoint.h"      
#include "mozilla/gfx/BaseRect.h"       
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Rect.h"           
#include "mozilla/gfx/ScaleFactor.h"    
#include "mozilla/layers/APZCTreeManager.h"  
#include "mozilla/layers/APZThreadUtils.h"  
#include "mozilla/layers/AsyncCompositionManager.h"  
#include "mozilla/layers/AxisPhysicsModel.h" 
#include "mozilla/layers/AxisPhysicsMSDModel.h" 
#include "mozilla/layers/CompositorParent.h" 
#include "mozilla/layers/LayerTransactionParent.h" 
#include "mozilla/layers/PCompositorParent.h" 
#include "mozilla/mozalloc.h"           
#include "mozilla/unused.h"             
#include "mozilla/FloatingPoint.h"      
#include "nsAlgorithm.h"                
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsDebug.h"                    
#include "nsIDOMWindowUtils.h"          
#include "nsMathUtils.h"                
#include "nsPoint.h"                    
#include "nsStyleConsts.h"
#include "nsStyleStruct.h"              
#include "nsTArray.h"                   
#include "nsThreadUtils.h"              
#include "SharedMemoryBasic.h"          
#include "WheelScrollAnimation.h"



#define ENABLE_APZC_LOGGING 0


#if ENABLE_APZC_LOGGING
#  include "LayersLogging.h"
#  define APZC_LOG(...) printf_stderr("APZC: " __VA_ARGS__)
#  define APZC_LOG_FM(fm, prefix, ...) \
    { std::stringstream ss; \
      ss << nsPrintfCString(prefix, __VA_ARGS__).get(); \
      AppendToString(ss, fm, ":", "", true); \
      APZC_LOG("%s\n", ss.str().c_str()); \
    }
#else
#  define APZC_LOG(...)
#  define APZC_LOG_FM(fm, prefix, ...)
#endif

namespace mozilla {
namespace layers {

typedef mozilla::layers::AllowedTouchBehavior AllowedTouchBehavior;
typedef GeckoContentController::APZStateChange APZStateChange;
typedef mozilla::gfx::Point Point;
typedef mozilla::gfx::Matrix4x4 Matrix4x4;
using mozilla::gfx::PointTyped;




















































































































































































































































StaticAutoPtr<ComputedTimingFunction> gZoomAnimationFunction;




StaticAutoPtr<ComputedTimingFunction> gVelocityCurveFunction;




static const CSSToParentLayerScale MAX_ZOOM(8.0f);




static const CSSToParentLayerScale MIN_ZOOM(0.125f);






static bool IsCloseToHorizontal(float aAngle, float aThreshold)
{
  return (aAngle < aThreshold || aAngle > (M_PI - aThreshold));
}


static bool IsCloseToVertical(float aAngle, float aThreshold)
{
  return (fabs(aAngle - (M_PI / 2)) < aThreshold);
}

template <typename Units>
static bool IsZero(const gfx::PointTyped<Units>& aPoint)
{
  return FuzzyEqualsAdditive(aPoint.x, 0.0f)
      && FuzzyEqualsAdditive(aPoint.y, 0.0f);
}

static inline void LogRendertraceRect(const ScrollableLayerGuid& aGuid, const char* aDesc, const char* aColor, const CSSRect& aRect)
{
#ifdef APZC_ENABLE_RENDERTRACE
  static const TimeStamp sRenderStart = TimeStamp::Now();
  TimeDuration delta = TimeStamp::Now() - sRenderStart;
  printf_stderr("(%llu,%lu,%llu)%s RENDERTRACE %f rect %s %f %f %f %f\n",
    aGuid.mLayersId, aGuid.mPresShellId, aGuid.mScrollId,
    aDesc, delta.ToMilliseconds(), aColor,
    aRect.x, aRect.y, aRect.width, aRect.height);
#endif
}

static TimeStamp sFrameTime;


static uint32_t sAsyncPanZoomControllerCount = 0;

TimeStamp
AsyncPanZoomController::GetFrameTime()
{
  if (sFrameTime.IsNull()) {
    return TimeStamp::Now();
  }
  return sFrameTime;
}

class MOZ_STACK_CLASS StateChangeNotificationBlocker {
public:
  explicit StateChangeNotificationBlocker(AsyncPanZoomController* aApzc)
    : mApzc(aApzc)
  {
    ReentrantMonitorAutoEnter lock(mApzc->mMonitor);
    mInitialState = mApzc->mState;
    mApzc->mNotificationBlockers++;
  }

  ~StateChangeNotificationBlocker()
  {
    AsyncPanZoomController::PanZoomState newState;
    {
      ReentrantMonitorAutoEnter lock(mApzc->mMonitor);
      mApzc->mNotificationBlockers--;
      newState = mApzc->mState;
    }
    mApzc->DispatchStateChangeNotification(mInitialState, newState);
  }

private:
  AsyncPanZoomController* mApzc;
  AsyncPanZoomController::PanZoomState mInitialState;
};

class FlingAnimation: public AsyncPanZoomAnimation {
public:
  FlingAnimation(AsyncPanZoomController& aApzc,
                 const nsRefPtr<const OverscrollHandoffChain>& aOverscrollHandoffChain,
                 bool aApplyAcceleration)
    : AsyncPanZoomAnimation(TimeDuration::FromMilliseconds(gfxPrefs::APZFlingRepaintInterval()))
    , mApzc(aApzc)
    , mOverscrollHandoffChain(aOverscrollHandoffChain)
  {
    MOZ_ASSERT(mOverscrollHandoffChain);
    TimeStamp now = AsyncPanZoomController::GetFrameTime();

    
    
    
    {
      ReentrantMonitorAutoEnter lock(mApzc.mMonitor);
      if (!mApzc.mX.CanScroll()) {
        mApzc.mX.SetVelocity(0);
      }
      if (!mApzc.mY.CanScroll()) {
        mApzc.mY.SetVelocity(0);
      }
    }

    ParentLayerPoint velocity = mApzc.GetVelocityVector();

    
    
    
    
    
    
    
    if (aApplyAcceleration && !mApzc.mLastFlingTime.IsNull()
        && (now - mApzc.mLastFlingTime).ToMilliseconds() < gfxPrefs::APZFlingAccelInterval()) {
      if (SameDirection(velocity.x, mApzc.mLastFlingVelocity.x)) {
        velocity.x = Accelerate(velocity.x, mApzc.mLastFlingVelocity.x);
        APZC_LOG("%p Applying fling x-acceleration from %f to %f (delta %f)\n",
                 &mApzc, mApzc.mX.GetVelocity(), velocity.x, mApzc.mLastFlingVelocity.x);
        mApzc.mX.SetVelocity(velocity.x);
      }
      if (SameDirection(velocity.y, mApzc.mLastFlingVelocity.y)) {
        velocity.y = Accelerate(velocity.y, mApzc.mLastFlingVelocity.y);
        APZC_LOG("%p Applying fling y-acceleration from %f to %f (delta %f)\n",
                 &mApzc, mApzc.mY.GetVelocity(), velocity.y, mApzc.mLastFlingVelocity.y);
        mApzc.mY.SetVelocity(velocity.y);
      }
    }

    mApzc.mLastFlingTime = now;
    mApzc.mLastFlingVelocity = velocity;
  }

  





  virtual bool DoSample(FrameMetrics& aFrameMetrics,
                        const TimeDuration& aDelta) override
  {
    float friction = gfxPrefs::APZFlingFriction();
    float threshold = gfxPrefs::APZFlingStoppedThreshold();

    bool shouldContinueFlingX = mApzc.mX.FlingApplyFrictionOrCancel(aDelta, friction, threshold),
         shouldContinueFlingY = mApzc.mY.FlingApplyFrictionOrCancel(aDelta, friction, threshold);
    
    if (!shouldContinueFlingX && !shouldContinueFlingY) {
      APZC_LOG("%p ending fling animation. overscrolled=%d\n", &mApzc, mApzc.IsOverscrolled());
      
      
      
      
      
      
      
      mDeferredTasks.append(NewRunnableMethod(mOverscrollHandoffChain.get(),
                                              &OverscrollHandoffChain::SnapBackOverscrolledApzc,
                                              &mApzc));
      return false;
    }

    
    
    
    
    ParentLayerPoint velocity = mApzc.GetVelocityVector();

    ParentLayerPoint offset = velocity * aDelta.ToMilliseconds();

    
    
    
    
    ParentLayerPoint overscroll;
    ParentLayerPoint adjustedOffset;
    mApzc.mX.AdjustDisplacement(offset.x, adjustedOffset.x, overscroll.x);
    mApzc.mY.AdjustDisplacement(offset.y, adjustedOffset.y, overscroll.y);

    aFrameMetrics.ScrollBy(adjustedOffset / aFrameMetrics.GetZoom());

    
    if (!IsZero(overscroll)) {
      

      
      
      
      
      
      if (FuzzyEqualsAdditive(overscroll.x, 0.0f, COORDINATE_EPSILON)) {
        velocity.x = 0;
      } else if (FuzzyEqualsAdditive(overscroll.y, 0.0f, COORDINATE_EPSILON)) {
        velocity.y = 0;
      }

      
      
      
      
      
      
      
      
      
      
      
      
      mDeferredTasks.append(NewRunnableMethod(&mApzc,
                                              &AsyncPanZoomController::HandleFlingOverscroll,
                                              velocity,
                                              mOverscrollHandoffChain));

      return false;
    }

    return true;
  }

private:
  static bool SameDirection(float aVelocity1, float aVelocity2)
  {
    return (aVelocity1 == 0.0f)
        || (aVelocity2 == 0.0f)
        || (IsNegative(aVelocity1) == IsNegative(aVelocity2));
  }

  static float Accelerate(float aBase, float aSupplemental)
  {
    return (aBase * gfxPrefs::APZFlingAccelBaseMultiplier())
         + (aSupplemental * gfxPrefs::APZFlingAccelSupplementalMultiplier());
  }

  AsyncPanZoomController& mApzc;
  nsRefPtr<const OverscrollHandoffChain> mOverscrollHandoffChain;
};

class ZoomAnimation: public AsyncPanZoomAnimation {
public:
  ZoomAnimation(CSSPoint aStartOffset, CSSToParentLayerScale2D aStartZoom,
                CSSPoint aEndOffset, CSSToParentLayerScale2D aEndZoom)
    : mTotalDuration(TimeDuration::FromMilliseconds(gfxPrefs::APZZoomAnimationDuration()))
    , mStartOffset(aStartOffset)
    , mStartZoom(aStartZoom)
    , mEndOffset(aEndOffset)
    , mEndZoom(aEndZoom)
  {}

  virtual bool DoSample(FrameMetrics& aFrameMetrics,
                        const TimeDuration& aDelta) override
  {
    mDuration += aDelta;
    double animPosition = mDuration / mTotalDuration;

    if (animPosition >= 1.0) {
      aFrameMetrics.SetZoom(mEndZoom);
      aFrameMetrics.SetScrollOffset(mEndOffset);
      return false;
    }

    
    
    float sampledPosition = gZoomAnimationFunction->GetValue(animPosition);

    
    
    aFrameMetrics.SetZoom(CSSToParentLayerScale2D(
      1 / (sampledPosition / mEndZoom.xScale + (1 - sampledPosition) / mStartZoom.xScale),
      1 / (sampledPosition / mEndZoom.yScale + (1 - sampledPosition) / mStartZoom.yScale)));

    aFrameMetrics.SetScrollOffset(CSSPoint::FromUnknownPoint(gfx::Point(
      mEndOffset.x * sampledPosition + mStartOffset.x * (1 - sampledPosition),
      mEndOffset.y * sampledPosition + mStartOffset.y * (1 - sampledPosition)
    )));

    return true;
  }

private:
  TimeDuration mDuration;
  const TimeDuration mTotalDuration;

  
  
  
  
  CSSPoint mStartOffset;
  CSSToParentLayerScale2D mStartZoom;

  
  
  
  CSSPoint mEndOffset;
  CSSToParentLayerScale2D mEndZoom;
};

class OverscrollAnimation: public AsyncPanZoomAnimation {
public:
  explicit OverscrollAnimation(AsyncPanZoomController& aApzc, const ParentLayerPoint& aVelocity)
    : mApzc(aApzc)
  {
    mApzc.mX.SetVelocity(aVelocity.x);
    mApzc.mY.SetVelocity(aVelocity.y);
  }

  virtual bool DoSample(FrameMetrics& aFrameMetrics,
                        const TimeDuration& aDelta) override
  {
    
    bool continueX = mApzc.mX.SampleOverscrollAnimation(aDelta);
    bool continueY = mApzc.mY.SampleOverscrollAnimation(aDelta);
    return continueX || continueY;
  }
private:
  AsyncPanZoomController& mApzc;
};

class SmoothScrollAnimation : public AsyncPanZoomAnimation {
public:
  SmoothScrollAnimation(AsyncPanZoomController& aApzc,
                        ScrollSource aSource,
                        const nsPoint &aInitialPosition,
                        const nsPoint &aInitialVelocity,
                        const nsPoint& aDestination, double aSpringConstant,
                        double aDampingRatio)
   : AsyncPanZoomAnimation(TimeDuration::FromMilliseconds(
                           gfxPrefs::APZSmoothScrollRepaintInterval()))
   , mApzc(aApzc)
   , mXAxisModel(aInitialPosition.x, aDestination.x, aInitialVelocity.x,
                 aSpringConstant, aDampingRatio)
   , mYAxisModel(aInitialPosition.y, aDestination.y, aInitialVelocity.y,
                 aSpringConstant, aDampingRatio)
  {
  }

  





  bool DoSample(FrameMetrics& aFrameMetrics, const TimeDuration& aDelta) {
    if (mXAxisModel.IsFinished() && mYAxisModel.IsFinished()) {
      return false;
    }

    mXAxisModel.Simulate(aDelta);
    mYAxisModel.Simulate(aDelta);

    CSSPoint position = CSSPoint::FromAppUnits(nsPoint(mXAxisModel.GetPosition(),
                                                       mYAxisModel.GetPosition()));
    CSSPoint css_velocity = CSSPoint::FromAppUnits(nsPoint(mXAxisModel.GetVelocity(),
                                                           mYAxisModel.GetVelocity()));

    
    ParentLayerPoint velocity = ParentLayerPoint(css_velocity.x, css_velocity.y) / 1000.0f;

    
    
    if (mXAxisModel.IsFinished()) {
      mApzc.mX.SetVelocity(0);
    } else {
      mApzc.mX.SetVelocity(velocity.x);
    }
    if (mYAxisModel.IsFinished()) {
      mApzc.mY.SetVelocity(0);
    } else {
      mApzc.mY.SetVelocity(velocity.y);
    }
    
    
    CSSToParentLayerScale2D zoom = aFrameMetrics.GetZoom();
    ParentLayerPoint displacement = (position - aFrameMetrics.GetScrollOffset()) * zoom;

    ParentLayerPoint overscroll;
    ParentLayerPoint adjustedOffset;
    mApzc.mX.AdjustDisplacement(displacement.x, adjustedOffset.x, overscroll.x);
    mApzc.mY.AdjustDisplacement(displacement.y, adjustedOffset.y, overscroll.y);

    aFrameMetrics.ScrollBy(adjustedOffset / zoom);

    
    
    
    
    if (!IsZero(overscroll)) {
      
      

      
      
      
      if (FuzzyEqualsAdditive(overscroll.x, 0.0f, COORDINATE_EPSILON)) {
        velocity.x = 0;
      } else if (FuzzyEqualsAdditive(overscroll.y, 0.0f, COORDINATE_EPSILON)) {
        velocity.y = 0;
      }

      
      
      
      
      
      
      
      
      
      
      
      
      mDeferredTasks.append(NewRunnableMethod(&mApzc,
                                              &AsyncPanZoomController::HandleSmoothScrollOverscroll,
                                              velocity));

      return false;
    }

    return true;
  }
private:
  AsyncPanZoomController& mApzc;
  AxisPhysicsMSDModel mXAxisModel, mYAxisModel;
};

void
AsyncPanZoomController::SetFrameTime(const TimeStamp& aTime) {
  sFrameTime = aTime;
}

 void
AsyncPanZoomController::InitializeGlobalState()
{
  MOZ_ASSERT(NS_IsMainThread());

  static bool sInitialized = false;
  if (sInitialized)
    return;
  sInitialized = true;

  gZoomAnimationFunction = new ComputedTimingFunction();
  gZoomAnimationFunction->Init(
    nsTimingFunction(NS_STYLE_TRANSITION_TIMING_FUNCTION_EASE));
  ClearOnShutdown(&gZoomAnimationFunction);
  gVelocityCurveFunction = new ComputedTimingFunction();
  gVelocityCurveFunction->Init(
    nsTimingFunction(gfxPrefs::APZCurveFunctionX1(),
                     gfxPrefs::APZCurveFunctionY2(),
                     gfxPrefs::APZCurveFunctionX2(),
                     gfxPrefs::APZCurveFunctionY2()));
  ClearOnShutdown(&gVelocityCurveFunction);
}

AsyncPanZoomController::AsyncPanZoomController(uint64_t aLayersId,
                                               APZCTreeManager* aTreeManager,
                                               const nsRefPtr<InputQueue>& aInputQueue,
                                               GeckoContentController* aGeckoContentController,
                                               GestureBehavior aGestures)
  :  mLayersId(aLayersId),
     mPaintThrottler(GetFrameTime(), TimeDuration::FromMilliseconds(500)),
     mGeckoContentController(aGeckoContentController),
     mRefPtrMonitor("RefPtrMonitor"),
     mSharingFrameMetricsAcrossProcesses(false),
     mMonitor("AsyncPanZoomController"),
     mX(this),
     mY(this),
     mPanDirRestricted(false),
     mZoomConstraints(false, false, MIN_ZOOM, MAX_ZOOM),
     mLastSampleTime(GetFrameTime()),
     mLastAsyncScrollTime(GetFrameTime()),
     mLastAsyncScrollOffset(0, 0),
     mCurrentAsyncScrollOffset(0, 0),
     mAsyncScrollTimeoutTask(nullptr),
     mState(NOTHING),
     mNotificationBlockers(0),
     mInputQueue(aInputQueue),
     mTreeManager(aTreeManager),
     mAPZCId(sAsyncPanZoomControllerCount++),
     mSharedLock(nullptr),
     mAsyncTransformAppliedToContent(false)
{
  if (aGestures == USE_GESTURE_DETECTOR) {
    mGestureEventListener = new GestureEventListener(this);
  }
}

AsyncPanZoomController::~AsyncPanZoomController()
{
  MOZ_ASSERT(IsDestroyed());
}

PCompositorParent*
AsyncPanZoomController::GetSharedFrameMetricsCompositor()
{
  APZThreadUtils::AssertOnCompositorThread();

  if (mSharingFrameMetricsAcrossProcesses) {
    const CompositorParent::LayerTreeState* state = CompositorParent::GetIndirectShadowTree(mLayersId);
    
    return state ? state->mCrossProcessParent : nullptr;
  }
  return mCompositorParent.get();
}

already_AddRefed<GeckoContentController>
AsyncPanZoomController::GetGeckoContentController() const {
  MonitorAutoLock lock(mRefPtrMonitor);
  nsRefPtr<GeckoContentController> controller = mGeckoContentController;
  return controller.forget();
}

already_AddRefed<GestureEventListener>
AsyncPanZoomController::GetGestureEventListener() const {
  MonitorAutoLock lock(mRefPtrMonitor);
  nsRefPtr<GestureEventListener> listener = mGestureEventListener;
  return listener.forget();
}

const nsRefPtr<InputQueue>&
AsyncPanZoomController::GetInputQueue() const {
  return mInputQueue;
}

void
AsyncPanZoomController::Destroy()
{
  APZThreadUtils::AssertOnCompositorThread();

  CancelAnimation();

  { 
    MonitorAutoLock lock(mRefPtrMonitor);
    mGeckoContentController = nullptr;
    mGestureEventListener = nullptr;
  }
  mParent = nullptr;
  mTreeManager = nullptr;

  PCompositorParent* compositor = GetSharedFrameMetricsCompositor();
  
  if (compositor && mSharedFrameMetricsBuffer) {
    unused << compositor->SendReleaseSharedCompositorFrameMetrics(mFrameMetrics.GetScrollId(), mAPZCId);
  }

  { 
    ReentrantMonitorAutoEnter lock(mMonitor);
    mSharedFrameMetricsBuffer = nullptr;
    delete mSharedLock;
    mSharedLock = nullptr;
  }
}

bool
AsyncPanZoomController::IsDestroyed() const
{
  return mTreeManager == nullptr;
}

ScreenCoord
AsyncPanZoomController::GetTouchStartTolerance()
{
  return (gfxPrefs::APZTouchStartTolerance() * APZCTreeManager::GetDPI());
}

AsyncPanZoomController::AxisLockMode AsyncPanZoomController::GetAxisLockMode()
{
  return static_cast<AxisLockMode>(gfxPrefs::APZAxisLockMode());
}

bool
AsyncPanZoomController::ArePointerEventsConsumable(TouchBlockState* aBlock, uint32_t aTouchPoints) {
  if (aTouchPoints == 0) {
    
    return false;
  }

  
  
  
  
  
  

  bool pannable = aBlock->GetOverscrollHandoffChain()->CanBePanned(this);
  bool zoomable = mZoomConstraints.mAllowZoom;

  pannable &= (aBlock->TouchActionAllowsPanningX() || aBlock->TouchActionAllowsPanningY());
  zoomable &= (aBlock->TouchActionAllowsPinchZoom());

  
  
  bool consumable = (aTouchPoints == 1 ? pannable : zoomable);
  if (!consumable) {
    return false;
  }

  return true;
}

nsEventStatus AsyncPanZoomController::HandleInputEvent(const InputData& aEvent,
                                                       const Matrix4x4& aTransformToApzc) {
  APZThreadUtils::AssertOnControllerThread();

  nsEventStatus rv = nsEventStatus_eIgnore;

  switch (aEvent.mInputType) {
  case MULTITOUCH_INPUT: {
    MultiTouchInput multiTouchInput = aEvent.AsMultiTouchInput();
    multiTouchInput.TransformToLocal(aTransformToApzc);

    nsRefPtr<GestureEventListener> listener = GetGestureEventListener();
    if (listener) {
      rv = listener->HandleInputEvent(multiTouchInput);
      if (rv == nsEventStatus_eConsumeNoDefault) {
        return rv;
      }
    }

    switch (multiTouchInput.mType) {
      case MultiTouchInput::MULTITOUCH_START: rv = OnTouchStart(multiTouchInput); break;
      case MultiTouchInput::MULTITOUCH_MOVE: rv = OnTouchMove(multiTouchInput); break;
      case MultiTouchInput::MULTITOUCH_END: rv = OnTouchEnd(multiTouchInput); break;
      case MultiTouchInput::MULTITOUCH_CANCEL: rv = OnTouchCancel(multiTouchInput); break;
      default: NS_WARNING("Unhandled multitouch"); break;
    }
    break;
  }
  case PANGESTURE_INPUT: {
    PanGestureInput panGestureInput = aEvent.AsPanGestureInput();
    panGestureInput.TransformToLocal(aTransformToApzc);

    switch (panGestureInput.mType) {
      case PanGestureInput::PANGESTURE_MAYSTART: rv = OnPanMayBegin(panGestureInput); break;
      case PanGestureInput::PANGESTURE_CANCELLED: rv = OnPanCancelled(panGestureInput); break;
      case PanGestureInput::PANGESTURE_START: rv = OnPanBegin(panGestureInput); break;
      case PanGestureInput::PANGESTURE_PAN: rv = OnPan(panGestureInput, ScrollSource::Touch, true); break;
      case PanGestureInput::PANGESTURE_END: rv = OnPanEnd(panGestureInput); break;
      case PanGestureInput::PANGESTURE_MOMENTUMSTART: rv = OnPanMomentumStart(panGestureInput); break;
      case PanGestureInput::PANGESTURE_MOMENTUMPAN: rv = OnPan(panGestureInput, ScrollSource::Touch, false); break;
      case PanGestureInput::PANGESTURE_MOMENTUMEND: rv = OnPanMomentumEnd(panGestureInput); break;
      default: NS_WARNING("Unhandled pan gesture"); break;
    }
    break;
  }
  case SCROLLWHEEL_INPUT: {
    ScrollWheelInput scrollInput = aEvent.AsScrollWheelInput();
    scrollInput.TransformToLocal(aTransformToApzc);

    rv = OnScrollWheel(scrollInput);
    break;
  }
  case PINCHGESTURE_INPUT: {
    PinchGestureInput pinchInput = aEvent.AsPinchGestureInput();
    pinchInput.TransformToLocal(aTransformToApzc);

    rv = HandleGestureEvent(pinchInput);
    break;
  }
  case TAPGESTURE_INPUT: {
    TapGestureInput tapInput = aEvent.AsTapGestureInput();
    tapInput.TransformToLocal(aTransformToApzc);

    rv = HandleGestureEvent(tapInput);
    break;
  }
  default: NS_WARNING("Unhandled input event type"); break;
  }

  return rv;
}

nsEventStatus AsyncPanZoomController::HandleGestureEvent(const InputData& aEvent)
{
  APZThreadUtils::AssertOnControllerThread();

  nsEventStatus rv = nsEventStatus_eIgnore;

  switch (aEvent.mInputType) {
  case PINCHGESTURE_INPUT: {
    const PinchGestureInput& pinchGestureInput = aEvent.AsPinchGestureInput();
    switch (pinchGestureInput.mType) {
      case PinchGestureInput::PINCHGESTURE_START: rv = OnScaleBegin(pinchGestureInput); break;
      case PinchGestureInput::PINCHGESTURE_SCALE: rv = OnScale(pinchGestureInput); break;
      case PinchGestureInput::PINCHGESTURE_END: rv = OnScaleEnd(pinchGestureInput); break;
      default: NS_WARNING("Unhandled pinch gesture"); break;
    }
    break;
  }
  case TAPGESTURE_INPUT: {
    const TapGestureInput& tapGestureInput = aEvent.AsTapGestureInput();
    switch (tapGestureInput.mType) {
      case TapGestureInput::TAPGESTURE_LONG: rv = OnLongPress(tapGestureInput); break;
      case TapGestureInput::TAPGESTURE_LONG_UP: rv = OnLongPressUp(tapGestureInput); break;
      case TapGestureInput::TAPGESTURE_UP: rv = OnSingleTapUp(tapGestureInput); break;
      case TapGestureInput::TAPGESTURE_CONFIRMED: rv = OnSingleTapConfirmed(tapGestureInput); break;
      case TapGestureInput::TAPGESTURE_DOUBLE: rv = OnDoubleTap(tapGestureInput); break;
      case TapGestureInput::TAPGESTURE_CANCEL: rv = OnCancelTap(tapGestureInput); break;
      default: NS_WARNING("Unhandled tap gesture"); break;
    }
    break;
  }
  default: NS_WARNING("Unhandled input event"); break;
  }

  return rv;
}

nsEventStatus AsyncPanZoomController::OnTouchStart(const MultiTouchInput& aEvent) {
  APZC_LOG("%p got a touch-start in state %d\n", this, mState);
  mPanDirRestricted = false;
  ParentLayerPoint point = GetFirstTouchPoint(aEvent);

  switch (mState) {
    case FLING:
    case ANIMATING_ZOOM:
    case SMOOTH_SCROLL:
    case OVERSCROLL_ANIMATION:
    case WHEEL_SCROLL:
      CurrentTouchBlock()->GetOverscrollHandoffChain()->CancelAnimations(ExcludeOverscroll);
      
    case NOTHING: {
      mX.StartTouch(point.x, aEvent.mTime);
      mY.StartTouch(point.y, aEvent.mTime);
      if (nsRefPtr<GeckoContentController> controller = GetGeckoContentController()) {
        controller->NotifyAPZStateChange(
            GetGuid(), APZStateChange::StartTouch,
            CurrentTouchBlock()->GetOverscrollHandoffChain()->CanBePanned(this));
      }
      SetState(TOUCHING);
      break;
    }
    case TOUCHING:
    case PANNING:
    case PANNING_LOCKED_X:
    case PANNING_LOCKED_Y:
    case CROSS_SLIDING_X:
    case CROSS_SLIDING_Y:
    case PINCHING:
      NS_WARNING("Received impossible touch in OnTouchStart");
      break;
    default:
      NS_WARNING("Unhandled case in OnTouchStart");
      break;
  }

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnTouchMove(const MultiTouchInput& aEvent) {
  APZC_LOG("%p got a touch-move in state %d\n", this, mState);
  switch (mState) {
    case FLING:
    case SMOOTH_SCROLL:
    case NOTHING:
    case ANIMATING_ZOOM:
      
      
      return nsEventStatus_eIgnore;

    case CROSS_SLIDING_X:
    case CROSS_SLIDING_Y:
      
      
      return nsEventStatus_eIgnore;

    case TOUCHING: {
      ScreenCoord panThreshold = GetTouchStartTolerance();
      UpdateWithTouchAtDevicePoint(aEvent);

      if (PanDistance() < panThreshold) {
        return nsEventStatus_eIgnore;
      }

      if (gfxPrefs::TouchActionEnabled() && CurrentTouchBlock()->TouchActionAllowsPanningXY()) {
        
        
        
        
        StartPanning(aEvent);
        return nsEventStatus_eConsumeNoDefault;
      }

      return StartPanning(aEvent);
    }

    case PANNING:
    case PANNING_LOCKED_X:
    case PANNING_LOCKED_Y:
      TrackTouch(aEvent);
      return nsEventStatus_eConsumeNoDefault;

    case PINCHING:
      
      NS_WARNING("Gesture listener should have handled pinching in OnTouchMove.");
      return nsEventStatus_eIgnore;

    case WHEEL_SCROLL:
    case OVERSCROLL_ANIMATION:
      
      
      
      NS_WARNING("Received impossible touch in OnTouchMove");
      break;
  }

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnTouchEnd(const MultiTouchInput& aEvent) {
  APZC_LOG("%p got a touch-end in state %d\n", this, mState);

  OnTouchEndOrCancel();

  
  
  
  
  
  if (mState != NOTHING) {
    ReentrantMonitorAutoEnter lock(mMonitor);
    SendAsyncScrollEvent();
  }

  switch (mState) {
  case FLING:
    
    NS_WARNING("Received impossible touch end in OnTouchEnd.");
    
  case ANIMATING_ZOOM:
  case SMOOTH_SCROLL:
  case NOTHING:
    
    
    return nsEventStatus_eIgnore;

  case TOUCHING:
  case CROSS_SLIDING_X:
  case CROSS_SLIDING_Y:
    
    
    mX.SetVelocity(0);
    mY.SetVelocity(0);
    
    
    if (!SnapBackIfOverscrolled()) {
      SetState(NOTHING);
    }
    return nsEventStatus_eIgnore;

  case PANNING:
  case PANNING_LOCKED_X:
  case PANNING_LOCKED_Y:
  {
    CurrentTouchBlock()->GetOverscrollHandoffChain()->FlushRepaints();
    mX.EndTouch(aEvent.mTime);
    mY.EndTouch(aEvent.mTime);
    ParentLayerPoint flingVelocity = GetVelocityVector();
    
    
    
    mX.SetVelocity(0);
    mY.SetVelocity(0);
    
    
    
    
    StateChangeNotificationBlocker blocker(this);
    SetState(NOTHING);
    APZC_LOG("%p starting a fling animation\n", this);
    
    
    
    if (APZCTreeManager* treeManagerLocal = GetApzcTreeManager()) {
      treeManagerLocal->DispatchFling(this,
                                      flingVelocity,
                                      CurrentTouchBlock()->GetOverscrollHandoffChain(),
                                      false );
    }
    return nsEventStatus_eConsumeNoDefault;
  }
  case PINCHING:
    SetState(NOTHING);
    
    NS_WARNING("Gesture listener should have handled pinching in OnTouchEnd.");
    return nsEventStatus_eIgnore;

  case WHEEL_SCROLL:
  case OVERSCROLL_ANIMATION:
    
    
    
    NS_WARNING("Received impossible touch in OnTouchEnd");
    break;
  }

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnTouchCancel(const MultiTouchInput& aEvent) {
  APZC_LOG("%p got a touch-cancel in state %d\n", this, mState);
  OnTouchEndOrCancel();
  mX.CancelTouch();
  mY.CancelTouch();
  CancelAnimation();
  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnScaleBegin(const PinchGestureInput& aEvent) {
  APZC_LOG("%p got a scale-begin in state %d\n", this, mState);

  
  
  if (HasReadyTouchBlock() && !CurrentTouchBlock()->TouchActionAllowsPinchZoom()) {
    return nsEventStatus_eIgnore;
  }

  if (!mZoomConstraints.mAllowZoom) {
    return nsEventStatus_eConsumeNoDefault;
  }

  SetState(PINCHING);
  mX.SetVelocity(0);
  mY.SetVelocity(0);
  mLastZoomFocus = aEvent.mLocalFocusPoint - mFrameMetrics.mCompositionBounds.TopLeft();

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnScale(const PinchGestureInput& aEvent) {
  APZC_LOG("%p got a scale in state %d\n", this, mState);

  if (HasReadyTouchBlock() && !CurrentTouchBlock()->TouchActionAllowsPinchZoom()) {
    return nsEventStatus_eIgnore;
  }

  if (mState != PINCHING) {
    return nsEventStatus_eConsumeNoDefault;
  }

  
  
  
  
  
  MOZ_ASSERT(mFrameMetrics.IsRootScrollable());
  MOZ_ASSERT(mFrameMetrics.GetZoom().AreScalesSame());

  float prevSpan = aEvent.mPreviousSpan;
  if (fabsf(prevSpan) <= EPSILON || fabsf(aEvent.mCurrentSpan) <= EPSILON) {
    
    return nsEventStatus_eConsumeNoDefault;
  }

  float spanRatio = aEvent.mCurrentSpan / aEvent.mPreviousSpan;

  {
    ReentrantMonitorAutoEnter lock(mMonitor);

    CSSToParentLayerScale userZoom = mFrameMetrics.GetZoom().ToScaleFactor();
    ParentLayerPoint focusPoint = aEvent.mLocalFocusPoint - mFrameMetrics.mCompositionBounds.TopLeft();
    CSSPoint cssFocusPoint = focusPoint / mFrameMetrics.GetZoom();

    CSSPoint focusChange = (mLastZoomFocus - focusPoint) / userZoom;
    
    
    focusChange.x -= mX.DisplacementWillOverscrollAmount(focusChange.x);
    focusChange.y -= mY.DisplacementWillOverscrollAmount(focusChange.y);
    ScrollBy(focusChange);

    
    
    
    CSSPoint neededDisplacement;

    CSSToParentLayerScale realMinZoom = mZoomConstraints.mMinZoom;
    CSSToParentLayerScale realMaxZoom = mZoomConstraints.mMaxZoom;
    realMinZoom.scale = std::max(realMinZoom.scale,
                                 mFrameMetrics.mCompositionBounds.width / mFrameMetrics.GetScrollableRect().width);
    realMinZoom.scale = std::max(realMinZoom.scale,
                                 mFrameMetrics.mCompositionBounds.height / mFrameMetrics.GetScrollableRect().height);
    if (realMaxZoom < realMinZoom) {
      realMaxZoom = realMinZoom;
    }

    bool doScale = (spanRatio > 1.0 && userZoom < realMaxZoom) ||
                   (spanRatio < 1.0 && userZoom > realMinZoom);

    if (doScale) {
      spanRatio = clamped(spanRatio,
                          realMinZoom.scale / userZoom.scale,
                          realMaxZoom.scale / userZoom.scale);

      
      
      neededDisplacement.x = -mX.ScaleWillOverscrollAmount(spanRatio, cssFocusPoint.x);
      neededDisplacement.y = -mY.ScaleWillOverscrollAmount(spanRatio, cssFocusPoint.y);

      ScaleWithFocus(spanRatio, cssFocusPoint);

      if (neededDisplacement != CSSPoint()) {
        ScrollBy(neededDisplacement);
      }

      ScheduleComposite();
      
      
      UpdateSharedCompositorFrameMetrics();
    }

    mLastZoomFocus = focusPoint;
  }

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnScaleEnd(const PinchGestureInput& aEvent) {
  APZC_LOG("%p got a scale-end in state %d\n", this, mState);

  if (HasReadyTouchBlock() && !CurrentTouchBlock()->TouchActionAllowsPinchZoom()) {
    return nsEventStatus_eIgnore;
  }

  SetState(NOTHING);

  {
    ReentrantMonitorAutoEnter lock(mMonitor);

    
    
    
    
    
    
    
    
    if (HasReadyTouchBlock()) {
      CurrentTouchBlock()->GetOverscrollHandoffChain()->ClearOverscroll();
    } else {
      ClearOverscroll();
    }

    ScheduleComposite();
    RequestContentRepaint();
    UpdateSharedCompositorFrameMetrics();
  }

  return nsEventStatus_eConsumeNoDefault;
}

bool
AsyncPanZoomController::ConvertToGecko(const ParentLayerPoint& aPoint, CSSPoint* aOut)
{
  if (APZCTreeManager* treeManagerLocal = GetApzcTreeManager()) {
    Matrix4x4 transformToGecko = treeManagerLocal->GetApzcToGeckoTransform(this);
    
    
    LayoutDevicePoint layoutPoint = TransformTo<LayoutDevicePixel>(transformToGecko, aPoint);
    { 
      ReentrantMonitorAutoEnter lock(mMonitor);
      *aOut = layoutPoint / mFrameMetrics.GetDevPixelsPerCSSPixel();
    }
    return true;
  }
  return false;
}

LayoutDevicePoint
AsyncPanZoomController::GetScrollWheelDelta(const ScrollWheelInput& aEvent) const
{
  ReentrantMonitorAutoEnter lock(mMonitor);

  LayoutDevicePoint delta(aEvent.mDeltaX, aEvent.mDeltaY);
  switch (aEvent.mDeltaType) {
    case ScrollWheelInput::SCROLLDELTA_LINE: {
      LayoutDeviceIntSize scrollAmount = mFrameMetrics.GetLineScrollAmount();
      delta.x *= scrollAmount.width;
      delta.y *= scrollAmount.height;
      break;
    }
    default:
      MOZ_ASSERT_UNREACHABLE("unexpected scroll delta type");
  }

  if (gfxPrefs::MouseWheelHasRootScrollDeltaOverride()) {
    
    double hfactor = double(gfxPrefs::MouseWheelRootHScrollDeltaFactor()) / 100;
    double vfactor = double(gfxPrefs::MouseWheelRootVScrollDeltaFactor()) / 100;
    if (vfactor > 1.0) {
      delta.x *= hfactor;
    }
    if (hfactor > 1.0) {
      delta.y *= vfactor;
    }
  }

  LayoutDeviceIntSize pageScrollSize = mFrameMetrics.GetPageScrollAmount();
  if (Abs(delta.x) > pageScrollSize.width) {
    delta.x = (delta.x >= 0)
              ? pageScrollSize.width
              : -pageScrollSize.width;
  }
  if (Abs(delta.y) > pageScrollSize.height) {
    delta.y = (delta.y >= 0)
              ? pageScrollSize.height
              : -pageScrollSize.height;
  }

  return delta;
}


bool
AsyncPanZoomController::CanScroll(const ScrollWheelInput& aEvent) const
{
  LayoutDevicePoint delta = GetScrollWheelDelta(aEvent);
  if (!delta.x && !delta.y) {
    return false;
  }

  return CanScroll(delta.x, delta.y);
}

bool
AsyncPanZoomController::CanScroll(double aDeltaX, double aDeltaY) const
{
  ReentrantMonitorAutoEnter lock(mMonitor);
  return mX.CanScroll(aDeltaX) || mY.CanScroll(aDeltaY);
}

bool
AsyncPanZoomController::AllowScrollHandoffInWheelTransaction() const
{
  WheelBlockState* block = mInputQueue->CurrentWheelBlock();
  return block->AllowScrollHandoff();
}

nsEventStatus AsyncPanZoomController::OnScrollWheel(const ScrollWheelInput& aEvent)
{
  LayoutDevicePoint delta = GetScrollWheelDelta(aEvent);

  if ((delta.x || delta.y) &&
      !CanScroll(delta.x, delta.y) &&
      mInputQueue->GetCurrentWheelTransaction())
  {
    
    if (gfxPrefs::MouseScrollTestingEnabled()) {
      if (nsRefPtr<GeckoContentController> controller = GetGeckoContentController()) {
        controller->NotifyMozMouseScrollEvent(
          mFrameMetrics.GetScrollId(),
          NS_LITERAL_STRING("MozMouseScrollFailed"));
      }
    }
    return nsEventStatus_eConsumeNoDefault;
  }

  switch (aEvent.mScrollMode) {
    case ScrollWheelInput::SCROLLMODE_INSTANT: {
      
      PanGestureInput start(PanGestureInput::PANGESTURE_START, aEvent.mTime, aEvent.mTimeStamp,
                            aEvent.mOrigin, ScreenPoint(0, 0), aEvent.modifiers);
      start.mLocalPanStartPoint = aEvent.mLocalOrigin;
      OnPanBegin(start);

      
      
      
      
      ParentLayerPoint panDelta = (-delta / mFrameMetrics.GetDevPixelsPerCSSPixel()) *
                               mFrameMetrics.GetZoom();

      PanGestureInput move(PanGestureInput::PANGESTURE_PAN, aEvent.mTime, aEvent.mTimeStamp,
                           aEvent.mOrigin,
                           ToScreenCoordinates(panDelta, aEvent.mLocalOrigin),
                           aEvent.modifiers);
      move.mLocalPanStartPoint = aEvent.mLocalOrigin;
      move.mLocalPanDisplacement = panDelta;
      OnPan(move, ScrollSource::Wheel, false);

      PanGestureInput end(PanGestureInput::PANGESTURE_END, aEvent.mTime, aEvent.mTimeStamp,
                            aEvent.mOrigin, ScreenPoint(0, 0), aEvent.modifiers);
      end.mLocalPanStartPoint = aEvent.mLocalOrigin;
      OnPanEnd(start);
      break;
    }

    case ScrollWheelInput::SCROLLMODE_SMOOTH: {
      if (mState != WHEEL_SCROLL) {
        CancelAnimation();
        SetState(WHEEL_SCROLL);

        nsPoint initialPosition = CSSPoint::ToAppUnits(mFrameMetrics.GetScrollOffset());
        StartAnimation(new WheelScrollAnimation(
          *this,
          initialPosition));
      }

      
      
      nsPoint deltaInAppUnits =
        CSSPoint::ToAppUnits(delta / mFrameMetrics.GetDevPixelsPerCSSPixel());
      nsPoint velocity =
        CSSPoint::ToAppUnits(CSSPoint(mX.GetVelocity(), mY.GetVelocity())) * 1000.0f;

      WheelScrollAnimation* animation = mAnimation->AsWheelScrollAnimation();
      animation->Update(aEvent.mTimeStamp, deltaInAppUnits, nsSize(velocity.x, velocity.y));
      break;
    }
  }

  return nsEventStatus_eConsumeNoDefault;
}

void
AsyncPanZoomController::NotifyMozMouseScrollEvent(const nsString& aString) const
{
  nsRefPtr<GeckoContentController> controller = GetGeckoContentController();
  if (!controller) {
    return;
  }

  controller->NotifyMozMouseScrollEvent(mFrameMetrics.GetScrollId(), aString);
}

nsEventStatus AsyncPanZoomController::OnPanMayBegin(const PanGestureInput& aEvent) {
  APZC_LOG("%p got a pan-maybegin in state %d\n", this, mState);

  mX.StartTouch(aEvent.mLocalPanStartPoint.x, aEvent.mTime);
  mY.StartTouch(aEvent.mLocalPanStartPoint.y, aEvent.mTime);
  if (mPanGestureState) {
    mPanGestureState->GetOverscrollHandoffChain()->CancelAnimations();
  } else {
    CancelAnimation();
  }

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnPanCancelled(const PanGestureInput& aEvent) {
  APZC_LOG("%p got a pan-cancelled in state %d\n", this, mState);

  mX.CancelTouch();
  mY.CancelTouch();

  return nsEventStatus_eConsumeNoDefault;
}


nsEventStatus AsyncPanZoomController::OnPanBegin(const PanGestureInput& aEvent) {
  APZC_LOG("%p got a pan-begin in state %d\n", this, mState);

  if (mState == SMOOTH_SCROLL) {
    
    CancelAnimation();
  }

  mPanGestureState = MakeUnique<InputBlockState>(this, true);

  mX.StartTouch(aEvent.mLocalPanStartPoint.x, aEvent.mTime);
  mY.StartTouch(aEvent.mLocalPanStartPoint.y, aEvent.mTime);

  if (GetAxisLockMode() == FREE) {
    SetState(PANNING);
    return nsEventStatus_eConsumeNoDefault;
  }

  float dx = aEvent.mPanDisplacement.x, dy = aEvent.mPanDisplacement.y;
  double angle = atan2(dy, dx); 
  angle = fabs(angle); 

  HandlePanning(angle);

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnPan(const PanGestureInput& aEvent, ScrollSource aSource, bool aFingersOnTouchpad) {
  APZC_LOG("%p got a pan-pan in state %d\n", this, mState);

  if (mState == SMOOTH_SCROLL) {
    if (aEvent.mType == PanGestureInput::PANGESTURE_MOMENTUMPAN) {
      
      
      
      
      
      return nsEventStatus_eConsumeNoDefault;
    } else {
      
      CancelAnimation();
    }
  }

  
  
  
  
  mX.UpdateWithTouchAtDevicePoint(aEvent.mLocalPanStartPoint.x, aEvent.mTime);
  mY.UpdateWithTouchAtDevicePoint(aEvent.mLocalPanStartPoint.y, aEvent.mTime);

  HandlePanningUpdate(aEvent.mPanDisplacement);

  
  if (mPanGestureState) {
    ScreenPoint panDistance(fabs(aEvent.mPanDisplacement.x), fabs(aEvent.mPanDisplacement.y));
    OverscrollHandoffState handoffState(
        *mPanGestureState->GetOverscrollHandoffChain(),
        panDistance,
        aSource);
    CallDispatchScroll(aEvent.mLocalPanStartPoint,
                       aEvent.mLocalPanStartPoint + aEvent.mLocalPanDisplacement,
                       handoffState);
  }

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnPanEnd(const PanGestureInput& aEvent) {
  APZC_LOG("%p got a pan-end in state %d\n", this, mState);

  mPanGestureState = nullptr;

  mX.EndTouch(aEvent.mTime);
  mY.EndTouch(aEvent.mTime);
  RequestContentRepaint();

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnPanMomentumStart(const PanGestureInput& aEvent) {
  APZC_LOG("%p got a pan-momentumstart in state %d\n", this, mState);

  if (mState == SMOOTH_SCROLL) {
    
    CancelAnimation();
  }

  mPanGestureState = MakeUnique<InputBlockState>(this, true);

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnPanMomentumEnd(const PanGestureInput& aEvent) {
  APZC_LOG("%p got a pan-momentumend in state %d\n", this, mState);

  mPanGestureState = nullptr;

  
  
  
  mX.CancelTouch();
  mY.CancelTouch();

  RequestContentRepaint();

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnLongPress(const TapGestureInput& aEvent) {
  APZC_LOG("%p got a long-press in state %d\n", this, mState);
  nsRefPtr<GeckoContentController> controller = GetGeckoContentController();
  if (controller) {
    CSSPoint geckoScreenPoint;
    if (ConvertToGecko(aEvent.mLocalPoint, &geckoScreenPoint)) {
      if (CurrentTouchBlock()->IsDuringFastMotion()) {
        APZC_LOG("%p dropping long-press because of fast motion\n", this);
        return nsEventStatus_eIgnore;
      }
      uint64_t blockId = GetInputQueue()->InjectNewTouchBlock(this);
      controller->HandleLongTap(geckoScreenPoint, aEvent.modifiers, GetGuid(), blockId);
      return nsEventStatus_eConsumeNoDefault;
    }
  }
  return nsEventStatus_eIgnore;
}

nsEventStatus AsyncPanZoomController::OnLongPressUp(const TapGestureInput& aEvent) {
  APZC_LOG("%p got a long-tap-up in state %d\n", this, mState);
  return GenerateSingleTap(aEvent.mLocalPoint, aEvent.modifiers);
}

nsEventStatus AsyncPanZoomController::GenerateSingleTap(const ParentLayerPoint& aPoint, mozilla::Modifiers aModifiers) {
  nsRefPtr<GeckoContentController> controller = GetGeckoContentController();
  if (controller) {
    CSSPoint geckoScreenPoint;
    if (ConvertToGecko(aPoint, &geckoScreenPoint)) {
      if (!CurrentTouchBlock()->SetSingleTapOccurred()) {
        return nsEventStatus_eIgnore;
      }
      
      
      
      
      
      controller->PostDelayedTask(
        NewRunnableMethod(controller.get(), &GeckoContentController::HandleSingleTap,
                          geckoScreenPoint, aModifiers,
                          GetGuid()),
        0);
      return nsEventStatus_eConsumeNoDefault;
    }
  }
  return nsEventStatus_eIgnore;
}

void AsyncPanZoomController::OnTouchEndOrCancel() {
  if (nsRefPtr<GeckoContentController> controller = GetGeckoContentController()) {
    controller->NotifyAPZStateChange(
        GetGuid(), APZStateChange::EndTouch, CurrentTouchBlock()->SingleTapOccurred());
  }
}

nsEventStatus AsyncPanZoomController::OnSingleTapUp(const TapGestureInput& aEvent) {
  APZC_LOG("%p got a single-tap-up in state %d\n", this, mState);
  
  
  if (!(mZoomConstraints.mAllowDoubleTapZoom && CurrentTouchBlock()->TouchActionAllowsDoubleTapZoom())) {
    return GenerateSingleTap(aEvent.mLocalPoint, aEvent.modifiers);
  }
  return nsEventStatus_eIgnore;
}

nsEventStatus AsyncPanZoomController::OnSingleTapConfirmed(const TapGestureInput& aEvent) {
  APZC_LOG("%p got a single-tap-confirmed in state %d\n", this, mState);
  return GenerateSingleTap(aEvent.mLocalPoint, aEvent.modifiers);
}

nsEventStatus AsyncPanZoomController::OnDoubleTap(const TapGestureInput& aEvent) {
  APZC_LOG("%p got a double-tap in state %d\n", this, mState);
  nsRefPtr<GeckoContentController> controller = GetGeckoContentController();
  if (controller) {
    if (mZoomConstraints.mAllowDoubleTapZoom && CurrentTouchBlock()->TouchActionAllowsDoubleTapZoom()) {
      CSSPoint geckoScreenPoint;
      if (ConvertToGecko(aEvent.mLocalPoint, &geckoScreenPoint)) {
        controller->HandleDoubleTap(geckoScreenPoint, aEvent.modifiers, GetGuid());
      }
    }
    return nsEventStatus_eConsumeNoDefault;
  }
  return nsEventStatus_eIgnore;
}

nsEventStatus AsyncPanZoomController::OnCancelTap(const TapGestureInput& aEvent) {
  APZC_LOG("%p got a cancel-tap in state %d\n", this, mState);
  
  return nsEventStatus_eIgnore;
}


Matrix4x4 AsyncPanZoomController::GetTransformToThis() const {
  if (APZCTreeManager* treeManagerLocal = GetApzcTreeManager()) {
    return treeManagerLocal->GetScreenToApzcTransform(this);
  }
  return Matrix4x4();
}

ScreenPoint AsyncPanZoomController::ToScreenCoordinates(const ParentLayerPoint& aVector,
                                                        const ParentLayerPoint& aAnchor) const {
  return TransformVector<ScreenPixel>(GetTransformToThis().Inverse(), aVector, aAnchor);
}

ParentLayerPoint AsyncPanZoomController::ToParentLayerCoordinates(const ScreenPoint& aVector,
                                                                  const ScreenPoint& aAnchor) const {
  return TransformVector<ParentLayerPixel>(GetTransformToThis(), aVector, aAnchor);
}

bool AsyncPanZoomController::Contains(const ScreenIntPoint& aPoint) const
{
  Matrix4x4 transformToThis = GetTransformToThis();
  ParentLayerIntPoint point = TransformTo<ParentLayerPixel>(transformToThis, aPoint);

  ParentLayerIntRect cb;
  {
    ReentrantMonitorAutoEnter lock(mMonitor);
    GetFrameMetrics().mCompositionBounds.ToIntRect(&cb);
  }
  return cb.Contains(point);
}

ScreenCoord AsyncPanZoomController::PanDistance() const {
  ParentLayerPoint panVector;
  ParentLayerPoint panStart;
  {
    ReentrantMonitorAutoEnter lock(mMonitor);
    panVector = ParentLayerPoint(mX.PanDistance(), mY.PanDistance());
    panStart = PanStart();
  }
  return ToScreenCoordinates(panVector, panStart).Length();
}

ParentLayerPoint AsyncPanZoomController::PanStart() const {
  return ParentLayerPoint(mX.PanStart(), mY.PanStart());
}

const ParentLayerPoint AsyncPanZoomController::GetVelocityVector() const {
  return ParentLayerPoint(mX.GetVelocity(), mY.GetVelocity());
}

void AsyncPanZoomController::HandlePanningWithTouchAction(double aAngle) {
  
  
  if (CurrentTouchBlock()->TouchActionAllowsPanningXY()) {
    if (mX.CanScrollNow() && mY.CanScrollNow()) {
      if (IsCloseToHorizontal(aAngle, gfxPrefs::APZAxisLockAngle())) {
        mY.SetAxisLocked(true);
        SetState(PANNING_LOCKED_X);
      } else if (IsCloseToVertical(aAngle, gfxPrefs::APZAxisLockAngle())) {
        mX.SetAxisLocked(true);
        SetState(PANNING_LOCKED_Y);
      } else {
        SetState(PANNING);
      }
    } else if (mX.CanScrollNow() || mY.CanScrollNow()) {
      SetState(PANNING);
    } else {
      SetState(NOTHING);
    }
  } else if (CurrentTouchBlock()->TouchActionAllowsPanningX()) {
    
    
    if (IsCloseToHorizontal(aAngle, gfxPrefs::APZAllowedDirectPanAngle())) {
      mY.SetAxisLocked(true);
      SetState(PANNING_LOCKED_X);
      mPanDirRestricted = true;
    } else {
      
      
      SetState(NOTHING);
    }
  } else if (CurrentTouchBlock()->TouchActionAllowsPanningY()) {
    if (IsCloseToVertical(aAngle, gfxPrefs::APZAllowedDirectPanAngle())) {
      mX.SetAxisLocked(true);
      SetState(PANNING_LOCKED_Y);
      mPanDirRestricted = true;
    } else {
      SetState(NOTHING);
    }
  } else {
    SetState(NOTHING);
  }
}

void AsyncPanZoomController::HandlePanning(double aAngle) {
  ReentrantMonitorAutoEnter lock(mMonitor);
  if (!gfxPrefs::APZCrossSlideEnabled() && (!mX.CanScrollNow() || !mY.CanScrollNow())) {
    SetState(PANNING);
  } else if (IsCloseToHorizontal(aAngle, gfxPrefs::APZAxisLockAngle())) {
    mY.SetAxisLocked(true);
    if (mX.CanScrollNow()) {
      SetState(PANNING_LOCKED_X);
    } else {
      SetState(CROSS_SLIDING_X);
      mX.SetAxisLocked(true);
    }
  } else if (IsCloseToVertical(aAngle, gfxPrefs::APZAxisLockAngle())) {
    mX.SetAxisLocked(true);
    if (mY.CanScrollNow()) {
      SetState(PANNING_LOCKED_Y);
    } else {
      SetState(CROSS_SLIDING_Y);
      mY.SetAxisLocked(true);
    }
  } else {
    SetState(PANNING);
  }
}

void AsyncPanZoomController::HandlePanningUpdate(const ScreenPoint& aPanDistance) {
  
  if (GetAxisLockMode() == STICKY && !mPanDirRestricted) {

    double angle = atan2(aPanDistance.y, aPanDistance.x); 
    angle = fabs(angle); 

    float breakThreshold = gfxPrefs::APZAxisBreakoutThreshold() * APZCTreeManager::GetDPI();

    if (fabs(aPanDistance.x) > breakThreshold || fabs(aPanDistance.y) > breakThreshold) {
      if (mState == PANNING_LOCKED_X || mState == CROSS_SLIDING_X) {
        if (!IsCloseToHorizontal(angle, gfxPrefs::APZAxisBreakoutAngle())) {
          mY.SetAxisLocked(false);
          SetState(PANNING);
        }
      } else if (mState == PANNING_LOCKED_Y || mState == CROSS_SLIDING_Y) {
        if (!IsCloseToVertical(angle, gfxPrefs::APZAxisLockAngle())) {
          mX.SetAxisLocked(false);
          SetState(PANNING);
        }
      }
    }
  }
}

nsEventStatus AsyncPanZoomController::StartPanning(const MultiTouchInput& aEvent) {
  ReentrantMonitorAutoEnter lock(mMonitor);

  ParentLayerPoint point = GetFirstTouchPoint(aEvent);
  float dx = mX.PanDistance(point.x);
  float dy = mY.PanDistance(point.y);

  
  
  mX.StartTouch(point.x, aEvent.mTime);
  mY.StartTouch(point.y, aEvent.mTime);

  double angle = atan2(dy, dx); 
  angle = fabs(angle); 

  if (gfxPrefs::TouchActionEnabled()) {
    HandlePanningWithTouchAction(angle);
  } else {
    if (GetAxisLockMode() == FREE) {
      SetState(PANNING);
    } else {
      HandlePanning(angle);
    }
  }

  if (IsInPanningState()) {
    if (nsRefPtr<GeckoContentController> controller = GetGeckoContentController()) {
      controller->NotifyAPZStateChange(GetGuid(), APZStateChange::StartPanning);
    }
    return nsEventStatus_eConsumeNoDefault;
  }
  
  return nsEventStatus_eIgnore;
}

void AsyncPanZoomController::UpdateWithTouchAtDevicePoint(const MultiTouchInput& aEvent) {
  ParentLayerPoint point = GetFirstTouchPoint(aEvent);
  mX.UpdateWithTouchAtDevicePoint(point.x, aEvent.mTime);
  mY.UpdateWithTouchAtDevicePoint(point.y, aEvent.mTime);
}

bool AsyncPanZoomController::AttemptScroll(const ParentLayerPoint& aStartPoint,
                                           const ParentLayerPoint& aEndPoint,
                                           OverscrollHandoffState& aOverscrollHandoffState) {

  
  
  
  ParentLayerPoint displacement = aStartPoint - aEndPoint;

  ParentLayerPoint overscroll;  
  {
    ReentrantMonitorAutoEnter lock(mMonitor);

    ParentLayerPoint adjustedDisplacement;
    bool forceVerticalOverscroll =
      (aOverscrollHandoffState.mScrollSource == ScrollSource::Wheel &&
       !mFrameMetrics.AllowVerticalScrollWithWheel());
    bool yChanged = mY.AdjustDisplacement(displacement.y, adjustedDisplacement.y, overscroll.y,
                                          forceVerticalOverscroll);
    bool xChanged = mX.AdjustDisplacement(displacement.x, adjustedDisplacement.x, overscroll.x);

    if (xChanged || yChanged) {
      ScheduleComposite();
    }

    if (!IsZero(adjustedDisplacement)) {
      ScrollBy(adjustedDisplacement / mFrameMetrics.GetZoom());
      ScheduleCompositeAndMaybeRepaint();
      UpdateSharedCompositorFrameMetrics();
    }
  }

  
  if (IsZero(overscroll)) {
    return true;
  }

  
  if (aOverscrollHandoffState.mScrollSource == ScrollSource::Wheel &&
      !AllowScrollHandoffInWheelTransaction())
  {
    return true;
  }

  
  
  
  
  
  ++aOverscrollHandoffState.mChainIndex;
  if (CallDispatchScroll(aEndPoint + overscroll, aEndPoint,
                         aOverscrollHandoffState)) {
    return true;
  }

  
  
  
  APZC_LOG("%p taking overscroll during panning\n", this);
  return OverscrollForPanning(overscroll, aOverscrollHandoffState.mPanDistance);
}

bool AsyncPanZoomController::OverscrollForPanning(ParentLayerPoint aOverscroll,
                                                  const ScreenPoint& aPanDistance) {
  
  
  
  if (!IsOverscrolled()) {
    if (aPanDistance.x < gfxPrefs::APZMinPanDistanceRatio() * aPanDistance.y) {
      aOverscroll.x = 0;
    }
    if (aPanDistance.y < gfxPrefs::APZMinPanDistanceRatio() * aPanDistance.x) {
      aOverscroll.y = 0;
    }
  }

  return OverscrollBy(aOverscroll);
}

bool AsyncPanZoomController::OverscrollBy(const ParentLayerPoint& aOverscroll) {
  if (!gfxPrefs::APZOverscrollEnabled()) {
    return false;
  }

  ReentrantMonitorAutoEnter lock(mMonitor);
  
  
  bool xCanScroll = mX.CanScroll();
  bool yCanScroll = mY.CanScroll();
  if (xCanScroll) {
    mX.OverscrollBy(aOverscroll.x);
  }
  if (yCanScroll) {
    mY.OverscrollBy(aOverscroll.y);
  }
  if (xCanScroll || yCanScroll) {
    ScheduleComposite();
    return true;
  }
  
  
  return false;
}

nsRefPtr<const OverscrollHandoffChain> AsyncPanZoomController::BuildOverscrollHandoffChain() {
  if (APZCTreeManager* treeManagerLocal = GetApzcTreeManager()) {
    return treeManagerLocal->BuildOverscrollHandoffChain(this);
  }

  
  
  OverscrollHandoffChain* result = new OverscrollHandoffChain;
  result->Add(this);
  return result;
}

void AsyncPanZoomController::AcceptFling(const ParentLayerPoint& aVelocity,
                                         const nsRefPtr<const OverscrollHandoffChain>& aOverscrollHandoffChain,
                                         bool aHandoff) {
  
  
  mX.SetVelocity(mX.GetVelocity() + aVelocity.x);
  mY.SetVelocity(mY.GetVelocity() + aVelocity.y);
  SetState(FLING);
  FlingAnimation *fling = new FlingAnimation(*this,
      aOverscrollHandoffChain,
      !aHandoff);  

  float friction = gfxPrefs::APZFlingFriction();
  ParentLayerPoint velocity(mX.GetVelocity(), mY.GetVelocity());
  ParentLayerPoint predictedDelta;
  
  
  if (velocity.x != 0.0f) {
    predictedDelta.x = -velocity.x / log(1.0 - friction);
  }
  if (velocity.y != 0.0f) {
    predictedDelta.y = -velocity.y / log(1.0 - friction);
  }
  CSSPoint predictedDestination = mFrameMetrics.GetScrollOffset() + predictedDelta / mFrameMetrics.GetZoom();

  nsRefPtr<GeckoContentController> controller = GetGeckoContentController();
  if (controller) {
    APZC_LOG("%p fling snapping.  friction: %f velocity: %f, %f "
             "predictedDelta: %f, %f position: %f, %f "
             "predictedDestination: %f, %f\n",
             this, friction, velocity.x, velocity.y, (float)predictedDelta.x,
             (float)predictedDelta.y, (float)mFrameMetrics.GetScrollOffset().x,
             (float)mFrameMetrics.GetScrollOffset().y,
             (float)predictedDestination.x, (float)predictedDestination.y);
    controller->RequestFlingSnap(mFrameMetrics.GetScrollId(),
                                 predictedDestination);
  }

  StartAnimation(fling);
}

bool AsyncPanZoomController::AttemptFling(ParentLayerPoint aVelocity,
                                          const nsRefPtr<const OverscrollHandoffChain>& aOverscrollHandoffChain,
                                          bool aHandoff) {
  
  if (IsPannable()) {
    AcceptFling(aVelocity,
                aOverscrollHandoffChain,
                aHandoff);
    return true;
  }

  return false;
}

void AsyncPanZoomController::HandleFlingOverscroll(const ParentLayerPoint& aVelocity,
                                                   const nsRefPtr<const OverscrollHandoffChain>& aOverscrollHandoffChain) {
  APZCTreeManager* treeManagerLocal = GetApzcTreeManager();
  if (!(treeManagerLocal && treeManagerLocal->DispatchFling(this,
                                                            aVelocity,
                                                            aOverscrollHandoffChain,
                                                            true ))) {
    
    
    if (IsPannable() && gfxPrefs::APZOverscrollEnabled()) {
      StartOverscrollAnimation(aVelocity);
    }
  }
}

void AsyncPanZoomController::HandleSmoothScrollOverscroll(const ParentLayerPoint& aVelocity) {
  
  
  HandleFlingOverscroll(aVelocity, BuildOverscrollHandoffChain());
}

void AsyncPanZoomController::StartSmoothScroll(ScrollSource aSource) {
  SetState(SMOOTH_SCROLL);
  nsPoint initialPosition = CSSPoint::ToAppUnits(mFrameMetrics.GetScrollOffset());
  
  
  nsPoint initialVelocity = CSSPoint::ToAppUnits(CSSPoint(mX.GetVelocity(),
                                                          mY.GetVelocity())) * 1000.0f;
  nsPoint destination = CSSPoint::ToAppUnits(mFrameMetrics.GetSmoothScrollOffset());

  StartAnimation(new SmoothScrollAnimation(*this,
                                           aSource,
                                           initialPosition, initialVelocity,
                                           destination,
                                           gfxPrefs::ScrollBehaviorSpringConstant(),
                                           gfxPrefs::ScrollBehaviorDampingRatio()));
}

void AsyncPanZoomController::StartOverscrollAnimation(const ParentLayerPoint& aVelocity) {
  SetState(OVERSCROLL_ANIMATION);
  StartAnimation(new OverscrollAnimation(*this, aVelocity));
}

bool AsyncPanZoomController::CallDispatchScroll(const ParentLayerPoint& aStartPoint,
                                                const ParentLayerPoint& aEndPoint,
                                                OverscrollHandoffState& aOverscrollHandoffState) {
  
  
  
  APZCTreeManager* treeManagerLocal = GetApzcTreeManager();
  if (!treeManagerLocal) {
    return false;
  }
  return treeManagerLocal->DispatchScroll(this,
                                          aStartPoint, aEndPoint,
                                          aOverscrollHandoffState);
}

void AsyncPanZoomController::TrackTouch(const MultiTouchInput& aEvent) {
  ParentLayerPoint prevTouchPoint(mX.GetPos(), mY.GetPos());
  ParentLayerPoint touchPoint = GetFirstTouchPoint(aEvent);

  ScreenPoint panDistance = ToScreenCoordinates(
      ParentLayerPoint(mX.PanDistance(touchPoint.x),
                       mY.PanDistance(touchPoint.y)),
      PanStart());
  HandlePanningUpdate(panDistance);

  UpdateWithTouchAtDevicePoint(aEvent);

  if (prevTouchPoint != touchPoint) {
    OverscrollHandoffState handoffState(
        *CurrentTouchBlock()->GetOverscrollHandoffChain(),
        panDistance,
        ScrollSource::Touch);
    CallDispatchScroll(prevTouchPoint, touchPoint, handoffState);
  }
}

ParentLayerPoint AsyncPanZoomController::GetFirstTouchPoint(const MultiTouchInput& aEvent) {
  return ((SingleTouchData&)aEvent.mTouches[0]).mLocalScreenPoint;
}

void AsyncPanZoomController::StartAnimation(AsyncPanZoomAnimation* aAnimation)
{
  ReentrantMonitorAutoEnter lock(mMonitor);
  mAnimation = aAnimation;
  mLastSampleTime = GetFrameTime();
  ScheduleComposite();
}

void AsyncPanZoomController::CancelAnimation(CancelAnimationFlags aFlags) {
  ReentrantMonitorAutoEnter lock(mMonitor);
  APZC_LOG("%p running CancelAnimation in state %d\n", this, mState);
  SetState(NOTHING);
  mAnimation = nullptr;
  
  
  mX.SetVelocity(0);
  mY.SetVelocity(0);
  
  
  
  if (!(aFlags & ExcludeOverscroll) && IsOverscrolled()) {
    ClearOverscroll();
    RequestContentRepaint();
    ScheduleComposite();
    UpdateSharedCompositorFrameMetrics();
  }
}

void AsyncPanZoomController::ClearOverscroll() {
  ReentrantMonitorAutoEnter lock(mMonitor);
  mX.ClearOverscroll();
  mY.ClearOverscroll();
}

void AsyncPanZoomController::SetCompositorParent(CompositorParent* aCompositorParent) {
  mCompositorParent = aCompositorParent;
}

void AsyncPanZoomController::ShareFrameMetricsAcrossProcesses() {
  mSharingFrameMetricsAcrossProcesses = true;
}

void AsyncPanZoomController::ScrollBy(const CSSPoint& aOffset) {
  mFrameMetrics.ScrollBy(aOffset);
}

void AsyncPanZoomController::ScaleWithFocus(float aScale,
                                            const CSSPoint& aFocus) {
  mFrameMetrics.ZoomBy(aScale);
  
  
  
  
  mFrameMetrics.SetScrollOffset((mFrameMetrics.GetScrollOffset() + aFocus) - (aFocus / aScale));
}




static CSSSize
CalculateDisplayPortSize(const CSSSize& aCompositionSize,
                         const CSSPoint& aVelocity)
{
  float xMultiplier = fabsf(aVelocity.x) < gfxPrefs::APZMinSkateSpeed()
                        ? gfxPrefs::APZXStationarySizeMultiplier()
                        : gfxPrefs::APZXSkateSizeMultiplier();
  float yMultiplier = fabsf(aVelocity.y) < gfxPrefs::APZMinSkateSpeed()
                        ? gfxPrefs::APZYStationarySizeMultiplier()
                        : gfxPrefs::APZYSkateSizeMultiplier();

  
  
  
  
  float xSize = std::max(aCompositionSize.width * xMultiplier,
                         aCompositionSize.width + (2 * gfxPrefs::APZDangerZoneX()));
  float ySize = std::max(aCompositionSize.height * yMultiplier,
                         aCompositionSize.height + (2 * gfxPrefs::APZDangerZoneY()));

  return CSSSize(xSize, ySize);
}






static void
RedistributeDisplayPortExcess(CSSSize& aDisplayPortSize,
                              const CSSRect& aScrollableRect)
{
  float xSlack = std::max(0.0f, aDisplayPortSize.width - aScrollableRect.width);
  float ySlack = std::max(0.0f, aDisplayPortSize.height - aScrollableRect.height);

  if (ySlack > 0) {
    
    aDisplayPortSize.height -= ySlack;
    float xExtra = ySlack * aDisplayPortSize.width / aDisplayPortSize.height;
    aDisplayPortSize.width += xExtra;
  } else if (xSlack > 0) {
    
    aDisplayPortSize.width -= xSlack;
    float yExtra = xSlack * aDisplayPortSize.height / aDisplayPortSize.width;
    aDisplayPortSize.height += yExtra;
  }
}


const ScreenMargin AsyncPanZoomController::CalculatePendingDisplayPort(
  const FrameMetrics& aFrameMetrics,
  const ParentLayerPoint& aVelocity,
  double aEstimatedPaintDuration)
{
  CSSSize compositionSize = aFrameMetrics.CalculateBoundedCompositedSizeInCssPixels();
  CSSPoint velocity = aVelocity / aFrameMetrics.GetZoom();
  CSSPoint scrollOffset = aFrameMetrics.GetScrollOffset();
  CSSRect scrollableRect = aFrameMetrics.GetExpandedScrollableRect();

  
  CSSSize displayPortSize = CalculateDisplayPortSize(compositionSize, velocity);

  if (gfxPrefs::APZEnlargeDisplayPortWhenClipped()) {
    RedistributeDisplayPortExcess(displayPortSize, scrollableRect);
  }

  
  
  float estimatedPaintDurationMillis = (float)(aEstimatedPaintDuration * 1000.0);
  float paintFactor = (gfxPrefs::APZUsePaintDuration() ? estimatedPaintDurationMillis : 50.0f);
  CSSRect displayPort = CSSRect(scrollOffset + (velocity * paintFactor * gfxPrefs::APZVelocityBias()),
                                displayPortSize);

  
  displayPort.MoveBy((compositionSize.width - displayPort.width)/2.0f,
                     (compositionSize.height - displayPort.height)/2.0f);

  
  displayPort = displayPort.ForceInside(scrollableRect) - scrollOffset;

  APZC_LOG_FM(aFrameMetrics,
    "Calculated displayport as (%f %f %f %f) from velocity %s paint time %f metrics",
    displayPort.x, displayPort.y, displayPort.width, displayPort.height,
    ToString(aVelocity).c_str(), (float)estimatedPaintDurationMillis);

  CSSMargin cssMargins;
  cssMargins.left = -displayPort.x;
  cssMargins.top = -displayPort.y;
  cssMargins.right = displayPort.width - compositionSize.width - cssMargins.left;
  cssMargins.bottom = displayPort.height - compositionSize.height - cssMargins.top;

  return cssMargins * aFrameMetrics.DisplayportPixelsPerCSSPixel();
}

void AsyncPanZoomController::ScheduleComposite() {
  if (mCompositorParent) {
    mCompositorParent->ScheduleRenderOnCompositorThread();
  }
}

void AsyncPanZoomController::ScheduleCompositeAndMaybeRepaint() {
  ScheduleComposite();

  TimeDuration timePaintDelta = mPaintThrottler.TimeSinceLastRequest(GetFrameTime());
  if (timePaintDelta.ToMilliseconds() > gfxPrefs::APZPanRepaintInterval()) {
    RequestContentRepaint();
  }
}

void AsyncPanZoomController::FlushRepaintForOverscrollHandoff() {
  ReentrantMonitorAutoEnter lock(mMonitor);
  RequestContentRepaint();
  UpdateSharedCompositorFrameMetrics();
}

void AsyncPanZoomController::FlushRepaintForNewInputBlock() {
  APZC_LOG("%p flushing repaint for new input block\n", this);

  ReentrantMonitorAutoEnter lock(mMonitor);
  
  
  
  
  
  mPaintThrottler.CancelPendingTask();
  mLastPaintRequestMetrics = mLastDispatchedPaintMetrics;

  RequestContentRepaint(mFrameMetrics, false );
  UpdateSharedCompositorFrameMetrics();
}

bool AsyncPanZoomController::SnapBackIfOverscrolled() {
  ReentrantMonitorAutoEnter lock(mMonitor);
  if (IsOverscrolled()) {
    APZC_LOG("%p is overscrolled, starting snap-back\n", this);
    StartOverscrollAnimation(ParentLayerPoint(0, 0));
    return true;
  }
  return false;
}

bool AsyncPanZoomController::IsMovingFast() const {
  ReentrantMonitorAutoEnter lock(mMonitor);
  if (GetVelocityVector().Length() > gfxPrefs::APZFlingStopOnTapThreshold()) {
    APZC_LOG("%p is moving fast\n", this);
    return true;
  }
  return false;
}

bool AsyncPanZoomController::IsPannable() const {
  ReentrantMonitorAutoEnter lock(mMonitor);
  return mX.CanScroll() || mY.CanScroll();
}

int32_t AsyncPanZoomController::GetLastTouchIdentifier() const {
  nsRefPtr<GestureEventListener> listener = GetGestureEventListener();
  return listener ? listener->GetLastTouchIdentifier() : -1;
}

void AsyncPanZoomController::RequestContentRepaint() {
  RequestContentRepaint(mFrameMetrics);
}

void AsyncPanZoomController::RequestContentRepaint(FrameMetrics& aFrameMetrics, bool aThrottled) {
  aFrameMetrics.SetDisplayPortMargins(
    CalculatePendingDisplayPort(aFrameMetrics,
                                GetVelocityVector(),
                                mPaintThrottler.AverageDuration().ToSeconds()));
  aFrameMetrics.SetUseDisplayPortMargins();

  
  
  ScreenMargin marginDelta = (mLastPaintRequestMetrics.GetDisplayPortMargins()
                           - aFrameMetrics.GetDisplayPortMargins());
  if (fabsf(marginDelta.left) < EPSILON &&
      fabsf(marginDelta.top) < EPSILON &&
      fabsf(marginDelta.right) < EPSILON &&
      fabsf(marginDelta.bottom) < EPSILON &&
      fabsf(mLastPaintRequestMetrics.GetScrollOffset().x -
            aFrameMetrics.GetScrollOffset().x) < EPSILON &&
      fabsf(mLastPaintRequestMetrics.GetScrollOffset().y -
            aFrameMetrics.GetScrollOffset().y) < EPSILON &&
      aFrameMetrics.GetZoom() == mLastPaintRequestMetrics.GetZoom() &&
      fabsf(aFrameMetrics.GetViewport().width - mLastPaintRequestMetrics.GetViewport().width) < EPSILON &&
      fabsf(aFrameMetrics.GetViewport().height - mLastPaintRequestMetrics.GetViewport().height) < EPSILON) {
    return;
  }

  SendAsyncScrollEvent();
  if (aThrottled) {
    mPaintThrottler.PostTask(
      FROM_HERE,
      UniquePtr<CancelableTask>(NewRunnableMethod(this,
                        &AsyncPanZoomController::DispatchRepaintRequest,
                        aFrameMetrics)),
      GetFrameTime());
  } else {
    DispatchRepaintRequest(aFrameMetrics);
  }

  aFrameMetrics.SetPresShellId(mLastContentPaintMetrics.GetPresShellId());
  mLastPaintRequestMetrics = aFrameMetrics;
}

 CSSRect
GetDisplayPortRect(const FrameMetrics& aFrameMetrics)
{
  
  
  CSSRect baseRect(aFrameMetrics.GetScrollOffset(),
                   aFrameMetrics.CalculateBoundedCompositedSizeInCssPixels());
  baseRect.Inflate(aFrameMetrics.GetDisplayPortMargins() / aFrameMetrics.DisplayportPixelsPerCSSPixel());
  return baseRect;
}

void
AsyncPanZoomController::DispatchRepaintRequest(const FrameMetrics& aFrameMetrics) {
  nsRefPtr<GeckoContentController> controller = GetGeckoContentController();
  if (controller) {
    APZC_LOG_FM(aFrameMetrics, "%p requesting content repaint", this);
    LogRendertraceRect(GetGuid(), "requested displayport", "yellow", GetDisplayPortRect(aFrameMetrics));

    if (NS_IsMainThread()) {
      controller->RequestContentRepaint(aFrameMetrics);
    } else {
      NS_DispatchToMainThread(NS_NewRunnableMethodWithArg<FrameMetrics>(
        controller, &GeckoContentController::RequestContentRepaint, aFrameMetrics));
    }
    mLastDispatchedPaintMetrics = aFrameMetrics;
  }
}

void
AsyncPanZoomController::FireAsyncScrollOnTimeout()
{
  if (mCurrentAsyncScrollOffset != mLastAsyncScrollOffset) {
    ReentrantMonitorAutoEnter lock(mMonitor);
    SendAsyncScrollEvent();
  }
  mAsyncScrollTimeoutTask = nullptr;
}

bool AsyncPanZoomController::UpdateAnimation(const TimeStamp& aSampleTime,
                                             Vector<Task*>* aOutDeferredTasks)
{
  APZThreadUtils::AssertOnCompositorThread();

  
  
  
  
  if (mLastSampleTime == aSampleTime) {
    return false;
  }
  TimeDuration sampleTimeDelta = aSampleTime - mLastSampleTime;
  mLastSampleTime = aSampleTime;

  if (mAnimation) {
    bool continueAnimation = mAnimation->Sample(mFrameMetrics, sampleTimeDelta);
    *aOutDeferredTasks = mAnimation->TakeDeferredTasks();
    if (continueAnimation) {
      if (mPaintThrottler.TimeSinceLastRequest(aSampleTime) >
          mAnimation->mRepaintInterval) {
        RequestContentRepaint();
      }
    } else {
      mAnimation = nullptr;
      SetState(NOTHING);
      SendAsyncScrollEvent();
      RequestContentRepaint();
    }
    UpdateSharedCompositorFrameMetrics();
    return true;
  }
  return false;
}

Matrix4x4 AsyncPanZoomController::GetOverscrollTransform() const {
  ReentrantMonitorAutoEnter lock(mMonitor);
  if (!IsOverscrolled()) {
    return Matrix4x4();
  }

  
  
  

  
  
  const float kStretchFactor = gfxPrefs::APZOverscrollStretchFactor();

  
  
  ParentLayerSize compositionSize(mX.GetCompositionLength(), mY.GetCompositionLength());
  float scaleX = 1 + kStretchFactor * fabsf(mX.GetOverscroll()) / mX.GetCompositionLength();
  float scaleY = 1 + kStretchFactor * fabsf(mY.GetOverscroll()) / mY.GetCompositionLength();

  
  
  
  
  
  ParentLayerPoint translation;
  bool overscrolledOnRight = mX.GetOverscroll() > 0;
  if (overscrolledOnRight) {
    ParentLayerCoord overscrolledCompositionWidth = scaleX * compositionSize.width;
    ParentLayerCoord extraCompositionWidth = overscrolledCompositionWidth - compositionSize.width;
    translation.x = -extraCompositionWidth;
  }
  bool overscrolledAtBottom = mY.GetOverscroll() > 0;
  if (overscrolledAtBottom) {
    ParentLayerCoord overscrolledCompositionHeight = scaleY * compositionSize.height;
    ParentLayerCoord extraCompositionHeight = overscrolledCompositionHeight - compositionSize.height;
    translation.y = -extraCompositionHeight;
  }

  
  return Matrix4x4::Scaling(scaleX, scaleY, 1)
                    .PostTranslate(translation.x, translation.y, 0);
}

bool AsyncPanZoomController::AdvanceAnimations(const TimeStamp& aSampleTime)
{
  APZThreadUtils::AssertOnCompositorThread();

  
  
  
  StateChangeNotificationBlocker blocker(this);

  
  
  
  
  
  mAsyncTransformAppliedToContent = false;
  bool requestAnimationFrame = false;
  Vector<Task*> deferredTasks;

  {
    ReentrantMonitorAutoEnter lock(mMonitor);

    requestAnimationFrame = UpdateAnimation(aSampleTime, &deferredTasks);

    LogRendertraceRect(GetGuid(), "viewport", "red",
      CSSRect(mFrameMetrics.GetScrollOffset(),
              mFrameMetrics.CalculateCompositedSizeInCssPixels()));

    mCurrentAsyncScrollOffset = mFrameMetrics.GetScrollOffset();
  }

  
  
  
  
  for (uint32_t i = 0; i < deferredTasks.length(); ++i) {
    deferredTasks[i]->Run();
    delete deferredTasks[i];
  }

  
  
  requestAnimationFrame |= (mAnimation != nullptr);

  
  
  if (mAsyncScrollTimeoutTask) {
    mAsyncScrollTimeoutTask->Cancel();
    mAsyncScrollTimeoutTask = nullptr;
  }
  
  
  
  
  
  TimeDuration delta = aSampleTime - mLastAsyncScrollTime;
  if (delta.ToMilliseconds() > gfxPrefs::APZAsyncScrollThrottleTime() &&
      mCurrentAsyncScrollOffset != mLastAsyncScrollOffset) {
    ReentrantMonitorAutoEnter lock(mMonitor);
    mLastAsyncScrollTime = aSampleTime;
    mLastAsyncScrollOffset = mCurrentAsyncScrollOffset;
    SendAsyncScrollEvent();
  } else {
    mAsyncScrollTimeoutTask =
      NewRunnableMethod(this, &AsyncPanZoomController::FireAsyncScrollOnTimeout);
    MessageLoop::current()->PostDelayedTask(FROM_HERE,
                                            mAsyncScrollTimeoutTask,
                                            gfxPrefs::APZAsyncScrollTimeout());
  }

  return requestAnimationFrame;
}

void AsyncPanZoomController::SampleContentTransformForFrame(ViewTransform* aOutTransform,
                                                            ParentLayerPoint& aScrollOffset)
{
  ReentrantMonitorAutoEnter lock(mMonitor);

  aScrollOffset = mFrameMetrics.GetScrollOffset() * mFrameMetrics.GetZoom();
  *aOutTransform = GetCurrentAsyncTransform();
}

ViewTransform AsyncPanZoomController::GetCurrentAsyncTransform() const {
  ReentrantMonitorAutoEnter lock(mMonitor);

  CSSPoint lastPaintScrollOffset;
  if (mLastContentPaintMetrics.IsScrollable()) {
    lastPaintScrollOffset = mLastContentPaintMetrics.GetScrollOffset();
  }

  CSSPoint currentScrollOffset = mFrameMetrics.GetScrollOffset() +
    mTestAsyncScrollOffset;

  
  
  if (!gfxPrefs::APZAllowCheckerboarding() &&
      !mLastContentPaintMetrics.GetDisplayPort().IsEmpty()) {
    CSSSize compositedSize = mLastContentPaintMetrics.CalculateCompositedSizeInCssPixels();
    CSSPoint maxScrollOffset = lastPaintScrollOffset +
      CSSPoint(mLastContentPaintMetrics.GetDisplayPort().XMost() - compositedSize.width,
               mLastContentPaintMetrics.GetDisplayPort().YMost() - compositedSize.height);
    CSSPoint minScrollOffset = lastPaintScrollOffset + mLastContentPaintMetrics.GetDisplayPort().TopLeft();

    if (minScrollOffset.x < maxScrollOffset.x) {
      currentScrollOffset.x = clamped(currentScrollOffset.x, minScrollOffset.x, maxScrollOffset.x);
    }
    if (minScrollOffset.y < maxScrollOffset.y) {
      currentScrollOffset.y = clamped(currentScrollOffset.y, minScrollOffset.y, maxScrollOffset.y);
    }
  }

  ParentLayerPoint translation = (currentScrollOffset - lastPaintScrollOffset)
                               * mFrameMetrics.GetZoom();

  return ViewTransform(mFrameMetrics.GetAsyncZoom(), -translation);
}

Matrix4x4 AsyncPanZoomController::GetCurrentAsyncTransformWithOverscroll() const {
  return Matrix4x4(GetCurrentAsyncTransform()) * GetOverscrollTransform();
}

Matrix4x4 AsyncPanZoomController::GetTransformToLastDispatchedPaint() const {
  ReentrantMonitorAutoEnter lock(mMonitor);

  LayerPoint scrollChange =
    (mLastContentPaintMetrics.GetScrollOffset() - mLastDispatchedPaintMetrics.GetScrollOffset())
    * mLastContentPaintMetrics.GetDevPixelsPerCSSPixel()
    * mLastContentPaintMetrics.GetCumulativeResolution();

  gfxSize zoomChange = mLastContentPaintMetrics.GetZoom() / mLastDispatchedPaintMetrics.GetZoom();

  return Matrix4x4::Translation(scrollChange.x, scrollChange.y, 0).
           PostScale(zoomChange.width, zoomChange.height, 1);
}

bool AsyncPanZoomController::IsCurrentlyCheckerboarding() const {
  ReentrantMonitorAutoEnter lock(mMonitor);

  if (!gfxPrefs::APZAllowCheckerboarding()) {
    return false;
  }

  CSSPoint currentScrollOffset = mFrameMetrics.GetScrollOffset() + mTestAsyncScrollOffset;
  CSSRect painted = mLastContentPaintMetrics.GetDisplayPort() + mLastContentPaintMetrics.GetScrollOffset();
  painted.Inflate(CSSMargin::FromAppUnits(nsMargin(1, 1, 1, 1)));   
  CSSRect visible = CSSRect(currentScrollOffset, mFrameMetrics.CalculateCompositedSizeInCssPixels());
  return !painted.Contains(visible);
}

void AsyncPanZoomController::NotifyLayersUpdated(const FrameMetrics& aLayerMetrics, bool aIsFirstPaint) {
  APZThreadUtils::AssertOnCompositorThread();

  ReentrantMonitorAutoEnter lock(mMonitor);
  bool isDefault = mFrameMetrics.IsDefault();

  mLastContentPaintMetrics = aLayerMetrics;

  mFrameMetrics.SetScrollParentId(aLayerMetrics.GetScrollParentId());
  APZC_LOG_FM(aLayerMetrics, "%p got a NotifyLayersUpdated with aIsFirstPaint=%d", this, aIsFirstPaint);

  LogRendertraceRect(GetGuid(), "page", "brown", aLayerMetrics.GetScrollableRect());
  LogRendertraceRect(GetGuid(), "painted displayport", "lightgreen",
    aLayerMetrics.GetDisplayPort() + aLayerMetrics.GetScrollOffset());
  if (!aLayerMetrics.GetCriticalDisplayPort().IsEmpty()) {
    LogRendertraceRect(GetGuid(), "painted critical displayport", "darkgreen",
      aLayerMetrics.GetCriticalDisplayPort() + aLayerMetrics.GetScrollOffset());
  }

  mPaintThrottler.TaskComplete(GetFrameTime());
  bool needContentRepaint = false;
  bool viewportUpdated = false;
  if (FuzzyEqualsAdditive(aLayerMetrics.mCompositionBounds.width, mFrameMetrics.mCompositionBounds.width) &&
      FuzzyEqualsAdditive(aLayerMetrics.mCompositionBounds.height, mFrameMetrics.mCompositionBounds.height)) {
    
    
    if (mFrameMetrics.GetViewport().width != aLayerMetrics.GetViewport().width ||
        mFrameMetrics.GetViewport().height != aLayerMetrics.GetViewport().height) {
      needContentRepaint = true;
      viewportUpdated = true;
    }
    mFrameMetrics.SetViewport(aLayerMetrics.GetViewport());
  }

  
  
  
  
  bool scrollOffsetUpdated = aLayerMetrics.GetScrollOffsetUpdated()
        && (aLayerMetrics.GetScrollGeneration() != mFrameMetrics.GetScrollGeneration());

  bool smoothScrollRequested = aLayerMetrics.GetDoSmoothScroll()
       && (aLayerMetrics.GetScrollGeneration() != mFrameMetrics.GetScrollGeneration());

  if (aIsFirstPaint || isDefault) {
    
    
    mPaintThrottler.ClearHistory();
    mPaintThrottler.SetMaxDurations(gfxPrefs::APZNumPaintDurationSamples());

    CancelAnimation();

    mFrameMetrics = aLayerMetrics;
    mLastDispatchedPaintMetrics = aLayerMetrics;
    ShareCompositorFrameMetrics();

    if (mFrameMetrics.GetDisplayPortMargins() != ScreenMargin()) {
      
      
      
      
      APZC_LOG("%p detected non-empty margins which probably need updating\n", this);
      needContentRepaint = true;
    }
  } else {
    
    
    

    if (FuzzyEqualsAdditive(mFrameMetrics.mCompositionBounds.width, aLayerMetrics.mCompositionBounds.width) &&
        mFrameMetrics.GetDevPixelsPerCSSPixel() == aLayerMetrics.GetDevPixelsPerCSSPixel() &&
        !viewportUpdated) {
      
      
      
      
      
      
      
      gfxSize totalResolutionChange = aLayerMetrics.GetCumulativeResolution()
                                    / mFrameMetrics.GetCumulativeResolution();
      float presShellResolutionChange = aLayerMetrics.GetPresShellResolution()
                                      / mFrameMetrics.GetPresShellResolution();
      mFrameMetrics.ZoomBy(totalResolutionChange / presShellResolutionChange);
    } else {
      
      
      
      mFrameMetrics.SetZoom(aLayerMetrics.GetZoom());
      mFrameMetrics.SetDevPixelsPerCSSPixel(aLayerMetrics.GetDevPixelsPerCSSPixel());
    }
    if (!mFrameMetrics.GetScrollableRect().IsEqualEdges(aLayerMetrics.GetScrollableRect())) {
      mFrameMetrics.SetScrollableRect(aLayerMetrics.GetScrollableRect());
      needContentRepaint = true;
    }
    mFrameMetrics.mCompositionBounds = aLayerMetrics.mCompositionBounds;
    mFrameMetrics.SetRootCompositionSize(aLayerMetrics.GetRootCompositionSize());
    mFrameMetrics.SetPresShellResolution(aLayerMetrics.GetPresShellResolution());
    mFrameMetrics.SetCumulativeResolution(aLayerMetrics.GetCumulativeResolution());
    mFrameMetrics.SetHasScrollgrab(aLayerMetrics.GetHasScrollgrab());
    mFrameMetrics.SetLineScrollAmount(aLayerMetrics.GetLineScrollAmount());
    mFrameMetrics.SetPageScrollAmount(aLayerMetrics.GetPageScrollAmount());

    if (scrollOffsetUpdated) {
      APZC_LOG("%p updating scroll offset from %s to %s\n", this,
        ToString(mFrameMetrics.GetScrollOffset()).c_str(),
        ToString(aLayerMetrics.GetScrollOffset()).c_str());

      mFrameMetrics.CopyScrollInfoFrom(aLayerMetrics);

      
      
      
      CancelAnimation();

      
      
      
      
      
      mLastDispatchedPaintMetrics = aLayerMetrics;

      
      
      
      
      
      
      
      needContentRepaint = true;
    }
  }

  if (smoothScrollRequested) {
    
    
    

    APZC_LOG("%p smooth scrolling from %s to %s\n", this,
      Stringify(mFrameMetrics.GetScrollOffset()).c_str(),
      Stringify(aLayerMetrics.GetSmoothScrollOffset()).c_str());

    mFrameMetrics.CopySmoothScrollInfoFrom(aLayerMetrics);
    CancelAnimation();
    mLastDispatchedPaintMetrics = aLayerMetrics;
    StartSmoothScroll(ScrollSource::DOM);

    scrollOffsetUpdated = true; 
  }

  if (scrollOffsetUpdated) {
    
    
    
    
    nsRefPtr<GeckoContentController> controller = GetGeckoContentController();
    if (controller) {
      APZC_LOG("%p sending scroll update acknowledgement with gen %u\n", this, aLayerMetrics.GetScrollGeneration());
      controller->AcknowledgeScrollUpdate(aLayerMetrics.GetScrollId(),
                                          aLayerMetrics.GetScrollGeneration());
    }
  }

  if (needContentRepaint) {
    RequestContentRepaint();
  }
  UpdateSharedCompositorFrameMetrics();
}

const FrameMetrics& AsyncPanZoomController::GetFrameMetrics() const {
  mMonitor.AssertCurrentThreadIn();
  return mFrameMetrics;
}

APZCTreeManager* AsyncPanZoomController::GetApzcTreeManager() const {
  mMonitor.AssertNotCurrentThreadIn();
  return mTreeManager;
}

void AsyncPanZoomController::ZoomToRect(CSSRect aRect) {
  if (!aRect.IsFinite()) {
    NS_WARNING("ZoomToRect got called with a non-finite rect; ignoring...\n");
    return;
  }

  
  
  
  
  
  MOZ_ASSERT(mFrameMetrics.IsRootScrollable());
  MOZ_ASSERT(mFrameMetrics.GetZoom().AreScalesSame());

  SetState(ANIMATING_ZOOM);

  {
    ReentrantMonitorAutoEnter lock(mMonitor);

    ParentLayerRect compositionBounds = mFrameMetrics.mCompositionBounds;
    CSSRect cssPageRect = mFrameMetrics.GetScrollableRect();
    CSSPoint scrollOffset = mFrameMetrics.GetScrollOffset();
    CSSToParentLayerScale currentZoom = mFrameMetrics.GetZoom().ToScaleFactor();
    CSSToParentLayerScale targetZoom;

    
    
    
    
    
    CSSToParentLayerScale localMinZoom(std::max(mZoomConstraints.mMinZoom.scale,
                                       std::max(compositionBounds.width / cssPageRect.width,
                                                compositionBounds.height / cssPageRect.height)));
    CSSToParentLayerScale localMaxZoom = mZoomConstraints.mMaxZoom;

    if (!aRect.IsEmpty()) {
      
      aRect = aRect.Intersect(cssPageRect);
      targetZoom = CSSToParentLayerScale(std::min(compositionBounds.width / aRect.width,
                                                  compositionBounds.height / aRect.height));
    }
    
    
    
    
    if (aRect.IsEmpty() ||
        (currentZoom == localMaxZoom && targetZoom >= localMaxZoom) ||
        (currentZoom == localMinZoom && targetZoom <= localMinZoom)) {
      CSSSize compositedSize = mFrameMetrics.CalculateCompositedSizeInCssPixels();
      float y = scrollOffset.y;
      float newHeight =
        cssPageRect.width * (compositedSize.height / compositedSize.width);
      float dh = compositedSize.height - newHeight;

      aRect = CSSRect(0.0f,
                      y + dh/2,
                      cssPageRect.width,
                      newHeight);
      aRect = aRect.Intersect(cssPageRect);
      targetZoom = CSSToParentLayerScale(std::min(compositionBounds.width / aRect.width,
                                                  compositionBounds.height / aRect.height));
    }

    targetZoom.scale = clamped(targetZoom.scale, localMinZoom.scale, localMaxZoom.scale);
    FrameMetrics endZoomToMetrics = mFrameMetrics;
    endZoomToMetrics.SetZoom(CSSToParentLayerScale2D(targetZoom));

    
    CSSSize sizeAfterZoom = endZoomToMetrics.CalculateCompositedSizeInCssPixels();

    
    
    if (aRect.y + sizeAfterZoom.height > cssPageRect.height) {
      aRect.y = cssPageRect.height - sizeAfterZoom.height;
      aRect.y = aRect.y > 0 ? aRect.y : 0;
    }
    if (aRect.x + sizeAfterZoom.width > cssPageRect.width) {
      aRect.x = cssPageRect.width - sizeAfterZoom.width;
      aRect.x = aRect.x > 0 ? aRect.x : 0;
    }

    endZoomToMetrics.SetScrollOffset(aRect.TopLeft());
    endZoomToMetrics.SetDisplayPortMargins(
      CalculatePendingDisplayPort(endZoomToMetrics,
                                  ParentLayerPoint(0,0),
                                  0));
    endZoomToMetrics.SetUseDisplayPortMargins();

    StartAnimation(new ZoomAnimation(
        mFrameMetrics.GetScrollOffset(),
        mFrameMetrics.GetZoom(),
        endZoomToMetrics.GetScrollOffset(),
        endZoomToMetrics.GetZoom()));

    
    
    RequestContentRepaint(endZoomToMetrics);
  }
}

TouchBlockState*
AsyncPanZoomController::CurrentTouchBlock()
{
  return GetInputQueue()->CurrentTouchBlock();
}

void
AsyncPanZoomController::ResetInputState()
{
  SetState(NOTHING);
  
  nsRefPtr<GestureEventListener> listener = GetGestureEventListener();
  if (listener) {
    MultiTouchInput cancel(MultiTouchInput::MULTITOUCH_CANCEL, 0, TimeStamp::Now(), 0);
    listener->HandleInputEvent(cancel);
  }
}

bool
AsyncPanZoomController::HasReadyTouchBlock()
{
  return GetInputQueue()->HasReadyTouchBlock();
}

void AsyncPanZoomController::SetState(PanZoomState aNewState)
{
  PanZoomState oldState;

  
  {
    ReentrantMonitorAutoEnter lock(mMonitor);
    oldState = mState;
    mState = aNewState;
  }

  DispatchStateChangeNotification(oldState, aNewState);
}

void AsyncPanZoomController::DispatchStateChangeNotification(PanZoomState aOldState,
                                                             PanZoomState aNewState)
{
  { 
    ReentrantMonitorAutoEnter lock(mMonitor);
    if (mNotificationBlockers > 0) {
      return;
    }
  }

  if (nsRefPtr<GeckoContentController> controller = GetGeckoContentController()) {
    if (!IsTransformingState(aOldState) && IsTransformingState(aNewState)) {
      controller->NotifyAPZStateChange(
          GetGuid(), APZStateChange::TransformBegin);
    } else if (IsTransformingState(aOldState) && !IsTransformingState(aNewState)) {
      controller->NotifyAPZStateChange(
          GetGuid(), APZStateChange::TransformEnd);
    }
  }
}

bool AsyncPanZoomController::IsTransformingState(PanZoomState aState) {
  return !(aState == NOTHING || aState == TOUCHING);
}

bool AsyncPanZoomController::IsInPanningState() const {
  return (mState == PANNING || mState == PANNING_LOCKED_X || mState == PANNING_LOCKED_Y);
}

void AsyncPanZoomController::UpdateZoomConstraints(const ZoomConstraints& aConstraints) {
  APZC_LOG("%p updating zoom constraints to %d %d %f %f\n", this, aConstraints.mAllowZoom,
    aConstraints.mAllowDoubleTapZoom, aConstraints.mMinZoom.scale, aConstraints.mMaxZoom.scale);
  if (IsNaN(aConstraints.mMinZoom.scale) || IsNaN(aConstraints.mMaxZoom.scale)) {
    NS_WARNING("APZC received zoom constraints with NaN values; dropping...\n");
    return;
  }
  
  mZoomConstraints.mAllowZoom = aConstraints.mAllowZoom;
  mZoomConstraints.mAllowDoubleTapZoom = aConstraints.mAllowDoubleTapZoom;
  mZoomConstraints.mMinZoom = (MIN_ZOOM > aConstraints.mMinZoom ? MIN_ZOOM : aConstraints.mMinZoom);
  mZoomConstraints.mMaxZoom = (MAX_ZOOM > aConstraints.mMaxZoom ? aConstraints.mMaxZoom : MAX_ZOOM);
  if (mZoomConstraints.mMaxZoom < mZoomConstraints.mMinZoom) {
    mZoomConstraints.mMaxZoom = mZoomConstraints.mMinZoom;
  }
}

ZoomConstraints
AsyncPanZoomController::GetZoomConstraints() const
{
  return mZoomConstraints;
}


void AsyncPanZoomController::PostDelayedTask(Task* aTask, int aDelayMs) {
  APZThreadUtils::AssertOnControllerThread();
  nsRefPtr<GeckoContentController> controller = GetGeckoContentController();
  if (controller) {
    controller->PostDelayedTask(aTask, aDelayMs);
  }
}

void AsyncPanZoomController::SendAsyncScrollEvent() {
  nsRefPtr<GeckoContentController> controller = GetGeckoContentController();
  if (!controller) {
    return;
  }

  bool isRoot;
  CSSRect contentRect;
  CSSSize scrollableSize;
  {
    ReentrantMonitorAutoEnter lock(mMonitor);

    isRoot = mFrameMetrics.GetIsRoot();
    scrollableSize = mFrameMetrics.GetScrollableRect().Size();
    contentRect = mFrameMetrics.CalculateCompositedRectInCssPixels();
    contentRect.MoveTo(mCurrentAsyncScrollOffset);
  }

  controller->SendAsyncScrollDOMEvent(isRoot, contentRect, scrollableSize);
}

bool AsyncPanZoomController::Matches(const ScrollableLayerGuid& aGuid)
{
  return aGuid == GetGuid();
}

bool AsyncPanZoomController::HasTreeManager(const APZCTreeManager* aTreeManager) const
{
  return GetApzcTreeManager() == aTreeManager;
}

void AsyncPanZoomController::GetGuid(ScrollableLayerGuid* aGuidOut) const
{
  if (aGuidOut) {
    *aGuidOut = GetGuid();
  }
}

ScrollableLayerGuid AsyncPanZoomController::GetGuid() const
{
  return ScrollableLayerGuid(mLayersId, mFrameMetrics);
}

void AsyncPanZoomController::UpdateSharedCompositorFrameMetrics()
{
  mMonitor.AssertCurrentThreadIn();

  FrameMetrics* frame = mSharedFrameMetricsBuffer ?
      static_cast<FrameMetrics*>(mSharedFrameMetricsBuffer->memory()) : nullptr;

  if (frame && mSharedLock && gfxPlatform::GetPlatform()->UseProgressivePaint()) {
    mSharedLock->Lock();
    *frame = mFrameMetrics.MakePODObject();
    mSharedLock->Unlock();
  }
}

void AsyncPanZoomController::ShareCompositorFrameMetrics() {

  PCompositorParent* compositor = GetSharedFrameMetricsCompositor();

  
  
  
  if (!mSharedFrameMetricsBuffer && compositor && gfxPlatform::GetPlatform()->UseProgressivePaint()) {

    
    mSharedFrameMetricsBuffer = new ipc::SharedMemoryBasic;
    FrameMetrics* frame = nullptr;
    mSharedFrameMetricsBuffer->Create(sizeof(FrameMetrics));
    mSharedFrameMetricsBuffer->Map(sizeof(FrameMetrics));
    frame = static_cast<FrameMetrics*>(mSharedFrameMetricsBuffer->memory());

    if (frame) {

      { 
        ReentrantMonitorAutoEnter lock(mMonitor);
        *frame = mFrameMetrics;
      }

      
      base::ProcessId otherPid = compositor->OtherPid();
      ipc::SharedMemoryBasic::Handle mem = ipc::SharedMemoryBasic::NULLHandle();

      
      mSharedFrameMetricsBuffer->ShareToProcess(otherPid, &mem);

      
      mSharedLock = new CrossProcessMutex("AsyncPanZoomControlLock");
      CrossProcessMutexHandle handle = mSharedLock->ShareToProcess(otherPid);

      
      
      
      if (!compositor->SendSharedCompositorFrameMetrics(mem, handle, mLayersId, mAPZCId)) {
        APZC_LOG("%p failed to share FrameMetrics with content process.", this);
      }
    }
  }
}

}
}
