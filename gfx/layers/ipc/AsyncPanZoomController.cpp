





#include <math.h>                       
#include <stdint.h>                     
#include <sys/types.h>                  
#include <algorithm>                    
#include "AnimationCommon.h"            
#include "AsyncPanZoomController.h"     
#include "CompositorParent.h"           
#include "FrameMetrics.h"               
#include "GestureEventListener.h"       
#include "InputData.h"                  
#include "Units.h"                      
#include "base/message_loop.h"          
#include "base/task.h"                  
#include "base/tracked.h"               
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
#include "mozilla/layers/GeckoContentController.h"
#include "mozilla/layers/TaskThrottler.h"  
#include "mozilla/mozalloc.h"           
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
#include "nsTraceRefcnt.h"              



#define APZC_LOG(...)

#define APZC_LOG_FM(fm, prefix, ...) \
  APZC_LOG(prefix ":" \
           " i=(%ld %lld) cb=(%d %d %d %d) dp=(%.3f %.3f %.3f %.3f) v=(%.3f %.3f %.3f %.3f) " \
           "s=(%.3f %.3f) sr=(%.3f %.3f %.3f %.3f) z=(%.3f %.3f %.3f %.3f)\n", \
           __VA_ARGS__, \
           fm.mPresShellId, fm.mScrollId, \
           fm.mCompositionBounds.x, fm.mCompositionBounds.y, fm.mCompositionBounds.width, fm.mCompositionBounds.height, \
           fm.mDisplayPort.x, fm.mDisplayPort.y, fm.mDisplayPort.width, fm.mDisplayPort.height, \
           fm.mViewport.x, fm.mViewport.y, fm.mViewport.width, fm.mViewport.height, \
           fm.mScrollOffset.x, fm.mScrollOffset.y, \
           fm.mScrollableRect.x, fm.mScrollableRect.y, fm.mScrollableRect.width, fm.mScrollableRect.height, \
           fm.mDevPixelsPerCSSPixel.scale, fm.mResolution.scale, fm.mCumulativeResolution.scale, fm.mZoom.scale); \


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

using namespace mozilla::css;

namespace mozilla {
namespace layers {







static float gTouchStartTolerance = 1.0f/16.0f;




static const double AXIS_LOCK_ANGLE = M_PI / 6.0; 




static const float AXIS_BREAKOUT_THRESHOLD = 1.0f/32.0f;




static const double AXIS_BREAKOUT_ANGLE = M_PI / 8.0; 




static int32_t gAxisLockMode = 0;





static int32_t gPanRepaintInterval = 250;





static int32_t gFlingRepaintInterval = 75;





static float gMinSkateSpeed = 0.7f;




static const TimeDuration ZOOM_TO_DURATION = TimeDuration::FromSeconds(0.25);




StaticAutoPtr<ComputedTimingFunction> gComputedTimingFunction;




static const CSSToScreenScale MAX_ZOOM(8.0f);




static const CSSToScreenScale MIN_ZOOM(0.125f);







static int gTouchListenerTimeout = 300;





static int gNumPaintDurationSamples = 3;







static float gXSkateSizeMultiplier = 3.0f;
static float gYSkateSizeMultiplier = 3.5f;






static float gXStationarySizeMultiplier = 1.5f;
static float gYStationarySizeMultiplier = 2.5f;






static int gAsyncScrollThrottleTime = 100;





static int gAsyncScrollTimeout = 300;




static bool gAsyncZoomDisabled = false;




static bool gCrossSlideEnabled = false;






static bool IsCloseToHorizontal(float aAngle, float aThreshold)
{
  return (aAngle < aThreshold || aAngle > (M_PI - aThreshold));
}


static bool IsCloseToVertical(float aAngle, float aThreshold)
{
  return (fabs(aAngle - (M_PI / 2)) < aThreshold);
}

static inline void LogRendertraceRect(const char* aDesc, const char* aColor, const CSSRect& aRect)
{
#ifdef APZC_ENABLE_RENDERTRACE
  static const TimeStamp sRenderStart = TimeStamp::Now();
  TimeDuration delta = TimeStamp::Now() - sRenderStart;
  printf_stderr("%s RENDERTRACE %f rect %s %f %f %f %f\n",
    aDesc, delta.ToMilliseconds(), aColor,
    aRect.x, aRect.y, aRect.width, aRect.height);
#endif
}

static TimeStamp sFrameTime;

static TimeStamp
GetFrameTime() {
  if (sFrameTime.IsNull()) {
    return TimeStamp::Now();
  }
  return sFrameTime;
}

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

  Preferences::AddIntVarCache(&gPanRepaintInterval, "apz.pan_repaint_interval", gPanRepaintInterval);
  Preferences::AddIntVarCache(&gFlingRepaintInterval, "apz.fling_repaint_interval", gFlingRepaintInterval);
  Preferences::AddFloatVarCache(&gMinSkateSpeed, "apz.min_skate_speed", gMinSkateSpeed);
  Preferences::AddIntVarCache(&gTouchListenerTimeout, "apz.touch_listener_timeout", gTouchListenerTimeout);
  Preferences::AddIntVarCache(&gNumPaintDurationSamples, "apz.num_paint_duration_samples", gNumPaintDurationSamples);
  Preferences::AddFloatVarCache(&gTouchStartTolerance, "apz.touch_start_tolerance", gTouchStartTolerance);
  Preferences::AddFloatVarCache(&gXSkateSizeMultiplier, "apz.x_skate_size_multiplier", gXSkateSizeMultiplier);
  Preferences::AddFloatVarCache(&gYSkateSizeMultiplier, "apz.y_skate_size_multiplier", gYSkateSizeMultiplier);
  Preferences::AddFloatVarCache(&gXStationarySizeMultiplier, "apz.x_stationary_size_multiplier", gXStationarySizeMultiplier);
  Preferences::AddFloatVarCache(&gYStationarySizeMultiplier, "apz.y_stationary_size_multiplier", gYStationarySizeMultiplier);
  Preferences::AddIntVarCache(&gAsyncScrollThrottleTime, "apz.asyncscroll.throttle", gAsyncScrollThrottleTime);
  Preferences::AddIntVarCache(&gAsyncScrollTimeout, "apz.asyncscroll.timeout", gAsyncScrollTimeout);
  Preferences::AddBoolVarCache(&gAsyncZoomDisabled, "apz.asynczoom.disabled", gAsyncZoomDisabled);
  Preferences::AddBoolVarCache(&gCrossSlideEnabled, "apz.cross_slide.enabled", gCrossSlideEnabled);
  Preferences::AddIntVarCache(&gAxisLockMode, "apz.axis_lock_mode", gAxisLockMode);

  gComputedTimingFunction = new ComputedTimingFunction();
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
     mMonitor("AsyncPanZoomController"),
     mTouchListenerTimeoutTask(nullptr),
     mX(MOZ_THIS_IN_INITIALIZER_LIST()),
     mY(MOZ_THIS_IN_INITIALIZER_LIST()),
     mAllowZoom(true),
     mMinZoom(MIN_ZOOM),
     mMaxZoom(MAX_ZOOM),
     mLastSampleTime(GetFrameTime()),
     mState(NOTHING),
     mLastAsyncScrollTime(GetFrameTime()),
     mLastAsyncScrollOffset(0, 0),
     mCurrentAsyncScrollOffset(0, 0),
     mAsyncScrollTimeoutTask(nullptr),
     mHandlingTouchQueue(false),
     mTreeManager(aTreeManager)
{
  MOZ_COUNT_CTOR(AsyncPanZoomController);

  if (aGestures == USE_GESTURE_DETECTOR) {
    mGestureEventListener = new GestureEventListener(this);
  }
  if (gAsyncZoomDisabled) {
    mAllowZoom = false;
  }
}

AsyncPanZoomController::~AsyncPanZoomController() {
  MOZ_COUNT_DTOR(AsyncPanZoomController);
}

already_AddRefed<GeckoContentController>
AsyncPanZoomController::GetGeckoContentController() {
  MonitorAutoLock lock(mRefPtrMonitor);
  nsRefPtr<GeckoContentController> controller = mGeckoContentController;
  return controller.forget();
}

already_AddRefed<GestureEventListener>
AsyncPanZoomController::GetGestureEventListener() {
  MonitorAutoLock lock(mRefPtrMonitor);
  nsRefPtr<GestureEventListener> listener = mGestureEventListener;
  return listener.forget();
}

void
AsyncPanZoomController::Destroy()
{
  { 
    MonitorAutoLock lock(mRefPtrMonitor);
    mGeckoContentController = nullptr;
    mGestureEventListener = nullptr;
  }
  mPrevSibling = nullptr;
  mLastChild = nullptr;
  mParent = nullptr;
  mTreeManager = nullptr;
}

bool
AsyncPanZoomController::IsDestroyed()
{
  return mTreeManager == nullptr;
}

float
AsyncPanZoomController::GetTouchStartTolerance()
{
  return gTouchStartTolerance;
}

AsyncPanZoomController::AxisLockMode AsyncPanZoomController::GetAxisLockMode()
{
  return static_cast<AxisLockMode>(gAxisLockMode);
}

static CSSPoint
WidgetSpaceToCompensatedViewportSpace(const ScreenPoint& aPoint,
                                      const CSSToScreenScale& aCurrentZoom)
{
  
  
  
  
  
  
  

  return aPoint / aCurrentZoom;
}

nsEventStatus AsyncPanZoomController::ReceiveInputEvent(const InputData& aEvent) {
  
  
  
  
  
  
  if (mFrameMetrics.mMayHaveTouchListeners && aEvent.mInputType == MULTITOUCH_INPUT &&
      (mState == NOTHING || mState == TOUCHING || IsPanningState(mState))) {
    const MultiTouchInput& multiTouchInput = aEvent.AsMultiTouchInput();
    if (multiTouchInput.mType == MultiTouchInput::MULTITOUCH_START) {
      SetState(WAITING_LISTENERS);
    }
  }

  if (mState == WAITING_LISTENERS || mHandlingTouchQueue) {
    if (aEvent.mInputType == MULTITOUCH_INPUT) {
      const MultiTouchInput& multiTouchInput = aEvent.AsMultiTouchInput();
      mTouchQueue.AppendElement(multiTouchInput);

      if (!mTouchListenerTimeoutTask) {
        mTouchListenerTimeoutTask =
          NewRunnableMethod(this, &AsyncPanZoomController::TimeoutTouchListeners);

        PostDelayedTask(mTouchListenerTimeoutTask, gTouchListenerTimeout);
      }
    }
    return nsEventStatus_eIgnore;
  }

  return HandleInputEvent(aEvent);
}

nsEventStatus AsyncPanZoomController::HandleInputEvent(const InputData& aEvent) {
  nsEventStatus rv = nsEventStatus_eIgnore;

  nsRefPtr<GestureEventListener> listener = GetGestureEventListener();
  if (listener) {
    rv = listener->HandleInputEvent(aEvent);
    if (rv == nsEventStatus_eConsumeNoDefault)
      return rv;
  }

  switch (aEvent.mInputType) {
  case MULTITOUCH_INPUT: {
    const MultiTouchInput& multiTouchInput = aEvent.AsMultiTouchInput();
    switch (multiTouchInput.mType) {
      case MultiTouchInput::MULTITOUCH_START: rv = OnTouchStart(multiTouchInput); break;
      case MultiTouchInput::MULTITOUCH_MOVE: rv = OnTouchMove(multiTouchInput); break;
      case MultiTouchInput::MULTITOUCH_END: rv = OnTouchEnd(multiTouchInput); break;
      case MultiTouchInput::MULTITOUCH_CANCEL: rv = OnTouchCancel(multiTouchInput); break;
      default: NS_WARNING("Unhandled multitouch"); break;
    }
    break;
  }
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

  mLastEventTime = aEvent.mTime;
  return rv;
}

nsEventStatus AsyncPanZoomController::OnTouchStart(const MultiTouchInput& aEvent) {
  APZC_LOG("%p got a touch-start in state %d\n", this, mState);
  ScreenIntPoint point = GetFirstTouchScreenPoint(aEvent);

  switch (mState) {
    case ANIMATING_ZOOM:
      
      
      {
        ReentrantMonitorAutoEnter lock(mMonitor);
        RequestContentRepaint();
        ScheduleComposite();
      }
      
    case FLING:
      CancelAnimation();
      
    case NOTHING:
      mX.StartTouch(point.x);
      mY.StartTouch(point.y);
      SetState(TOUCHING);
      break;
    case TOUCHING:
    case PANNING:
    case PANNING_LOCKED_X:
    case PANNING_LOCKED_Y:
    case CROSS_SLIDING_X:
    case CROSS_SLIDING_Y:
    case PINCHING:
    case WAITING_LISTENERS:
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
      float panThreshold = gTouchStartTolerance * APZCTreeManager::GetDPI();
      UpdateWithTouchAtDevicePoint(aEvent);

      if (PanDistance() < panThreshold) {
        return nsEventStatus_eIgnore;
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

    case WAITING_LISTENERS:
      NS_WARNING("Received impossible touch in OnTouchMove");
      break;
  }

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnTouchEnd(const MultiTouchInput& aEvent) {
  APZC_LOG("%p got a touch-end in state %d\n", this, mState);
  {
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
      ReentrantMonitorAutoEnter lock(mMonitor);
      ScheduleComposite();
      RequestContentRepaint();
    }
    mX.EndTouch();
    mY.EndTouch();
    SetState(FLING);
    return nsEventStatus_eConsumeNoDefault;

  case PINCHING:
    SetState(NOTHING);
    
    NS_WARNING("Gesture listener should have handled pinching in OnTouchEnd.");
    return nsEventStatus_eIgnore;

  case WAITING_LISTENERS:
    NS_WARNING("Received impossible touch in OnTouchEnd");
    break;
  }

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnTouchCancel(const MultiTouchInput& aEvent) {
  APZC_LOG("%p got a touch-cancel in state %d\n", this, mState);
  SetState(NOTHING);
  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnScaleBegin(const PinchGestureInput& aEvent) {
  APZC_LOG("%p got a scale-begin in state %d\n", this, mState);
  if (!mAllowZoom) {
    return nsEventStatus_eConsumeNoDefault;
  }

  SetState(PINCHING);
  mLastZoomFocus = aEvent.mFocusPoint - mFrameMetrics.mCompositionBounds.TopLeft();

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnScale(const PinchGestureInput& aEvent) {
  APZC_LOG("%p got a scale in state %d\n", this, mState);
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

    CSSToScreenScale userZoom = mFrameMetrics.mZoom;
    ScreenPoint focusPoint = aEvent.mFocusPoint - mFrameMetrics.mCompositionBounds.TopLeft();
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

    CSSToScreenScale realMinZoom = mMinZoom;
    CSSToScreenScale realMaxZoom = mMaxZoom;
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
      
      
    }

    mLastZoomFocus = focusPoint;
  }

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnScaleEnd(const PinchGestureInput& aEvent) {
  APZC_LOG("%p got a scale-end in state %d\n", this, mState);
  
  
  
  
  if (aEvent.mCurrentSpan >= 0) {
    SetState(PANNING);
    mX.StartTouch(aEvent.mFocusPoint.x);
    mY.StartTouch(aEvent.mFocusPoint.y);
  }
  {
    ReentrantMonitorAutoEnter lock(mMonitor);
    ScheduleComposite();
    RequestContentRepaint();
  }

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnLongPress(const TapGestureInput& aEvent) {
  APZC_LOG("%p got a long-press in state %d\n", this, mState);
  nsRefPtr<GeckoContentController> controller = GetGeckoContentController();
  if (controller) {
    ReentrantMonitorAutoEnter lock(mMonitor);

    CSSPoint point = WidgetSpaceToCompensatedViewportSpace(aEvent.mPoint, mFrameMetrics.mZoom);
    int32_t modifiers = WidgetModifiersToDOMModifiers(aEvent.modifiers);
    controller->HandleLongTap(gfx::RoundedToInt(point), modifiers);
    return nsEventStatus_eConsumeNoDefault;
  }
  return nsEventStatus_eIgnore;
}

nsEventStatus AsyncPanZoomController::OnSingleTapUp(const TapGestureInput& aEvent) {
  APZC_LOG("%p got a single-tap-up in state %d\n", this, mState);
  nsRefPtr<GeckoContentController> controller = GetGeckoContentController();
  if (controller && !mAllowZoom) {
    ReentrantMonitorAutoEnter lock(mMonitor);

    CSSPoint point = WidgetSpaceToCompensatedViewportSpace(aEvent.mPoint, mFrameMetrics.mZoom);
    int32_t modifiers = WidgetModifiersToDOMModifiers(aEvent.modifiers);
    controller->HandleSingleTap(gfx::RoundedToInt(point), modifiers);
    return nsEventStatus_eConsumeNoDefault;
  }
  return nsEventStatus_eIgnore;
}

nsEventStatus AsyncPanZoomController::OnSingleTapConfirmed(const TapGestureInput& aEvent) {
  APZC_LOG("%p got a single-tap-confirmed in state %d\n", this, mState);
  nsRefPtr<GeckoContentController> controller = GetGeckoContentController();
  
  if (controller && mAllowZoom) {
    ReentrantMonitorAutoEnter lock(mMonitor);

    CSSPoint point = WidgetSpaceToCompensatedViewportSpace(aEvent.mPoint, mFrameMetrics.mZoom);
    int32_t modifiers = WidgetModifiersToDOMModifiers(aEvent.modifiers);
    controller->HandleSingleTap(gfx::RoundedToInt(point), modifiers);
    return nsEventStatus_eConsumeNoDefault;
  }
  return nsEventStatus_eIgnore;
}

nsEventStatus AsyncPanZoomController::OnDoubleTap(const TapGestureInput& aEvent) {
  APZC_LOG("%p got a double-tap in state %d\n", this, mState);
  nsRefPtr<GeckoContentController> controller = GetGeckoContentController();
  if (controller) {
    ReentrantMonitorAutoEnter lock(mMonitor);

    if (mAllowZoom) {
      CSSPoint point = WidgetSpaceToCompensatedViewportSpace(aEvent.mPoint, mFrameMetrics.mZoom);
      int32_t modifiers = WidgetModifiersToDOMModifiers(aEvent.modifiers);
      controller->HandleDoubleTap(gfx::RoundedToInt(point), modifiers);
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

const gfx::Point AsyncPanZoomController::GetVelocityVector() {
  return gfx::Point(mX.GetVelocity(), mY.GetVelocity());
}

const gfx::Point AsyncPanZoomController::GetAccelerationVector() {
  return gfx::Point(mX.GetAccelerationFactor(), mY.GetAccelerationFactor());
}

nsEventStatus AsyncPanZoomController::StartPanning(const MultiTouchInput& aEvent) {
  ReentrantMonitorAutoEnter lock(mMonitor);

  ScreenIntPoint point = GetFirstTouchScreenPoint(aEvent);
  float dx = mX.PanDistance(point.x);
  float dy = mY.PanDistance(point.y);

  
  
  mX.StartTouch(point.x);
  mY.StartTouch(point.y);
  mLastEventTime = aEvent.mTime;

  if (GetAxisLockMode() == FREE) {
    SetState(PANNING);
    return nsEventStatus_eConsumeNoDefault;
  }

  double angle = atan2(dy, dx); 
  angle = fabs(angle); 

  if (!gCrossSlideEnabled && (!mX.Scrollable() || !mY.Scrollable())) {
    SetState(PANNING);
  } else if (IsCloseToHorizontal(angle, AXIS_LOCK_ANGLE)) {
    mY.SetScrollingDisabled(true);
    if (mX.Scrollable()) {
      SetState(PANNING_LOCKED_X);
    } else {
      SetState(CROSS_SLIDING_X);
      mX.SetScrollingDisabled(true);
    }
  } else if (IsCloseToVertical(angle, AXIS_LOCK_ANGLE)) {
    mX.SetScrollingDisabled(true);
    if (mY.Scrollable()) {
      SetState(PANNING_LOCKED_Y);
    } else {
      SetState(CROSS_SLIDING_Y);
      mY.SetScrollingDisabled(true);
    }
  } else {
    SetState(PANNING);
  }

  
  return IsPanningState(mState) ? nsEventStatus_eConsumeNoDefault
                                : nsEventStatus_eIgnore;
}

void AsyncPanZoomController::UpdateWithTouchAtDevicePoint(const MultiTouchInput& aEvent) {
  ScreenIntPoint point = GetFirstTouchScreenPoint(aEvent);
  TimeDuration timeDelta = TimeDuration().FromMilliseconds(aEvent.mTime - mLastEventTime);

  
  if (timeDelta.ToMilliseconds() <= EPSILON) {
    return;
  }

  mX.UpdateWithTouchAtDevicePoint(point.x, timeDelta);
  mY.UpdateWithTouchAtDevicePoint(point.y, timeDelta);
}

void AsyncPanZoomController::AttemptScroll(const ScreenPoint& aStartPoint,
                                           const ScreenPoint& aEndPoint,
                                           uint32_t aOverscrollHandoffChainIndex) {
  
  
  
  ScreenPoint displacement = aStartPoint - aEndPoint;

  ScreenPoint overscroll;  
  {
    ReentrantMonitorAutoEnter lock(mMonitor);

    CSSToScreenScale zoom = mFrameMetrics.mZoom;

    
    
    CSSPoint cssDisplacement = displacement / zoom;

    CSSPoint cssOverscroll;
    gfx::Point scrollOffset(mX.AdjustDisplacement(cssDisplacement.x, cssOverscroll.x),
                            mY.AdjustDisplacement(cssDisplacement.y, cssOverscroll.y));
    overscroll = cssOverscroll * zoom;

    if (fabs(scrollOffset.x) > EPSILON || fabs(scrollOffset.y) > EPSILON) {
      ScrollBy(CSSPoint::FromUnknownPoint(scrollOffset));
      ScheduleComposite();

      TimeDuration timePaintDelta = mPaintThrottler.TimeSinceLastRequest(GetFrameTime());
      if (timePaintDelta.ToMilliseconds() > gPanRepaintInterval) {
        RequestContentRepaint();
      }
    }
  }

  if (fabs(overscroll.x) > EPSILON || fabs(overscroll.y) > EPSILON) {
    
    
    CallDispatchScroll(aEndPoint + overscroll, aEndPoint, aOverscrollHandoffChainIndex + 1);
  }
}

void AsyncPanZoomController::CallDispatchScroll(const ScreenPoint& aStartPoint, const ScreenPoint& aEndPoint,
                                                uint32_t aOverscrollHandoffChainIndex) {
  
  
  
  APZCTreeManager* treeManagerLocal = mTreeManager;
  if (treeManagerLocal) {
    treeManagerLocal->DispatchScroll(this, aStartPoint, aEndPoint,
                                     aOverscrollHandoffChainIndex);
  }
}

void AsyncPanZoomController::TrackTouch(const MultiTouchInput& aEvent) {
  ScreenIntPoint prevTouchPoint(mX.GetPos(), mY.GetPos());
  ScreenIntPoint touchPoint = GetFirstTouchScreenPoint(aEvent);
  TimeDuration timeDelta = TimeDuration().FromMilliseconds(aEvent.mTime - mLastEventTime);

  
  if (timeDelta.ToMilliseconds() <= EPSILON) {
    return;
  }

  
  if (GetAxisLockMode() == STICKY) {
    ScreenIntPoint point = GetFirstTouchScreenPoint(aEvent);
    float dx = mX.PanDistance(point.x);
    float dy = mY.PanDistance(point.y);

    double angle = atan2(dy, dx); 
    angle = fabs(angle); 

    float breakThreshold = AXIS_BREAKOUT_THRESHOLD * APZCTreeManager::GetDPI();

    if (fabs(dx) > breakThreshold || fabs(dy) > breakThreshold) {
      if (mState == PANNING_LOCKED_X || mState == CROSS_SLIDING_X) {
        if (!IsCloseToHorizontal(angle, AXIS_BREAKOUT_ANGLE)) {
          mY.SetScrollingDisabled(false);
          SetState(PANNING);
        }
      } else if (mState == PANNING_LOCKED_Y || mState == CROSS_SLIDING_Y) {
        if (!IsCloseToVertical(angle, AXIS_BREAKOUT_ANGLE)) {
          mX.SetScrollingDisabled(false);
          SetState(PANNING);
        }
      }
    }
  }

  UpdateWithTouchAtDevicePoint(aEvent);

  CallDispatchScroll(prevTouchPoint, touchPoint, 0);
}

ScreenIntPoint& AsyncPanZoomController::GetFirstTouchScreenPoint(const MultiTouchInput& aEvent) {
  return ((SingleTouchData&)aEvent.mTouches[0]).mScreenPoint;
}

bool AsyncPanZoomController::DoFling(const TimeDuration& aDelta) {
  if (mState != FLING) {
    return false;
  }

  bool shouldContinueFlingX = mX.FlingApplyFrictionOrCancel(aDelta),
       shouldContinueFlingY = mY.FlingApplyFrictionOrCancel(aDelta);
  
  if (!shouldContinueFlingX && !shouldContinueFlingY) {
    SendAsyncScrollEvent();
    RequestContentRepaint();
    SetState(NOTHING);
    return false;
  }

  CSSPoint overscroll; 
  ScreenPoint offset(aDelta.ToMilliseconds() * mX.GetVelocity(),
                     aDelta.ToMilliseconds() * mY.GetVelocity());

  
  
  CSSPoint cssOffset = offset / mFrameMetrics.mZoom;
  ScrollBy(CSSPoint::FromUnknownPoint(gfx::Point(
    mX.AdjustDisplacement(cssOffset.x, overscroll.x),
    mY.AdjustDisplacement(cssOffset.y, overscroll.y)
  )));
  TimeDuration timePaintDelta = mPaintThrottler.TimeSinceLastRequest(GetFrameTime());
  if (timePaintDelta.ToMilliseconds() > gFlingRepaintInterval) {
    RequestContentRepaint();
  }

  return true;
}

void AsyncPanZoomController::CancelAnimation() {
  SetState(NOTHING);
}

void AsyncPanZoomController::SetCompositorParent(CompositorParent* aCompositorParent) {
  mCompositorParent = aCompositorParent;
}

void AsyncPanZoomController::ScrollBy(const CSSPoint& aOffset) {
  mFrameMetrics.mScrollOffset += aOffset;
}

void AsyncPanZoomController::ScaleWithFocus(float aScale,
                                            const CSSPoint& aFocus) {
  mFrameMetrics.mZoom.scale *= aScale;
  
  
  
  
  mFrameMetrics.mScrollOffset = (mFrameMetrics.mScrollOffset + aFocus) - (aFocus / aScale);
}

bool AsyncPanZoomController::EnlargeDisplayPortAlongAxis(float aSkateSizeMultiplier,
                                                         double aEstimatedPaintDuration,
                                                         float aCompositionBounds,
                                                         float aVelocity,
                                                         float aAcceleration,
                                                         float* aDisplayPortOffset,
                                                         float* aDisplayPortLength)
{
  if (fabsf(aVelocity) > gMinSkateSpeed) {
    
    *aDisplayPortLength = aCompositionBounds * aSkateSizeMultiplier;
    
    
    *aDisplayPortOffset = aVelocity > 0 ? 0 : aCompositionBounds - *aDisplayPortLength;

    
    
    if (aAcceleration > 1.01f) {
      
      
      *aDisplayPortOffset +=
        fabsf(aAcceleration) * aVelocity * aCompositionBounds * aEstimatedPaintDuration;
      
      
      
      
      
      *aDisplayPortOffset -= aVelocity < 0 ? aCompositionBounds : 0;
    }
    return true;
  }
  return false;
}

const CSSRect AsyncPanZoomController::CalculatePendingDisplayPort(
  const FrameMetrics& aFrameMetrics,
  const gfx::Point& aVelocity,
  const gfx::Point& aAcceleration,
  double aEstimatedPaintDuration)
{
  
  
  
  
  
  double estimatedPaintDuration =
    aEstimatedPaintDuration > EPSILON ? aEstimatedPaintDuration : 1.0;

  CSSIntRect compositionBounds = gfx::RoundedIn(aFrameMetrics.mCompositionBounds / aFrameMetrics.mZoom);
  CSSRect scrollableRect = aFrameMetrics.mScrollableRect;

  
  
  
  
  
  
  if (scrollableRect.width < compositionBounds.width) {
      scrollableRect.x = std::max(0.f,
                                  scrollableRect.x - (compositionBounds.width - scrollableRect.width));
      scrollableRect.width = compositionBounds.width;
  }
  if (scrollableRect.height < compositionBounds.height) {
      scrollableRect.y = std::max(0.f,
                                  scrollableRect.y - (compositionBounds.height - scrollableRect.height));
      scrollableRect.height = compositionBounds.height;
  }

  CSSPoint scrollOffset = aFrameMetrics.mScrollOffset;

  CSSRect displayPort = CSSRect(compositionBounds);
  displayPort.MoveTo(0, 0);
  displayPort.Scale(gXStationarySizeMultiplier, gYStationarySizeMultiplier);

  
  
  
  bool enlargedX = EnlargeDisplayPortAlongAxis(
    gXSkateSizeMultiplier, estimatedPaintDuration,
    compositionBounds.width, aVelocity.x, aAcceleration.x,
    &displayPort.x, &displayPort.width);
  bool enlargedY = EnlargeDisplayPortAlongAxis(
    gYSkateSizeMultiplier, estimatedPaintDuration,
    compositionBounds.height, aVelocity.y, aAcceleration.y,
    &displayPort.y, &displayPort.height);

  if (!enlargedX && !enlargedY) {
    
    displayPort.x = -(displayPort.width - compositionBounds.width) / 2;
    displayPort.y = -(displayPort.height - compositionBounds.height) / 2;
  } else if (!enlargedX) {
    displayPort.width = compositionBounds.width;
  } else if (!enlargedY) {
    displayPort.height = compositionBounds.height;
  }

  
  
  
  
  
  
  if (scrollOffset.x + compositionBounds.width > scrollableRect.width) {
    scrollOffset.x -= compositionBounds.width + scrollOffset.x - scrollableRect.width;
  } else if (scrollOffset.x < scrollableRect.x) {
    scrollOffset.x = scrollableRect.x;
  }
  if (scrollOffset.y + compositionBounds.height > scrollableRect.height) {
    scrollOffset.y -= compositionBounds.height + scrollOffset.y - scrollableRect.height;
  } else if (scrollOffset.y < scrollableRect.y) {
    scrollOffset.y = scrollableRect.y;
  }

  CSSRect shiftedDisplayPort = displayPort + scrollOffset;
  return scrollableRect.ClampRect(shiftedDisplayPort) - scrollOffset;
}

void AsyncPanZoomController::ScheduleComposite() {
  if (mCompositorParent) {
    mCompositorParent->ScheduleRenderOnCompositorThread();
  }
}

void AsyncPanZoomController::RequestContentRepaint() {
  mFrameMetrics.mDisplayPort =
    CalculatePendingDisplayPort(mFrameMetrics,
                                GetVelocityVector(),
                                GetAccelerationVector(),
                                mPaintThrottler.AverageDuration().ToSeconds());

  
  
  CSSRect oldDisplayPort = mLastPaintRequestMetrics.mDisplayPort
                         + mLastPaintRequestMetrics.mScrollOffset;
  CSSRect newDisplayPort = mFrameMetrics.mDisplayPort
                         + mFrameMetrics.mScrollOffset;

  if (fabsf(oldDisplayPort.x - newDisplayPort.x) < EPSILON &&
      fabsf(oldDisplayPort.y - newDisplayPort.y) < EPSILON &&
      fabsf(oldDisplayPort.width - newDisplayPort.width) < EPSILON &&
      fabsf(oldDisplayPort.height - newDisplayPort.height) < EPSILON &&
      fabsf(mLastPaintRequestMetrics.mScrollOffset.x -
            mFrameMetrics.mScrollOffset.x) < EPSILON &&
      fabsf(mLastPaintRequestMetrics.mScrollOffset.y -
            mFrameMetrics.mScrollOffset.y) < EPSILON &&
      mFrameMetrics.mZoom == mLastPaintRequestMetrics.mZoom) {
    return;
  }

  SendAsyncScrollEvent();
  ScheduleContentRepaint(mFrameMetrics);
}

void
AsyncPanZoomController::ScheduleContentRepaint(FrameMetrics &aFrameMetrics) {
  
  
  
  nsRefPtr<GeckoContentController> controller = GetGeckoContentController();
  if (controller) {
    APZC_LOG_FM(aFrameMetrics, "%p requesting content repaint", this);

    LogRendertraceRect("requested displayport", "yellow",
        aFrameMetrics.mDisplayPort + aFrameMetrics.mScrollOffset);

    mPaintThrottler.PostTask(
      FROM_HERE,
      NewRunnableMethod(controller.get(),
                        &GeckoContentController::RequestContentRepaint,
                        aFrameMetrics),
      GetFrameTime());
  }
  aFrameMetrics.mPresShellId = mLastContentPaintMetrics.mPresShellId;
  mLastPaintRequestMetrics = aFrameMetrics;
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

bool AsyncPanZoomController::SampleContentTransformForFrame(const TimeStamp& aSampleTime,
                                                            ViewTransform* aNewTransform,
                                                            ScreenPoint& aScrollOffset) {
  
  
  
  
  
  bool requestAnimationFrame = false;

  {
    ReentrantMonitorAutoEnter lock(mMonitor);

    switch (mState) {
    case FLING:
      
      
      requestAnimationFrame |= DoFling(aSampleTime - mLastSampleTime);
      break;
    case ANIMATING_ZOOM: {
      double animPosition = (aSampleTime - mAnimationStartTime) / ZOOM_TO_DURATION;
      if (animPosition > 1.0) {
        animPosition = 1.0;
      }
      
      
      double sampledPosition = gComputedTimingFunction->GetValue(animPosition);

      
      
      mFrameMetrics.mZoom = CSSToScreenScale(1 /
        (sampledPosition / mEndZoomToMetrics.mZoom.scale +
          (1 - sampledPosition) / mStartZoomToMetrics.mZoom.scale));

      mFrameMetrics.mScrollOffset = CSSPoint::FromUnknownPoint(gfx::Point(
        mEndZoomToMetrics.mScrollOffset.x * sampledPosition +
          mStartZoomToMetrics.mScrollOffset.x * (1 - sampledPosition),
        mEndZoomToMetrics.mScrollOffset.y * sampledPosition +
          mStartZoomToMetrics.mScrollOffset.y * (1 - sampledPosition)
      ));

      requestAnimationFrame = true;

      if (aSampleTime - mAnimationStartTime >= ZOOM_TO_DURATION) {
        SetState(NOTHING);
        SendAsyncScrollEvent();
        RequestContentRepaint();
      }

      break;
    }
    default:
      break;
    }

    aScrollOffset = mFrameMetrics.mScrollOffset * mFrameMetrics.mZoom;
    *aNewTransform = GetCurrentAsyncTransform();

    LogRendertraceRect("viewport", "red",
      CSSRect(mFrameMetrics.mScrollOffset,
              ScreenSize(mFrameMetrics.mCompositionBounds.Size()) / mFrameMetrics.mZoom));

    mCurrentAsyncScrollOffset = mFrameMetrics.mScrollOffset;
  }

  
  
  if (mAsyncScrollTimeoutTask) {
    mAsyncScrollTimeoutTask->Cancel();
    mAsyncScrollTimeoutTask = nullptr;
  }
  
  
  
  
  
  TimeDuration delta = aSampleTime - mLastAsyncScrollTime;
  if (delta.ToMilliseconds() > gAsyncScrollThrottleTime &&
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
                                            gAsyncScrollTimeout);
  }

  mLastSampleTime = aSampleTime;

  return requestAnimationFrame;
}

ViewTransform AsyncPanZoomController::GetCurrentAsyncTransform() {
  ReentrantMonitorAutoEnter lock(mMonitor);

  CSSPoint lastPaintScrollOffset;
  if (mLastContentPaintMetrics.IsScrollable()) {
    lastPaintScrollOffset = mLastContentPaintMetrics.mScrollOffset;
  }
  LayerPoint translation = (mFrameMetrics.mScrollOffset - lastPaintScrollOffset)
                         * mLastContentPaintMetrics.LayersPixelsPerCSSPixel();

  return ViewTransform(-translation,
                       mFrameMetrics.mZoom
                     / mLastContentPaintMetrics.mDevPixelsPerCSSPixel
                     / mFrameMetrics.GetParentResolution());
}

gfx3DMatrix AsyncPanZoomController::GetNontransientAsyncTransform() {
  ReentrantMonitorAutoEnter lock(mMonitor);
  return gfx3DMatrix::ScalingMatrix(mLastContentPaintMetrics.mResolution.scale,
                                    mLastContentPaintMetrics.mResolution.scale,
                                    1.0f);
}

void AsyncPanZoomController::NotifyLayersUpdated(const FrameMetrics& aLayerMetrics, bool aIsFirstPaint) {
  ReentrantMonitorAutoEnter lock(mMonitor);

  mLastContentPaintMetrics = aLayerMetrics;

  bool isDefault = mFrameMetrics.IsDefault();
  mFrameMetrics.mMayHaveTouchListeners = aLayerMetrics.mMayHaveTouchListeners;
  APZC_LOG_FM(aLayerMetrics, "%p got a NotifyLayersUpdated with aIsFirstPaint=%d", this, aIsFirstPaint);

  LogRendertraceRect("page", "brown", aLayerMetrics.mScrollableRect);
  LogRendertraceRect("painted displayport", "green",
    aLayerMetrics.mDisplayPort + aLayerMetrics.mScrollOffset);

  mPaintThrottler.TaskComplete(GetFrameTime());
  bool needContentRepaint = false;
  if (aLayerMetrics.mCompositionBounds.width == mFrameMetrics.mCompositionBounds.width &&
      aLayerMetrics.mCompositionBounds.height == mFrameMetrics.mCompositionBounds.height) {
    
    
    if (mFrameMetrics.mViewport.width != aLayerMetrics.mViewport.width)
      needContentRepaint = true;
    mFrameMetrics.mViewport = aLayerMetrics.mViewport;
  }

  if (aIsFirstPaint || isDefault) {
    mPaintThrottler.ClearHistory();
    mPaintThrottler.SetMaxDurations(gNumPaintDurationSamples);

    mX.CancelTouch();
    mY.CancelTouch();

    mFrameMetrics = aLayerMetrics;
    SetState(NOTHING);
  } else {
    
    
    
    mFrameMetrics.mScrollableRect = aLayerMetrics.mScrollableRect;
    mFrameMetrics.mCompositionBounds = aLayerMetrics.mCompositionBounds;
    float parentResolutionChange = aLayerMetrics.GetParentResolution().scale
                                 / mFrameMetrics.GetParentResolution().scale;
    mFrameMetrics.mZoom.scale *= parentResolutionChange;
    mFrameMetrics.mResolution = aLayerMetrics.mResolution;
    mFrameMetrics.mCumulativeResolution = aLayerMetrics.mCumulativeResolution;
  }

  if (needContentRepaint) {
    RequestContentRepaint();
  }
}

const FrameMetrics& AsyncPanZoomController::GetFrameMetrics() {
  mMonitor.AssertCurrentThreadIn();
  return mFrameMetrics;
}

void AsyncPanZoomController::UpdateCompositionBounds(const ScreenIntRect& aCompositionBounds) {
  ReentrantMonitorAutoEnter lock(mMonitor);

  ScreenIntRect oldCompositionBounds = mFrameMetrics.mCompositionBounds;
  mFrameMetrics.mCompositionBounds = aCompositionBounds;

  
  
  
  
  if (aCompositionBounds.width && aCompositionBounds.height &&
      oldCompositionBounds.width && oldCompositionBounds.height) {
    float adjustmentFactor = float(aCompositionBounds.width) / float(oldCompositionBounds.width);
    mFrameMetrics.mZoom.scale =
      clamped(mFrameMetrics.mZoom.scale * adjustmentFactor,
              mMinZoom.scale, mMaxZoom.scale);

    
    RequestContentRepaint();
  }
}

void AsyncPanZoomController::ZoomToRect(CSSRect aRect) {
  SetState(ANIMATING_ZOOM);

  {
    ReentrantMonitorAutoEnter lock(mMonitor);

    ScreenIntRect compositionBounds = mFrameMetrics.mCompositionBounds;
    CSSRect cssPageRect = mFrameMetrics.mScrollableRect;
    CSSPoint scrollOffset = mFrameMetrics.mScrollOffset;
    CSSToScreenScale currentZoom = mFrameMetrics.mZoom;
    CSSToScreenScale targetZoom;

    
    
    
    
    
    CSSToScreenScale localMinZoom(std::max(mMinZoom.scale,
                                  std::max(compositionBounds.width / cssPageRect.width,
                                           compositionBounds.height / cssPageRect.height)));
    CSSToScreenScale localMaxZoom = mMaxZoom;

    if (!aRect.IsEmpty()) {
      
      aRect = aRect.Intersect(cssPageRect);
      targetZoom = CSSToScreenScale(std::min(compositionBounds.width / aRect.width,
                                             compositionBounds.height / aRect.height));
    }
    
    
    
    
    if (aRect.IsEmpty() ||
        (currentZoom == localMaxZoom && targetZoom >= localMaxZoom) ||
        (currentZoom == localMinZoom && targetZoom <= localMinZoom)) {
      CSSRect compositedRect = mFrameMetrics.CalculateCompositedRectInCssPixels();
      float y = scrollOffset.y;
      float newHeight =
        cssPageRect.width * (compositedRect.height / compositedRect.width);
      float dh = compositedRect.height - newHeight;

      aRect = CSSRect(0.0f,
                           y + dh/2,
                           cssPageRect.width,
                           newHeight);
      aRect = aRect.Intersect(cssPageRect);
      targetZoom = CSSToScreenScale(std::min(compositionBounds.width / aRect.width,
                                             compositionBounds.height / aRect.height));
    }

    targetZoom.scale = clamped(targetZoom.scale, localMinZoom.scale, localMaxZoom.scale);
    mEndZoomToMetrics = mFrameMetrics;
    mEndZoomToMetrics.mZoom = targetZoom;

    
    CSSRect rectAfterZoom = mEndZoomToMetrics.CalculateCompositedRectInCssPixels();

    
    
    if (aRect.y + rectAfterZoom.height > cssPageRect.height) {
      aRect.y = cssPageRect.height - rectAfterZoom.height;
      aRect.y = aRect.y > 0 ? aRect.y : 0;
    }
    if (aRect.x + rectAfterZoom.width > cssPageRect.width) {
      aRect.x = cssPageRect.width - rectAfterZoom.width;
      aRect.x = aRect.x > 0 ? aRect.x : 0;
    }

    mStartZoomToMetrics = mFrameMetrics;
    mEndZoomToMetrics.mScrollOffset = aRect.TopLeft();
    mEndZoomToMetrics.mDisplayPort =
      CalculatePendingDisplayPort(mEndZoomToMetrics,
                                  gfx::Point(0,0),
                                  gfx::Point(0,0),
                                  0);

    mAnimationStartTime = GetFrameTime();

    ScheduleComposite();

    
    
    ScheduleContentRepaint(mEndZoomToMetrics);
  }
}

void AsyncPanZoomController::ContentReceivedTouch(bool aPreventDefault) {
  if (!mFrameMetrics.mMayHaveTouchListeners) {
    mTouchQueue.Clear();
    return;
  }

  if (mTouchListenerTimeoutTask) {
    mTouchListenerTimeoutTask->Cancel();
    mTouchListenerTimeoutTask = nullptr;
  }

  if (mState == WAITING_LISTENERS) {
    if (!aPreventDefault) {
      SetState(NOTHING);
    }

    mHandlingTouchQueue = true;

    while (!mTouchQueue.IsEmpty()) {
      if (!aPreventDefault) {
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

void AsyncPanZoomController::SetState(PanZoomState aNewState) {

  PanZoomState oldState;

  
  {
    ReentrantMonitorAutoEnter lock(mMonitor);
    oldState = mState;
    mState = aNewState;
  }

  if (mGeckoContentController) {
    if (!IsTransformingState(oldState) && IsTransformingState(aNewState)) {
      mGeckoContentController->NotifyTransformBegin();
    } else if (IsTransformingState(oldState) && !IsTransformingState(aNewState)) {
      mGeckoContentController->NotifyTransformEnd();
    }
  }
}

bool AsyncPanZoomController::IsTransformingState(PanZoomState aState) {
  return !(aState == NOTHING || aState == TOUCHING || aState == WAITING_LISTENERS);
}

bool AsyncPanZoomController::IsPanningState(PanZoomState aState) {
  return (aState == PANNING || aState == PANNING_LOCKED_X || aState == PANNING_LOCKED_Y);
}

void AsyncPanZoomController::TimeoutTouchListeners() {
  mTouchListenerTimeoutTask = nullptr;
  ContentReceivedTouch(false);
}

void AsyncPanZoomController::UpdateZoomConstraints(bool aAllowZoom,
                                                   const CSSToScreenScale& aMinZoom,
                                                   const CSSToScreenScale& aMaxZoom) {
  if (gAsyncZoomDisabled) {
    return;
  }
  mAllowZoom = aAllowZoom;
  mMinZoom = (MIN_ZOOM > aMinZoom ? MIN_ZOOM : aMinZoom);
  mMaxZoom = (MAX_ZOOM > aMaxZoom ? aMaxZoom : MAX_ZOOM);
}

void
AsyncPanZoomController::GetZoomConstraints(bool* aAllowZoom,
                                           CSSToScreenScale* aMinZoom,
                                           CSSToScreenScale* aMaxZoom)
{
  *aAllowZoom = mAllowZoom;
  *aMinZoom = mMinZoom;
  *aMaxZoom = mMaxZoom;
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

void AsyncPanZoomController::UpdateScrollOffset(const CSSPoint& aScrollOffset)
{
  APZC_LOG("Updating scroll offset from (%f, %f) to (%f, %f)\n",
    mFrameMetrics.mScrollOffset.x, mFrameMetrics.mScrollOffset.y,
    aScrollOffset.x, aScrollOffset.y);

  ReentrantMonitorAutoEnter lock(mMonitor);
  mFrameMetrics.mScrollOffset = aScrollOffset;
}

bool AsyncPanZoomController::Matches(const ScrollableLayerGuid& aGuid)
{
  return aGuid == ScrollableLayerGuid(mLayersId, mFrameMetrics);
}

void AsyncPanZoomController::GetGuid(ScrollableLayerGuid* aGuidOut)
{
  if (!aGuidOut) {
    return;
  }
  aGuidOut->mLayersId = mLayersId;
  aGuidOut->mScrollId = mFrameMetrics.mScrollId;
  aGuidOut->mPresShellId = mFrameMetrics.mPresShellId;
}

}
}
