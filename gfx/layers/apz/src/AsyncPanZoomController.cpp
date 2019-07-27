





#include <math.h>                       
#include <stdint.h>                     
#include <sys/types.h>                  
#include <algorithm>                    
#include "AnimationCommon.h"            
#include "AsyncPanZoomController.h"     
#include "Compositor.h"                 
#include "CompositorParent.h"           
#include "FrameMetrics.h"               
#include "GestureEventListener.h"       
#include "InputData.h"                  
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
#include "mozilla/dom/Touch.h"          
#include "mozilla/gfx/BasePoint.h"      
#include "mozilla/gfx/BaseRect.h"       
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Rect.h"           
#include "mozilla/gfx/ScaleFactor.h"    
#include "mozilla/layers/APZCTreeManager.h"  
#include "mozilla/layers/AsyncCompositionManager.h"  
#include "mozilla/layers/Axis.h"        
#include "mozilla/layers/LayerTransactionParent.h" 
#include "mozilla/layers/PCompositorParent.h" 
#include "mozilla/layers/TaskThrottler.h"  
#include "mozilla/mozalloc.h"           
#include "mozilla/unused.h"             
#include "mozilla/FloatingPoint.h"      
#include "nsAlgorithm.h"                
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsDebug.h"                    
#include "nsIDOMWindowUtils.h"          
#include "nsISupportsImpl.h"            
#include "nsMathUtils.h"                
#include "nsPoint.h"                    
#include "nsStyleConsts.h"
#include "nsStyleStruct.h"              
#include "nsTArray.h"                   
#include "nsThreadUtils.h"              
#include "SharedMemoryBasic.h"          



#define ENABLE_APZC_LOGGING 0


#if ENABLE_APZC_LOGGING
#  define APZC_LOG(...) printf_stderr("APZC: " __VA_ARGS__)
#  define APZC_LOG_FM(fm, prefix, ...) \
    { std::stringstream ss; \
      ss << nsPrintfCString(prefix, __VA_ARGS__).get(); \
      AppendToString(ss, fm, ":", "", true); \
      APZC_LOG("%s", ss.str().c_str()); \
    }
#else
#  define APZC_LOG(...)
#  define APZC_LOG_FM(fm, prefix, ...)
#endif


namespace {

int32_t
WidgetModifiersToDOMModifiers(mozilla::Modifiers aModifiers)
{
  int32_t result = 0;
  if (aModifiers & mozilla::MODIFIER_SHIFT) {
    result |= nsIDOMWindowUtils::MODIFIER_SHIFT;
  }
  if (aModifiers & mozilla::MODIFIER_CONTROL) {
    result |= nsIDOMWindowUtils::MODIFIER_CONTROL;
  }
  if (aModifiers & mozilla::MODIFIER_ALT) {
    result |= nsIDOMWindowUtils::MODIFIER_ALT;
  }
  if (aModifiers & mozilla::MODIFIER_META) {
    result |= nsIDOMWindowUtils::MODIFIER_META;
  }
  if (aModifiers & mozilla::MODIFIER_ALTGRAPH) {
    result |= nsIDOMWindowUtils::MODIFIER_ALTGRAPH;
  }
  if (aModifiers & mozilla::MODIFIER_CAPSLOCK) {
    result |= nsIDOMWindowUtils::MODIFIER_CAPSLOCK;
  }
  if (aModifiers & mozilla::MODIFIER_FN) {
    result |= nsIDOMWindowUtils::MODIFIER_FN;
  }
  if (aModifiers & mozilla::MODIFIER_NUMLOCK) {
    result |= nsIDOMWindowUtils::MODIFIER_NUMLOCK;
  }
  if (aModifiers & mozilla::MODIFIER_SCROLLLOCK) {
    result |= nsIDOMWindowUtils::MODIFIER_SCROLLLOCK;
  }
  if (aModifiers & mozilla::MODIFIER_SYMBOLLOCK) {
    result |= nsIDOMWindowUtils::MODIFIER_SYMBOLLOCK;
  }
  if (aModifiers & mozilla::MODIFIER_OS) {
    result |= nsIDOMWindowUtils::MODIFIER_OS;
  }
  return result;
}

}

namespace mozilla {
namespace layers {

typedef mozilla::layers::AllowedTouchBehavior AllowedTouchBehavior;
typedef GeckoContentController::APZStateChange APZStateChange;































































































































































































static const uint32_t DefaultTouchBehavior = AllowedTouchBehavior::VERTICAL_PAN |
                                             AllowedTouchBehavior::HORIZONTAL_PAN |
                                             AllowedTouchBehavior::PINCH_ZOOM |
                                             AllowedTouchBehavior::DOUBLE_TAP_ZOOM;




static const double AXIS_LOCK_ANGLE = M_PI / 6.0; 




static const float AXIS_BREAKOUT_THRESHOLD = 1.0f/32.0f;




static const double AXIS_BREAKOUT_ANGLE = M_PI / 8.0; 








static const double ALLOWED_DIRECT_PAN_ANGLE = M_PI / 3.0; 




StaticAutoPtr<css::ComputedTimingFunction> gComputedTimingFunction;




static const CSSToScreenScale MAX_ZOOM(8.0f);




static const CSSToScreenScale MIN_ZOOM(0.125f);






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
  return FuzzyEqualsMultiplicative(aPoint.x, 0.0f)
      && FuzzyEqualsMultiplicative(aPoint.y, 0.0f);
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
static bool sThreadAssertionsEnabled = true;


static uint32_t sAsyncPanZoomControllerCount = 0;

static TimeStamp
GetFrameTime() {
  if (sFrameTime.IsNull()) {
    return TimeStamp::Now();
  }
  return sFrameTime;
}

class FlingAnimation: public AsyncPanZoomAnimation {
public:
  FlingAnimation(AsyncPanZoomController& aApzc,
                 bool aApplyAcceleration,
                 bool aAllowOverscroll)
    : AsyncPanZoomAnimation(TimeDuration::FromMilliseconds(gfxPrefs::APZFlingRepaintInterval()))
    , mApzc(aApzc)
    , mAllowOverscroll(aAllowOverscroll)
  {
    TimeStamp now = GetFrameTime();
    ScreenPoint velocity(mApzc.mX.GetVelocity(), mApzc.mY.GetVelocity());

    
    
    
    
    
    
    
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

  





  virtual bool Sample(FrameMetrics& aFrameMetrics,
                      const TimeDuration& aDelta) MOZ_OVERRIDE
  {
    
    
    
    
    
    
    
    
    if (aDelta.ToMilliseconds() <= 0) {
      return true;
    }

    bool overscrolled = mApzc.IsOverscrolled();
    float friction = overscrolled ? gfxPrefs::APZOverscrollFlingFriction()
                                  : gfxPrefs::APZFlingFriction();
    float threshold = overscrolled ? gfxPrefs::APZOverscrollFlingStoppedThreshold()
                                   : gfxPrefs::APZFlingStoppedThreshold();

    bool shouldContinueFlingX = mApzc.mX.FlingApplyFrictionOrCancel(aDelta, friction, threshold),
         shouldContinueFlingY = mApzc.mY.FlingApplyFrictionOrCancel(aDelta, friction, threshold);
    
    if (!shouldContinueFlingX && !shouldContinueFlingY) {
      APZC_LOG("%p ending fling animation. overscrolled=%d\n", &mApzc, mApzc.IsOverscrolled());
      
      if (mApzc.IsOverscrolled()) {
        mDeferredTasks.append(NewRunnableMethod(&mApzc, &AsyncPanZoomController::StartSnapBack));
      }
      return false;
    }

    
    
    
    
    ScreenPoint velocity(mApzc.mX.GetVelocity(), mApzc.mY.GetVelocity());

    ScreenPoint offset = velocity * aDelta.ToMilliseconds();

    
    
    CSSPoint cssOffset = offset / aFrameMetrics.GetZoom();

    
    
    
    
    CSSPoint overscroll;
    CSSPoint adjustedOffset;
    mApzc.mX.AdjustDisplacement(cssOffset.x, adjustedOffset.x, overscroll.x);
    mApzc.mY.AdjustDisplacement(cssOffset.y, adjustedOffset.y, overscroll.y);

    aFrameMetrics.ScrollBy(adjustedOffset);

    
    if (!IsZero(overscroll)) {
      if (mAllowOverscroll) {
        

        mApzc.OverscrollBy(overscroll);

        
        
        mApzc.mX.SetVelocity(velocity.x);
        mApzc.mY.SetVelocity(velocity.y);

      } else {
        
        

        
        
        
        if (FuzzyEqualsAdditive(overscroll.x, 0.0f, COORDINATE_EPSILON)) {
          velocity.x = 0;
        } else if (FuzzyEqualsAdditive(overscroll.y, 0.0f, COORDINATE_EPSILON)) {
          velocity.y = 0;
        }

        
        
        
        
        
        
        
        
        
        
        
        
        mDeferredTasks.append(NewRunnableMethod(&mApzc,
                                                &AsyncPanZoomController::HandleFlingOverscroll,
                                                velocity));
      }
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
  bool mAllowOverscroll;
};

class ZoomAnimation: public AsyncPanZoomAnimation {
public:
  ZoomAnimation(CSSPoint aStartOffset, CSSToScreenScale aStartZoom,
                CSSPoint aEndOffset, CSSToScreenScale aEndZoom)
    : mTotalDuration(TimeDuration::FromMilliseconds(gfxPrefs::APZZoomAnimationDuration()))
    , mStartOffset(aStartOffset)
    , mStartZoom(aStartZoom)
    , mEndOffset(aEndOffset)
    , mEndZoom(aEndZoom)
  {}

  virtual bool Sample(FrameMetrics& aFrameMetrics,
                      const TimeDuration& aDelta) MOZ_OVERRIDE
  {
    mDuration += aDelta;
    double animPosition = mDuration / mTotalDuration;

    if (animPosition >= 1.0) {
      aFrameMetrics.SetZoom(mEndZoom);
      aFrameMetrics.SetScrollOffset(mEndOffset);
      return false;
    }

    
    
    double sampledPosition = gComputedTimingFunction->GetValue(animPosition);

    
    
    aFrameMetrics.SetZoom(CSSToScreenScale(1 /
      (sampledPosition / mEndZoom.scale +
      (1 - sampledPosition) / mStartZoom.scale)));

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
  CSSToScreenScale mStartZoom;

  
  
  
  CSSPoint mEndOffset;
  CSSToScreenScale mEndZoom;
};

class OverscrollSnapBackAnimation: public AsyncPanZoomAnimation {
public:
  OverscrollSnapBackAnimation(AsyncPanZoomController& aApzc)
    : mApzc(aApzc)
  {
  }

  virtual bool Sample(FrameMetrics& aFrameMetrics,
                      const TimeDuration& aDelta) MOZ_OVERRIDE
  {
    
    bool continueX = mApzc.mX.SampleSnapBack(aDelta);
    bool continueY = mApzc.mY.SampleSnapBack(aDelta);
    return continueX || continueY;
  }
private:
  AsyncPanZoomController& mApzc;
};

void
AsyncPanZoomController::SetFrameTime(const TimeStamp& aTime) {
  sFrameTime = aTime;
}

void
AsyncPanZoomController::SetThreadAssertionsEnabled(bool aEnabled) {
  sThreadAssertionsEnabled = aEnabled;
}

bool
AsyncPanZoomController::GetThreadAssertionsEnabled() {
  return sThreadAssertionsEnabled;
}

 void
AsyncPanZoomController::InitializeGlobalState()
{
  MOZ_ASSERT(NS_IsMainThread());

  static bool sInitialized = false;
  if (sInitialized)
    return;
  sInitialized = true;

  gComputedTimingFunction = new css::ComputedTimingFunction();
  gComputedTimingFunction->Init(
    nsTimingFunction(NS_STYLE_TRANSITION_TIMING_FUNCTION_EASE));
  ClearOnShutdown(&gComputedTimingFunction);
}

AsyncPanZoomController::AsyncPanZoomController(uint64_t aLayersId,
                                               APZCTreeManager* aTreeManager,
                                               GeckoContentController* aGeckoContentController,
                                               GestureBehavior aGestures)
  :  mLayersId(aLayersId),
     mPaintThrottler(GetFrameTime()),
     mGeckoContentController(aGeckoContentController),
     mRefPtrMonitor("RefPtrMonitor"),
     mSharingFrameMetricsAcrossProcesses(false),
     mMonitor("AsyncPanZoomController"),
     mState(NOTHING),
     mContentResponseTimeoutTask(nullptr),
     mX(MOZ_THIS_IN_INITIALIZER_LIST()),
     mY(MOZ_THIS_IN_INITIALIZER_LIST()),
     mPanDirRestricted(false),
     mZoomConstraints(false, false, MIN_ZOOM, MAX_ZOOM),
     mLastSampleTime(GetFrameTime()),
     mLastAsyncScrollTime(GetFrameTime()),
     mLastAsyncScrollOffset(0, 0),
     mCurrentAsyncScrollOffset(0, 0),
     mAsyncScrollTimeoutTask(nullptr),
     mHandlingTouchQueue(false),
     mTreeManager(aTreeManager),
     mScrollParentId(FrameMetrics::NULL_SCROLL_ID),
     mAPZCId(sAsyncPanZoomControllerCount++),
     mSharedFrameMetricsBuffer(nullptr),
     mSharedLock(nullptr)
{
  MOZ_COUNT_CTOR(AsyncPanZoomController);

  if (aGestures == USE_GESTURE_DETECTOR) {
    mGestureEventListener = new GestureEventListener(this);
  }
}

AsyncPanZoomController::~AsyncPanZoomController() {
  MOZ_COUNT_DTOR(AsyncPanZoomController);
}

PCompositorParent*
AsyncPanZoomController::GetSharedFrameMetricsCompositor()
{
  if (GetThreadAssertionsEnabled()) {
    Compositor::AssertOnCompositorThread();
  }

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

void
AsyncPanZoomController::Destroy()
{
  CancelAnimation();

  { 
    MonitorAutoLock lock(mRefPtrMonitor);
    mGeckoContentController = nullptr;
    mGestureEventListener = nullptr;
  }
  mPrevSibling = nullptr;
  mLastChild = nullptr;
  mParent = nullptr;
  mTreeManager = nullptr;

  PCompositorParent* compositor = GetSharedFrameMetricsCompositor();
  
  if (compositor && mSharedFrameMetricsBuffer) {
    unused << compositor->SendReleaseSharedCompositorFrameMetrics(mFrameMetrics.GetScrollId(), mAPZCId);
  }

  { 
    ReentrantMonitorAutoEnter lock(mMonitor);
    delete mSharedFrameMetricsBuffer;
    mSharedFrameMetricsBuffer = nullptr;
    delete mSharedLock;
    mSharedLock = nullptr;
  }
}

bool
AsyncPanZoomController::IsDestroyed()
{
  return mTreeManager == nullptr;
}

float
AsyncPanZoomController::GetTouchStartTolerance()
{
  return (gfxPrefs::APZTouchStartTolerance() * APZCTreeManager::GetDPI());
}

AsyncPanZoomController::AxisLockMode AsyncPanZoomController::GetAxisLockMode()
{
  return static_cast<AxisLockMode>(gfxPrefs::APZAxisLockMode());
}

nsEventStatus AsyncPanZoomController::ReceiveInputEvent(const InputData& aEvent) {
  if (aEvent.mInputType == MULTITOUCH_INPUT &&
      aEvent.AsMultiTouchInput().mType == MultiTouchInput::MULTITOUCH_START) {
    
    mTouchBlockState = TouchBlockState();
  }

  
  
  
  
  
  
  
  if ((mFrameMetrics.mMayHaveTouchListeners || mFrameMetrics.mMayHaveTouchCaret) &&
      aEvent.mInputType == MULTITOUCH_INPUT &&
      (mState == NOTHING || mState == TOUCHING || IsPanningState(mState))) {
    const MultiTouchInput& multiTouchInput = aEvent.AsMultiTouchInput();
    if (multiTouchInput.mType == MultiTouchInput::MULTITOUCH_START) {
      SetState(WAITING_CONTENT_RESPONSE);
    }
  }

  if (mState == WAITING_CONTENT_RESPONSE || mHandlingTouchQueue) {
    if (aEvent.mInputType == MULTITOUCH_INPUT) {
      const MultiTouchInput& multiTouchInput = aEvent.AsMultiTouchInput();
      mTouchQueue.AppendElement(multiTouchInput);

      SetContentResponseTimer();
    }
    return nsEventStatus_eIgnore;
  }

  return HandleInputEvent(aEvent);
}

nsEventStatus AsyncPanZoomController::HandleInputEvent(const InputData& aEvent) {
  nsEventStatus rv = nsEventStatus_eIgnore;

  switch (aEvent.mInputType) {
  case MULTITOUCH_INPUT: {
    const MultiTouchInput& multiTouchInput = aEvent.AsMultiTouchInput();

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
    const PanGestureInput& panGestureInput = aEvent.AsPanGestureInput();
    switch (panGestureInput.mType) {
      case PanGestureInput::PANGESTURE_MAYSTART: rv = OnPanMayBegin(panGestureInput); break;
      case PanGestureInput::PANGESTURE_CANCELLED: rv = OnPanCancelled(panGestureInput); break;
      case PanGestureInput::PANGESTURE_START: rv = OnPanBegin(panGestureInput); break;
      case PanGestureInput::PANGESTURE_PAN: rv = OnPan(panGestureInput, true); break;
      case PanGestureInput::PANGESTURE_END: rv = OnPanEnd(panGestureInput); break;
      case PanGestureInput::PANGESTURE_MOMENTUMSTART: rv = OnPanMomentumStart(panGestureInput); break;
      case PanGestureInput::PANGESTURE_MOMENTUMPAN: rv = OnPan(panGestureInput, false); break;
      case PanGestureInput::PANGESTURE_MOMENTUMEND: rv = OnPanMomentumEnd(panGestureInput); break;
      default: NS_WARNING("Unhandled pan gesture"); break;
    }
    break;
  }
  default: NS_WARNING("Unhandled input event"); break;
  }

  return rv;
}

nsEventStatus AsyncPanZoomController::HandleGestureEvent(const InputData& aEvent)
{
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
  ScreenIntPoint point = GetFirstTouchScreenPoint(aEvent);

  switch (mState) {
    case FLING:
      if (GetVelocityVector().Length() > gfxPrefs::APZFlingStopOnTapThreshold()) {
        
        
        
        
        nsRefPtr<GestureEventListener> listener = GetGestureEventListener();
        if (listener) {
          listener->CancelSingleTouchDown();
        }
      }
      
    case ANIMATING_ZOOM:
      CancelAnimation();
      
    case NOTHING: {
      mX.StartTouch(point.x, aEvent.mTime);
      mY.StartTouch(point.y, aEvent.mTime);
      APZCTreeManager* treeManagerLocal = mTreeManager;
      nsRefPtr<GeckoContentController> controller = GetGeckoContentController();
      if (treeManagerLocal && controller) {
        bool touchCanBePan = treeManagerLocal->CanBePanned(this);
        controller->NotifyAPZStateChange(
            GetGuid(), APZStateChange::StartTouch, touchCanBePan);
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
    case WAITING_CONTENT_RESPONSE:
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
    case NOTHING:
    case ANIMATING_ZOOM:
      
      
      return nsEventStatus_eIgnore;

    case CROSS_SLIDING_X:
    case CROSS_SLIDING_Y:
      
      
      return nsEventStatus_eIgnore;

    case TOUCHING: {
      float panThreshold = GetTouchStartTolerance();
      UpdateWithTouchAtDevicePoint(aEvent);

      if (PanDistance() < panThreshold) {
        return nsEventStatus_eIgnore;
      }

      if (gfxPrefs::TouchActionEnabled() &&
          (GetTouchBehavior(0) & AllowedTouchBehavior::VERTICAL_PAN) &&
          (GetTouchBehavior(0) & AllowedTouchBehavior::HORIZONTAL_PAN)) {
        
        
        
        
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

    case WAITING_CONTENT_RESPONSE:
    case SNAP_BACK:  
                     
                     
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
  case NOTHING:
    
    
    return nsEventStatus_eIgnore;

  case TOUCHING:
  case CROSS_SLIDING_X:
  case CROSS_SLIDING_Y:
    SetState(NOTHING);
    return nsEventStatus_eIgnore;

  case PANNING:
  case PANNING_LOCKED_X:
  case PANNING_LOCKED_Y:
    {
      
      
      
      
      APZCTreeManager* treeManagerLocal = mTreeManager;
      if (treeManagerLocal) {
        if (!treeManagerLocal->FlushRepaintsForOverscrollHandoffChain()) {
          NS_WARNING("Overscroll handoff chain was empty during panning! This should not be the case.");
          
          FlushRepaintForOverscrollHandoff();
        }
      }
    }
    mX.EndTouch(aEvent.mTime);
    mY.EndTouch(aEvent.mTime);
    SetState(FLING);
    APZC_LOG("%p starting a fling animation\n", this);
    StartAnimation(new FlingAnimation(*this,
                                      true  ,
                                      false ));
    return nsEventStatus_eConsumeNoDefault;

  case PINCHING:
    SetState(NOTHING);
    
    NS_WARNING("Gesture listener should have handled pinching in OnTouchEnd.");
    return nsEventStatus_eIgnore;

  case WAITING_CONTENT_RESPONSE:
  case SNAP_BACK:  
                   
                   
    NS_WARNING("Received impossible touch in OnTouchEnd");
    break;
  }

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnTouchCancel(const MultiTouchInput& aEvent) {
  APZC_LOG("%p got a touch-cancel in state %d\n", this, mState);
  OnTouchEndOrCancel();
  SetState(NOTHING);
  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnScaleBegin(const PinchGestureInput& aEvent) {
  APZC_LOG("%p got a scale-begin in state %d\n", this, mState);

  if (!TouchActionAllowPinchZoom()) {
    return nsEventStatus_eIgnore;
  }

  if (!mZoomConstraints.mAllowZoom) {
    return nsEventStatus_eConsumeNoDefault;
  }

  SetState(PINCHING);
  mLastZoomFocus = ToParentLayerCoords(aEvent.mFocusPoint) - mFrameMetrics.mCompositionBounds.TopLeft();

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnScale(const PinchGestureInput& aEvent) {
  APZC_LOG("%p got a scale in state %d\n", this, mState);

  if (!TouchActionAllowPinchZoom()) {
    return nsEventStatus_eIgnore;
  }

  if (mState != PINCHING) {
    return nsEventStatus_eConsumeNoDefault;
  }

  float prevSpan = aEvent.mPreviousSpan;
  if (fabsf(prevSpan) <= EPSILON || fabsf(aEvent.mCurrentSpan) <= EPSILON) {
    
    return nsEventStatus_eConsumeNoDefault;
  }

  float spanRatio = aEvent.mCurrentSpan / aEvent.mPreviousSpan;

  {
    ReentrantMonitorAutoEnter lock(mMonitor);

    CSSToParentLayerScale userZoom = mFrameMetrics.GetZoomToParent();
    ParentLayerPoint focusPoint = ToParentLayerCoords(aEvent.mFocusPoint) - mFrameMetrics.mCompositionBounds.TopLeft();
    CSSPoint cssFocusPoint = focusPoint / userZoom;

    CSSPoint focusChange = (mLastZoomFocus - focusPoint) / userZoom;
    
    
    if (mX.DisplacementWillOverscroll(focusChange.x) != Axis::OVERSCROLL_NONE) {
      focusChange.x -= mX.DisplacementWillOverscrollAmount(focusChange.x);
    }
    if (mY.DisplacementWillOverscroll(focusChange.y) != Axis::OVERSCROLL_NONE) {
      focusChange.y -= mY.DisplacementWillOverscrollAmount(focusChange.y);
    }
    ScrollBy(focusChange);

    
    
    
    CSSPoint neededDisplacement;

    CSSToParentLayerScale realMinZoom = mZoomConstraints.mMinZoom * mFrameMetrics.mTransformScale;
    CSSToParentLayerScale realMaxZoom = mZoomConstraints.mMaxZoom * mFrameMetrics.mTransformScale;
    realMinZoom.scale = std::max(realMinZoom.scale,
                                 mFrameMetrics.mCompositionBounds.width / mFrameMetrics.mScrollableRect.width);
    realMinZoom.scale = std::max(realMinZoom.scale,
                                 mFrameMetrics.mCompositionBounds.height / mFrameMetrics.mScrollableRect.height);
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

  if (!TouchActionAllowPinchZoom()) {
    return nsEventStatus_eIgnore;
  }

  SetState(NOTHING);

  {
    ReentrantMonitorAutoEnter lock(mMonitor);

    
    
    
    
    
    mX.ClearOverscroll();
    mY.ClearOverscroll();

    ScheduleComposite();
    RequestContentRepaint();
    UpdateSharedCompositorFrameMetrics();
  }

  return nsEventStatus_eConsumeNoDefault;
}

bool
AsyncPanZoomController::ConvertToGecko(const ScreenPoint& aPoint, CSSPoint* aOut)
{
  APZCTreeManager* treeManagerLocal = mTreeManager;
  if (treeManagerLocal) {
    gfx3DMatrix transformToApzc;
    gfx3DMatrix transformToGecko;
    treeManagerLocal->GetInputTransforms(this, transformToApzc, transformToGecko);
    gfxPoint result = transformToGecko.Transform(gfxPoint(aPoint.x, aPoint.y));
    
    
    LayoutDevicePoint layoutPoint = LayoutDevicePoint(result.x, result.y);
    { 
      ReentrantMonitorAutoEnter lock(mMonitor);
      *aOut = layoutPoint / mFrameMetrics.mDevPixelsPerCSSPixel;
    }
    return true;
  }
  return false;
}

nsEventStatus AsyncPanZoomController::OnPanMayBegin(const PanGestureInput& aEvent) {
  APZC_LOG("%p got a pan-maybegin in state %d\n", this, mState);

  mX.StartTouch(aEvent.mPanStartPoint.x, aEvent.mTime);
  mY.StartTouch(aEvent.mPanStartPoint.y, aEvent.mTime);
  CancelAnimation();

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

  mX.StartTouch(aEvent.mPanStartPoint.x, aEvent.mTime);
  mY.StartTouch(aEvent.mPanStartPoint.y, aEvent.mTime);

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

nsEventStatus AsyncPanZoomController::OnPan(const PanGestureInput& aEvent, bool aFingersOnTouchpad) {
  APZC_LOG("%p got a pan-pan in state %d\n", this, mState);

  
  
  
  
  mX.UpdateWithTouchAtDevicePoint(aEvent.mPanStartPoint.x, aEvent.mTime);
  mY.UpdateWithTouchAtDevicePoint(aEvent.mPanStartPoint.y, aEvent.mTime);

  HandlePanningUpdate(aEvent.mPanDisplacement.x, aEvent.mPanDisplacement.y);

  CallDispatchScroll(aEvent.mPanStartPoint, aEvent.mPanStartPoint + aEvent.mPanDisplacement, 0);

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnPanEnd(const PanGestureInput& aEvent) {
  APZC_LOG("%p got a pan-end in state %d\n", this, mState);

  mX.EndTouch(aEvent.mTime);
  mY.EndTouch(aEvent.mTime);
  RequestContentRepaint();

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnPanMomentumStart(const PanGestureInput& aEvent) {
  APZC_LOG("%p got a pan-momentumstart in state %d\n", this, mState);

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnPanMomentumEnd(const PanGestureInput& aEvent) {
  APZC_LOG("%p got a pan-momentumend in state %d\n", this, mState);

  
  
  
  mX.CancelTouch();
  mY.CancelTouch();

  RequestContentRepaint();

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnLongPress(const TapGestureInput& aEvent) {
  APZC_LOG("%p got a long-press in state %d\n", this, mState);
  nsRefPtr<GeckoContentController> controller = GetGeckoContentController();
  if (controller) {
    int32_t modifiers = WidgetModifiersToDOMModifiers(aEvent.modifiers);
    CSSPoint geckoScreenPoint;
    if (ConvertToGecko(aEvent.mPoint, &geckoScreenPoint)) {
      SetState(WAITING_CONTENT_RESPONSE);
      SetContentResponseTimer();
      controller->HandleLongTap(geckoScreenPoint, modifiers, GetGuid());
      return nsEventStatus_eConsumeNoDefault;
    }
  }
  return nsEventStatus_eIgnore;
}

nsEventStatus AsyncPanZoomController::OnLongPressUp(const TapGestureInput& aEvent) {
  APZC_LOG("%p got a long-tap-up in state %d\n", this, mState);
  nsRefPtr<GeckoContentController> controller = GetGeckoContentController();
  if (controller) {
    int32_t modifiers = WidgetModifiersToDOMModifiers(aEvent.modifiers);
    CSSPoint geckoScreenPoint;
    if (ConvertToGecko(aEvent.mPoint, &geckoScreenPoint)) {
      controller->HandleLongTapUp(geckoScreenPoint, modifiers, GetGuid());
      return nsEventStatus_eConsumeNoDefault;
    }
  }
  return nsEventStatus_eIgnore;
}

nsEventStatus AsyncPanZoomController::GenerateSingleTap(const ScreenIntPoint& aPoint, mozilla::Modifiers aModifiers) {
  nsRefPtr<GeckoContentController> controller = GetGeckoContentController();
  if (controller) {
    CSSPoint geckoScreenPoint;
    if (ConvertToGecko(aPoint, &geckoScreenPoint)) {
      int32_t modifiers = WidgetModifiersToDOMModifiers(aModifiers);
      
      
      
      
      
      controller->PostDelayedTask(
        NewRunnableMethod(controller.get(), &GeckoContentController::HandleSingleTap,
                          geckoScreenPoint, modifiers, GetGuid()),
        0);
      mTouchBlockState.mSingleTapOccurred = true;
      return nsEventStatus_eConsumeNoDefault;
    }
  }
  return nsEventStatus_eIgnore;
}

void AsyncPanZoomController::OnTouchEndOrCancel() {
  if (nsRefPtr<GeckoContentController> controller = GetGeckoContentController()) {
    controller->NotifyAPZStateChange(
        GetGuid(), APZStateChange::EndTouch, mTouchBlockState.mSingleTapOccurred);
  }
}

nsEventStatus AsyncPanZoomController::OnSingleTapUp(const TapGestureInput& aEvent) {
  APZC_LOG("%p got a single-tap-up in state %d\n", this, mState);
  
  
  if (!(mZoomConstraints.mAllowDoubleTapZoom && TouchActionAllowDoubleTapZoom())) {
    return GenerateSingleTap(aEvent.mPoint, aEvent.modifiers);
  }
  return nsEventStatus_eIgnore;
}

nsEventStatus AsyncPanZoomController::OnSingleTapConfirmed(const TapGestureInput& aEvent) {
  APZC_LOG("%p got a single-tap-confirmed in state %d\n", this, mState);
  return GenerateSingleTap(aEvent.mPoint, aEvent.modifiers);
}

nsEventStatus AsyncPanZoomController::OnDoubleTap(const TapGestureInput& aEvent) {
  APZC_LOG("%p got a double-tap in state %d\n", this, mState);
  nsRefPtr<GeckoContentController> controller = GetGeckoContentController();
  if (controller) {
    if (mZoomConstraints.mAllowDoubleTapZoom && TouchActionAllowDoubleTapZoom()) {
      int32_t modifiers = WidgetModifiersToDOMModifiers(aEvent.modifiers);
      CSSPoint geckoScreenPoint;
      if (ConvertToGecko(aEvent.mPoint, &geckoScreenPoint)) {
        controller->HandleDoubleTap(geckoScreenPoint, modifiers, GetGuid());
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

float AsyncPanZoomController::PanDistance() {
  ReentrantMonitorAutoEnter lock(mMonitor);
  return NS_hypot(mX.PanDistance(), mY.PanDistance());
}

const ScreenPoint AsyncPanZoomController::GetVelocityVector() {
  return ScreenPoint(mX.GetVelocity(), mY.GetVelocity());
}

void AsyncPanZoomController::HandlePanningWithTouchAction(double aAngle, TouchBehaviorFlags aBehavior) {
  
  
  if ((aBehavior & AllowedTouchBehavior::VERTICAL_PAN) && (aBehavior & AllowedTouchBehavior::HORIZONTAL_PAN)) {
    if (mX.CanScrollNow() && mY.CanScrollNow()) {
      if (IsCloseToHorizontal(aAngle, AXIS_LOCK_ANGLE)) {
        mY.SetAxisLocked(true);
        SetState(PANNING_LOCKED_X);
      } else if (IsCloseToVertical(aAngle, AXIS_LOCK_ANGLE)) {
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
  } else if (aBehavior & AllowedTouchBehavior::HORIZONTAL_PAN) {
    
    
    if (IsCloseToHorizontal(aAngle, ALLOWED_DIRECT_PAN_ANGLE)) {
      mY.SetAxisLocked(true);
      SetState(PANNING_LOCKED_X);
      mPanDirRestricted = true;
    } else {
      
      
      SetState(NOTHING);
    }
  } else if (aBehavior & AllowedTouchBehavior::VERTICAL_PAN) {
    if (IsCloseToVertical(aAngle, ALLOWED_DIRECT_PAN_ANGLE)) {
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
  } else if (IsCloseToHorizontal(aAngle, AXIS_LOCK_ANGLE)) {
    mY.SetAxisLocked(true);
    if (mX.CanScrollNow()) {
      SetState(PANNING_LOCKED_X);
    } else {
      SetState(CROSS_SLIDING_X);
      mX.SetAxisLocked(true);
    }
  } else if (IsCloseToVertical(aAngle, AXIS_LOCK_ANGLE)) {
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

void AsyncPanZoomController::HandlePanningUpdate(float aDX, float aDY) {
  
  if (GetAxisLockMode() == STICKY && !mPanDirRestricted) {

    double angle = atan2(aDY, aDX); 
    angle = fabs(angle); 

    float breakThreshold = AXIS_BREAKOUT_THRESHOLD * APZCTreeManager::GetDPI();

    if (fabs(aDX) > breakThreshold || fabs(aDY) > breakThreshold) {
      if (mState == PANNING_LOCKED_X || mState == CROSS_SLIDING_X) {
        if (!IsCloseToHorizontal(angle, AXIS_BREAKOUT_ANGLE)) {
          mY.SetAxisLocked(false);
          SetState(PANNING);
        }
      } else if (mState == PANNING_LOCKED_Y || mState == CROSS_SLIDING_Y) {
        if (!IsCloseToVertical(angle, AXIS_BREAKOUT_ANGLE)) {
          mX.SetAxisLocked(false);
          SetState(PANNING);
        }
      }
    }
  }
}

nsEventStatus AsyncPanZoomController::StartPanning(const MultiTouchInput& aEvent) {
  ReentrantMonitorAutoEnter lock(mMonitor);

  ScreenIntPoint point = GetFirstTouchScreenPoint(aEvent);
  float dx = mX.PanDistance(point.x);
  float dy = mY.PanDistance(point.y);

  
  
  mX.StartTouch(point.x, aEvent.mTime);
  mY.StartTouch(point.y, aEvent.mTime);

  double angle = atan2(dy, dx); 
  angle = fabs(angle); 

  if (gfxPrefs::TouchActionEnabled()) {
    HandlePanningWithTouchAction(angle, GetTouchBehavior(0));
  } else {
    if (GetAxisLockMode() == FREE) {
      SetState(PANNING);
    } else {
      HandlePanning(angle);
    }
  }

  if (IsPanningState(mState)) {
    if (nsRefPtr<GeckoContentController> controller = GetGeckoContentController()) {
      controller->NotifyAPZStateChange(GetGuid(), APZStateChange::StartPanning);
    }
    return nsEventStatus_eConsumeNoDefault;
  }
  
  return nsEventStatus_eIgnore;
}

void AsyncPanZoomController::UpdateWithTouchAtDevicePoint(const MultiTouchInput& aEvent) {
  ScreenIntPoint point = GetFirstTouchScreenPoint(aEvent);
  mX.UpdateWithTouchAtDevicePoint(point.x, aEvent.mTime);
  mY.UpdateWithTouchAtDevicePoint(point.y, aEvent.mTime);
}

bool AsyncPanZoomController::AttemptScroll(const ScreenPoint& aStartPoint,
                                           const ScreenPoint& aEndPoint,
                                           uint32_t aOverscrollHandoffChainIndex) {

  
  
  
  ScreenPoint displacement = aStartPoint - aEndPoint;

  ScreenPoint overscroll;  
  CSSPoint cssOverscroll;  
  {
    ReentrantMonitorAutoEnter lock(mMonitor);

    CSSToScreenScale zoom = mFrameMetrics.GetZoom();

    
    
    CSSPoint cssDisplacement = displacement / zoom;

    CSSPoint adjustedDisplacement;
    bool xChanged = mX.AdjustDisplacement(cssDisplacement.x, adjustedDisplacement.x, cssOverscroll.x);
    bool yChanged = mY.AdjustDisplacement(cssDisplacement.y, adjustedDisplacement.y, cssOverscroll.y);
    if (xChanged || yChanged) {
      ScheduleComposite();
    }

    overscroll = cssOverscroll * zoom;

    if (!IsZero(adjustedDisplacement)) {
      ScrollBy(adjustedDisplacement);
      ScheduleCompositeAndMaybeRepaint();
      UpdateSharedCompositorFrameMetrics();
    }
  }

  
  if (IsZero(overscroll)) {
    return true;
  }

  
  
  
  
  
  if (CallDispatchScroll(aEndPoint + overscroll, aEndPoint, aOverscrollHandoffChainIndex + 1)) {
    return true;
  }

  
  
  
  APZC_LOG("%p taking overscroll during panning\n", this);
  return OverscrollBy(cssOverscroll);
}

bool AsyncPanZoomController::OverscrollBy(const CSSPoint& aOverscroll) {
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

void AsyncPanZoomController::AcceptFling(const ScreenPoint& aVelocity,
                                         bool aAllowOverscroll) {
  
  
  mX.SetVelocity(mX.GetVelocity() + aVelocity.x);
  mY.SetVelocity(mY.GetVelocity() + aVelocity.y);
  SetState(FLING);
  StartAnimation(new FlingAnimation(*this, false ,
                                    aAllowOverscroll));
}

bool AsyncPanZoomController::TakeOverFling(ScreenPoint aVelocity) {
  
  if (IsPannable()) {
    AcceptFling(aVelocity, false );
    return true;
  }

  
  
  
  
  
  APZCTreeManager* treeManagerLocal = mTreeManager;
  return treeManagerLocal
      && treeManagerLocal->HandOffFling(this, aVelocity);
}

void AsyncPanZoomController::HandleFlingOverscroll(const ScreenPoint& aVelocity) {
  APZCTreeManager* treeManagerLocal = mTreeManager;
  if (!(treeManagerLocal && treeManagerLocal->HandOffFling(this, aVelocity))) {
    
    if (IsPannable()) {
      AcceptFling(aVelocity, true );
    }
  }
}

void AsyncPanZoomController::StartSnapBack() {
  SetState(SNAP_BACK);
  StartAnimation(new OverscrollSnapBackAnimation(*this));
}

bool AsyncPanZoomController::CallDispatchScroll(const ScreenPoint& aStartPoint, const ScreenPoint& aEndPoint,
                                                uint32_t aOverscrollHandoffChainIndex) {
  
  
  
  APZCTreeManager* treeManagerLocal = mTreeManager;
  return treeManagerLocal
      && treeManagerLocal->DispatchScroll(this, aStartPoint, aEndPoint,
                                          aOverscrollHandoffChainIndex);
}

void AsyncPanZoomController::TrackTouch(const MultiTouchInput& aEvent) {
  ScreenIntPoint prevTouchPoint(mX.GetPos(), mY.GetPos());
  ScreenIntPoint touchPoint = GetFirstTouchScreenPoint(aEvent);

  float dx = mX.PanDistance(touchPoint.x);
  float dy = mY.PanDistance(touchPoint.y);
  HandlePanningUpdate(dx, dy);

  UpdateWithTouchAtDevicePoint(aEvent);

  if (prevTouchPoint != touchPoint) {
    CallDispatchScroll(prevTouchPoint, touchPoint, 0);
  }
}

ScreenIntPoint& AsyncPanZoomController::GetFirstTouchScreenPoint(const MultiTouchInput& aEvent) {
  return ((SingleTouchData&)aEvent.mTouches[0]).mScreenPoint;
}

void AsyncPanZoomController::StartAnimation(AsyncPanZoomAnimation* aAnimation)
{
  ReentrantMonitorAutoEnter lock(mMonitor);
  mAnimation = aAnimation;
  mLastSampleTime = GetFrameTime();
  ScheduleComposite();
}

void AsyncPanZoomController::CancelAnimation() {
  ReentrantMonitorAutoEnter lock(mMonitor);
  APZC_LOG("%p running CancelAnimation in state %d\n", this, mState);
  SetState(NOTHING);
  mAnimation = nullptr;
  
  
  
  if (mX.IsOverscrolled() || mY.IsOverscrolled()) {
    mX.ClearOverscroll();
    mY.ClearOverscroll();
    RequestContentRepaint();
    ScheduleComposite();
    UpdateSharedCompositorFrameMetrics();
  }
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


const LayerMargin AsyncPanZoomController::CalculatePendingDisplayPort(
  const FrameMetrics& aFrameMetrics,
  const ScreenPoint& aVelocity,
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
    "Calculated displayport as (%f %f %f %f) from velocity (%f %f) paint time %f metrics",
    displayPort.x, displayPort.y, displayPort.width, displayPort.height,
    aVelocity.x, aVelocity.y, (float)estimatedPaintDurationMillis);

  CSSMargin cssMargins;
  cssMargins.left = -displayPort.x;
  cssMargins.top = -displayPort.y;
  cssMargins.right = displayPort.width - compositionSize.width - cssMargins.left;
  cssMargins.bottom = displayPort.height - compositionSize.height - cssMargins.top;

  LayerMargin layerMargins = cssMargins * aFrameMetrics.LayersPixelsPerCSSPixel();

  return layerMargins;
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

void AsyncPanZoomController::RequestContentRepaint(FrameMetrics& aFrameMetrics) {
  aFrameMetrics.SetDisplayPortMargins(
    CalculatePendingDisplayPort(aFrameMetrics,
                                GetVelocityVector(),
                                mPaintThrottler.AverageDuration().ToSeconds()));
  aFrameMetrics.SetUseDisplayPortMargins();

  
  
  LayerMargin marginDelta = mLastPaintRequestMetrics.GetDisplayPortMargins()
                          - aFrameMetrics.GetDisplayPortMargins();
  if (fabsf(marginDelta.left) < EPSILON &&
      fabsf(marginDelta.top) < EPSILON &&
      fabsf(marginDelta.right) < EPSILON &&
      fabsf(marginDelta.bottom) < EPSILON &&
      fabsf(mLastPaintRequestMetrics.GetScrollOffset().x -
            aFrameMetrics.GetScrollOffset().x) < EPSILON &&
      fabsf(mLastPaintRequestMetrics.GetScrollOffset().y -
            aFrameMetrics.GetScrollOffset().y) < EPSILON &&
      aFrameMetrics.GetZoom() == mLastPaintRequestMetrics.GetZoom() &&
      fabsf(aFrameMetrics.mViewport.width - mLastPaintRequestMetrics.mViewport.width) < EPSILON &&
      fabsf(aFrameMetrics.mViewport.height - mLastPaintRequestMetrics.mViewport.height) < EPSILON) {
    return;
  }

  SendAsyncScrollEvent();
  mPaintThrottler.PostTask(
    FROM_HERE,
    NewRunnableMethod(this,
                      &AsyncPanZoomController::DispatchRepaintRequest,
                      aFrameMetrics),
    GetFrameTime());

  aFrameMetrics.SetPresShellId(mLastContentPaintMetrics.GetPresShellId());
  mLastPaintRequestMetrics = aFrameMetrics;
}

 CSSRect
GetDisplayPortRect(const FrameMetrics& aFrameMetrics)
{
  
  
  CSSRect baseRect(aFrameMetrics.GetScrollOffset(),
                   aFrameMetrics.CalculateBoundedCompositedSizeInCssPixels());
  baseRect.Inflate(aFrameMetrics.GetDisplayPortMargins() / aFrameMetrics.LayersPixelsPerCSSPixel());
  return baseRect;
}

void
AsyncPanZoomController::DispatchRepaintRequest(const FrameMetrics& aFrameMetrics) {
  nsRefPtr<GeckoContentController> controller = GetGeckoContentController();
  if (controller) {
    APZC_LOG_FM(aFrameMetrics, "%p requesting content repaint", this);
    LogRendertraceRect(GetGuid(), "requested displayport", "yellow", GetDisplayPortRect(aFrameMetrics));

    controller->RequestContentRepaint(aFrameMetrics);
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
  if (mAnimation) {
    bool continueAnimation = mAnimation->Sample(mFrameMetrics, aSampleTime - mLastSampleTime);
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
    mLastSampleTime = aSampleTime;
    return true;
  }
  return false;
}

void AsyncPanZoomController::GetOverscrollTransform(ViewTransform* aTransform) const {
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  const float kClamping = gfxPrefs::APZOverscrollClamping();

  
  
  
  
  
  float spacePropX = kClamping * fabsf(mX.GetOverscroll()) / mX.GetCompositionLength();
  float spacePropY = kClamping * fabsf(mY.GetOverscroll()) / mY.GetCompositionLength();

  
  
  
  
  const float kZEffect = gfxPrefs::APZOverscrollZEffect();

  
  CSSPoint translationX;
  if (mX.IsOverscrolled()) {
    
    translationX.y = (spacePropX * kZEffect * mY.GetCompositionLength()) / 2;

    if (mX.GetOverscroll() < 0) {
      
      translationX.x = spacePropX * mX.GetCompositionLength();
    } else {
      
      
      
      
      translationX.x = - (spacePropX * (1 - kZEffect) * mX.GetCompositionLength());
    }
  }

  
  CSSPoint translationY;
  if (mY.IsOverscrolled()) {
    
    translationY.x = (spacePropY * kZEffect * mX.GetCompositionLength()) / 2;

    if (mY.GetOverscroll() < 0) {
      
      translationY.y = spacePropY * mY.GetCompositionLength();
    } else {
      
      
      
      
      translationY.y = - (spacePropY * (1 - kZEffect) * mY.GetCompositionLength());
    }
  }

  
  float spaceProp = sqrtf(spacePropX * spacePropX + spacePropY * spacePropY);
  CSSPoint translation = translationX + translationY;

  float scale = 1 - (kZEffect * spaceProp);

  
  
  translation.x /= scale;
  translation.y /= scale;

  
  aTransform->mScale.scale *= scale;
  aTransform->mTranslation += translation * mFrameMetrics.LayersPixelsPerCSSPixel();
}

bool AsyncPanZoomController::SampleContentTransformForFrame(const TimeStamp& aSampleTime,
                                                            ViewTransform* aOutTransform,
                                                            ScreenPoint& aScrollOffset,
                                                            ViewTransform* aOutOverscrollTransform) {
  
  
  
  
  
  bool requestAnimationFrame = false;
  Vector<Task*> deferredTasks;

  {
    ReentrantMonitorAutoEnter lock(mMonitor);

    requestAnimationFrame = UpdateAnimation(aSampleTime, &deferredTasks);

    aScrollOffset = mFrameMetrics.GetScrollOffset() * mFrameMetrics.GetZoom();
    *aOutTransform = GetCurrentAsyncTransform();

    
    
    if (aOutOverscrollTransform && IsOverscrolled()) {
      GetOverscrollTransform(aOutOverscrollTransform);

      
      
      
      
      
      
      
      aOutOverscrollTransform->mTranslation.x *= aOutTransform->mScale.scale;
      aOutOverscrollTransform->mTranslation.y *= aOutTransform->mScale.scale;
    }

    LogRendertraceRect(GetGuid(), "viewport", "red",
      CSSRect(mFrameMetrics.GetScrollOffset(),
              mFrameMetrics.CalculateCompositedSizeInCssPixels()));

    mCurrentAsyncScrollOffset = mFrameMetrics.GetScrollOffset();
  }

  
  
  
  
  for (uint32_t i = 0; i < deferredTasks.length(); ++i) {
    deferredTasks[i]->Run();
    delete deferredTasks[i];
  }

  
  
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
  }
  else {
    mAsyncScrollTimeoutTask =
      NewRunnableMethod(this, &AsyncPanZoomController::FireAsyncScrollOnTimeout);
    MessageLoop::current()->PostDelayedTask(FROM_HERE,
                                            mAsyncScrollTimeoutTask,
                                            gfxPrefs::APZAsyncScrollTimeout());
  }

  return requestAnimationFrame;
}

ViewTransform AsyncPanZoomController::GetCurrentAsyncTransform() {
  ReentrantMonitorAutoEnter lock(mMonitor);

  CSSPoint lastPaintScrollOffset;
  if (mLastContentPaintMetrics.IsScrollable()) {
    lastPaintScrollOffset = mLastContentPaintMetrics.GetScrollOffset();
  }

  CSSPoint currentScrollOffset = mFrameMetrics.GetScrollOffset() +
    mTestAsyncScrollOffset;

  
  
  if (!gfxPrefs::APZAllowCheckerboarding() &&
      !mLastContentPaintMetrics.mDisplayPort.IsEmpty()) {
    CSSSize compositedSize = mLastContentPaintMetrics.CalculateCompositedSizeInCssPixels();
    CSSPoint maxScrollOffset = lastPaintScrollOffset +
      CSSPoint(mLastContentPaintMetrics.mDisplayPort.XMost() - compositedSize.width,
               mLastContentPaintMetrics.mDisplayPort.YMost() - compositedSize.height);
    CSSPoint minScrollOffset = lastPaintScrollOffset + mLastContentPaintMetrics.mDisplayPort.TopLeft();

    if (minScrollOffset.x < maxScrollOffset.x) {
      currentScrollOffset.x = clamped(currentScrollOffset.x, minScrollOffset.x, maxScrollOffset.x);
    }
    if (minScrollOffset.y < maxScrollOffset.y) {
      currentScrollOffset.y = clamped(currentScrollOffset.y, minScrollOffset.y, maxScrollOffset.y);
    }
  }

  LayerPoint translation = (currentScrollOffset - lastPaintScrollOffset)
                         * mLastContentPaintMetrics.LayersPixelsPerCSSPixel();

  return ViewTransform(-translation,
                       mFrameMetrics.GetZoom()
                     / mLastContentPaintMetrics.mDevPixelsPerCSSPixel
                     / mFrameMetrics.GetParentResolution());
}

gfx3DMatrix AsyncPanZoomController::GetNontransientAsyncTransform() {
  ReentrantMonitorAutoEnter lock(mMonitor);
  return gfx3DMatrix::ScalingMatrix(mLastContentPaintMetrics.mResolution.scale,
                                    mLastContentPaintMetrics.mResolution.scale,
                                    1.0f);
}

gfx3DMatrix AsyncPanZoomController::GetTransformToLastDispatchedPaint() {
  ReentrantMonitorAutoEnter lock(mMonitor);
  LayerPoint scrollChange = (mLastContentPaintMetrics.GetScrollOffset() - mLastDispatchedPaintMetrics.GetScrollOffset())
                          * mLastContentPaintMetrics.LayersPixelsPerCSSPixel();
  float zoomChange = mLastContentPaintMetrics.GetZoom().scale / mLastDispatchedPaintMetrics.GetZoom().scale;
  return gfx3DMatrix::Translation(scrollChange.x, scrollChange.y, 0) *
         gfx3DMatrix::ScalingMatrix(zoomChange, zoomChange, 1);
}

void AsyncPanZoomController::NotifyLayersUpdated(const FrameMetrics& aLayerMetrics, bool aIsFirstPaint) {
  ReentrantMonitorAutoEnter lock(mMonitor);
  bool isDefault = mFrameMetrics.IsDefault();

  mLastContentPaintMetrics = aLayerMetrics;
  UpdateTransformScale();

  mFrameMetrics.mMayHaveTouchListeners = aLayerMetrics.mMayHaveTouchListeners;
  mFrameMetrics.mMayHaveTouchCaret = aLayerMetrics.mMayHaveTouchCaret;
  APZC_LOG_FM(aLayerMetrics, "%p got a NotifyLayersUpdated with aIsFirstPaint=%d", this, aIsFirstPaint);

  LogRendertraceRect(GetGuid(), "page", "brown", aLayerMetrics.mScrollableRect);
  LogRendertraceRect(GetGuid(), "painted displayport", "lightgreen",
    aLayerMetrics.mDisplayPort + aLayerMetrics.GetScrollOffset());
  if (!aLayerMetrics.mCriticalDisplayPort.IsEmpty()) {
    LogRendertraceRect(GetGuid(), "painted critical displayport", "darkgreen",
      aLayerMetrics.mCriticalDisplayPort + aLayerMetrics.GetScrollOffset());
  }

  mPaintThrottler.TaskComplete(GetFrameTime());
  bool needContentRepaint = false;
  if (FuzzyEqualsAdditive(aLayerMetrics.mCompositionBounds.width, mFrameMetrics.mCompositionBounds.width) &&
      FuzzyEqualsAdditive(aLayerMetrics.mCompositionBounds.height, mFrameMetrics.mCompositionBounds.height)) {
    
    
    if (mFrameMetrics.mViewport.width != aLayerMetrics.mViewport.width ||
        mFrameMetrics.mViewport.height != aLayerMetrics.mViewport.height) {
      needContentRepaint = true;
    }
    mFrameMetrics.mViewport = aLayerMetrics.mViewport;
  }

  
  
  
  
  bool scrollOffsetUpdated = aLayerMetrics.GetScrollOffsetUpdated()
        && (aLayerMetrics.GetScrollGeneration() != mFrameMetrics.GetScrollGeneration());

  if (aIsFirstPaint || isDefault) {
    
    
    mPaintThrottler.ClearHistory();
    mPaintThrottler.SetMaxDurations(gfxPrefs::APZNumPaintDurationSamples());

    mX.CancelTouch();
    mY.CancelTouch();
    SetState(NOTHING);

    mFrameMetrics = aLayerMetrics;
    mLastDispatchedPaintMetrics = aLayerMetrics;
    ShareCompositorFrameMetrics();
  } else {
    
    
    

    if (FuzzyEqualsAdditive(mFrameMetrics.mCompositionBounds.width, aLayerMetrics.mCompositionBounds.width) &&
        mFrameMetrics.mDevPixelsPerCSSPixel == aLayerMetrics.mDevPixelsPerCSSPixel) {
      float parentResolutionChange = aLayerMetrics.GetParentResolution().scale
                                   / mFrameMetrics.GetParentResolution().scale;
      mFrameMetrics.ZoomBy(parentResolutionChange);
    } else {
      
      
      mFrameMetrics.SetZoom(aLayerMetrics.GetZoom());
      mFrameMetrics.mDevPixelsPerCSSPixel.scale = aLayerMetrics.mDevPixelsPerCSSPixel.scale;
    }
    if (!mFrameMetrics.mScrollableRect.IsEqualEdges(aLayerMetrics.mScrollableRect)) {
      mFrameMetrics.mScrollableRect = aLayerMetrics.mScrollableRect;
      needContentRepaint = true;
    }
    mFrameMetrics.mCompositionBounds = aLayerMetrics.mCompositionBounds;
    mFrameMetrics.SetRootCompositionSize(aLayerMetrics.GetRootCompositionSize());
    mFrameMetrics.mResolution = aLayerMetrics.mResolution;
    mFrameMetrics.mCumulativeResolution = aLayerMetrics.mCumulativeResolution;
    mFrameMetrics.SetHasScrollgrab(aLayerMetrics.GetHasScrollgrab());

    if (scrollOffsetUpdated) {
      APZC_LOG("%p updating scroll offset from (%f, %f) to (%f, %f)\n", this,
        mFrameMetrics.GetScrollOffset().x, mFrameMetrics.GetScrollOffset().y,
        aLayerMetrics.GetScrollOffset().x, aLayerMetrics.GetScrollOffset().y);

      mFrameMetrics.CopyScrollInfoFrom(aLayerMetrics);

      
      
      
      CancelAnimation();

      
      
      
      
      
      mLastDispatchedPaintMetrics = aLayerMetrics;
    }
  }

  if (scrollOffsetUpdated) {
    
    
    
    
    nsRefPtr<GeckoContentController> controller = GetGeckoContentController();
    if (controller) {
      APZC_LOG("%p sending scroll update acknowledgement with gen %lu\n", this, aLayerMetrics.GetScrollGeneration());
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

void AsyncPanZoomController::ZoomToRect(CSSRect aRect) {
  if (!aRect.IsFinite()) {
    NS_WARNING("ZoomToRect got called with a non-finite rect; ignoring...\n");
    return;
  }

  SetState(ANIMATING_ZOOM);

  {
    ReentrantMonitorAutoEnter lock(mMonitor);

    ParentLayerRect compositionBounds = mFrameMetrics.mCompositionBounds;
    CSSRect cssPageRect = mFrameMetrics.mScrollableRect;
    CSSPoint scrollOffset = mFrameMetrics.GetScrollOffset();
    CSSToParentLayerScale currentZoom = mFrameMetrics.GetZoomToParent();
    CSSToParentLayerScale targetZoom;

    
    
    
    
    
    CSSToParentLayerScale localMinZoom(std::max((mZoomConstraints.mMinZoom * mFrameMetrics.mTransformScale).scale,
                                       std::max(compositionBounds.width / cssPageRect.width,
                                                compositionBounds.height / cssPageRect.height)));
    CSSToParentLayerScale localMaxZoom = mZoomConstraints.mMaxZoom * mFrameMetrics.mTransformScale;

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
    endZoomToMetrics.SetZoom(targetZoom / mFrameMetrics.mTransformScale);

    
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
                                  ScreenPoint(0,0),
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

void AsyncPanZoomController::ContentReceivedTouch(bool aPreventDefault) {
  mTouchBlockState.mPreventDefaultSet = true;
  mTouchBlockState.mPreventDefault = aPreventDefault;
  CheckContentResponse();
}

void AsyncPanZoomController::CheckContentResponse() {
  bool canProceedToTouchState = true;

  if (mFrameMetrics.mMayHaveTouchListeners ||
      mFrameMetrics.mMayHaveTouchCaret) {
    canProceedToTouchState &= mTouchBlockState.mPreventDefaultSet;
  }

  if (gfxPrefs::TouchActionEnabled()) {
    canProceedToTouchState &= mTouchBlockState.mAllowedTouchBehaviorSet;
  }

  if (!canProceedToTouchState) {
    return;
  }

  if (mContentResponseTimeoutTask) {
    mContentResponseTimeoutTask->Cancel();
    mContentResponseTimeoutTask = nullptr;
  }

  if (mState == WAITING_CONTENT_RESPONSE) {
    if (!mTouchBlockState.mPreventDefault) {
      SetState(NOTHING);
    }

    mHandlingTouchQueue = true;

    while (!mTouchQueue.IsEmpty()) {
      if (!mTouchBlockState.mPreventDefault) {
        HandleInputEvent(mTouchQueue[0]);
      }

      if (mTouchQueue[0].mType == MultiTouchInput::MULTITOUCH_END ||
          mTouchQueue[0].mType == MultiTouchInput::MULTITOUCH_CANCEL) {
        mTouchQueue.RemoveElementAt(0);
        break;
      }

      mTouchQueue.RemoveElementAt(0);
    }

    mHandlingTouchQueue = false;
  }
}

bool AsyncPanZoomController::TouchActionAllowPinchZoom() {
  if (!gfxPrefs::TouchActionEnabled()) {
    return true;
  }
  
  
  for (size_t i = 0; i < mTouchBlockState.mAllowedTouchBehaviors.Length(); i++) {
    if (!(mTouchBlockState.mAllowedTouchBehaviors[i] & AllowedTouchBehavior::PINCH_ZOOM)) {
      return false;
    }
  }
  return true;
}

bool AsyncPanZoomController::TouchActionAllowDoubleTapZoom() {
  if (!gfxPrefs::TouchActionEnabled()) {
    return true;
  }
  for (size_t i = 0; i < mTouchBlockState.mAllowedTouchBehaviors.Length(); i++) {
    if (!(mTouchBlockState.mAllowedTouchBehaviors[i] & AllowedTouchBehavior::DOUBLE_TAP_ZOOM)) {
      return false;
    }
  }
  return true;
}

AsyncPanZoomController::TouchBehaviorFlags
AsyncPanZoomController::GetTouchBehavior(uint32_t touchIndex) {
  if (touchIndex < mTouchBlockState.mAllowedTouchBehaviors.Length()) {
    return mTouchBlockState.mAllowedTouchBehaviors[touchIndex];
  }
  return DefaultTouchBehavior;
}

AsyncPanZoomController::TouchBehaviorFlags
AsyncPanZoomController::GetAllowedTouchBehavior(ScreenIntPoint& aPoint) {
  
  
  
  return AllowedTouchBehavior::UNKNOWN;
}

void AsyncPanZoomController::SetAllowedTouchBehavior(const nsTArray<TouchBehaviorFlags>& aBehaviors) {
  mTouchBlockState.mAllowedTouchBehaviors.Clear();
  mTouchBlockState.mAllowedTouchBehaviors.AppendElements(aBehaviors);
  mTouchBlockState.mAllowedTouchBehaviorSet = true;
  CheckContentResponse();
}

void AsyncPanZoomController::SetState(PanZoomState aNewState) {

  PanZoomState oldState;

  
  {
    ReentrantMonitorAutoEnter lock(mMonitor);
    oldState = mState;
    mState = aNewState;
  }

  if (nsRefPtr<GeckoContentController> controller = GetGeckoContentController()) {
    if (!IsTransformingState(oldState) && IsTransformingState(aNewState)) {
      controller->NotifyAPZStateChange(
          GetGuid(), APZStateChange::TransformBegin);
    } else if (IsTransformingState(oldState) && !IsTransformingState(aNewState)) {
      controller->NotifyAPZStateChange(
          GetGuid(), APZStateChange::TransformEnd);
    }
  }
}

bool AsyncPanZoomController::IsTransformingState(PanZoomState aState) {
  return !(aState == NOTHING || aState == TOUCHING || aState == WAITING_CONTENT_RESPONSE);
}

bool AsyncPanZoomController::IsPanningState(PanZoomState aState) {
  return (aState == PANNING || aState == PANNING_LOCKED_X || aState == PANNING_LOCKED_Y);
}

void AsyncPanZoomController::SetContentResponseTimer() {
  if (!mContentResponseTimeoutTask) {
    mContentResponseTimeoutTask =
      NewRunnableMethod(this, &AsyncPanZoomController::TimeoutContentResponse);

    PostDelayedTask(mContentResponseTimeoutTask, gfxPrefs::APZContentResponseTimeout());
  }
}

void AsyncPanZoomController::TimeoutContentResponse() {
  mContentResponseTimeoutTask = nullptr;
  ContentReceivedTouch(false);
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

    isRoot = mFrameMetrics.mIsRoot;
    scrollableSize = mFrameMetrics.mScrollableRect.Size();
    contentRect = mFrameMetrics.CalculateCompositedRectInCssPixels();
    contentRect.MoveTo(mCurrentAsyncScrollOffset);
  }

  controller->SendAsyncScrollDOMEvent(isRoot, contentRect, scrollableSize);
}

bool AsyncPanZoomController::Matches(const ScrollableLayerGuid& aGuid)
{
  return aGuid == GetGuid();
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

  if (frame && mSharedLock && gfxPrefs::UseProgressiveTilePainting()) {
    mSharedLock->Lock();
    *frame = mFrameMetrics;
    mSharedLock->Unlock();
  }
}

void AsyncPanZoomController::ShareCompositorFrameMetrics() {

  PCompositorParent* compositor = GetSharedFrameMetricsCompositor();

  
  
  
  if (!mSharedFrameMetricsBuffer && compositor && gfxPrefs::UseProgressiveTilePainting()) {

    
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

      
      base::ProcessHandle processHandle = compositor->OtherProcess();
      ipc::SharedMemoryBasic::Handle mem = ipc::SharedMemoryBasic::NULLHandle();

      
      mSharedFrameMetricsBuffer->ShareToProcess(processHandle, &mem);

      
      mSharedLock = new CrossProcessMutex("AsyncPanZoomControlLock");
      CrossProcessMutexHandle handle = mSharedLock->ShareToProcess(processHandle);

      
      
      
      if (!compositor->SendSharedCompositorFrameMetrics(mem, handle, mAPZCId)) {
        APZC_LOG("%p failed to share FrameMetrics with content process.", this);
      }
    }
  }
}

ParentLayerPoint AsyncPanZoomController::ToParentLayerCoords(const ScreenPoint& aPoint)
{
  return TransformTo<ParentLayerPixel>(GetNontransientAsyncTransform() * GetCSSTransform(), aPoint);
}

void AsyncPanZoomController::UpdateTransformScale()
{
  gfx3DMatrix nontransientTransforms = GetNontransientAsyncTransform() * GetCSSTransform();
  if (!FuzzyEqualsMultiplicative(nontransientTransforms.GetXScale(), nontransientTransforms.GetYScale())) {
    NS_WARNING("The x- and y-scales of the nontransient transforms should be equal");
  }
  mFrameMetrics.mTransformScale.scale = nontransientTransforms.GetXScale();
}

}
}
