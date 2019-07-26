





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
#include "mozilla/ClearOnShutdown.h"    
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
#include "nsISupportsImpl.h"
#include "nsMathUtils.h"                
#include "nsPoint.h"                    
#include "nsStyleConsts.h"
#include "nsStyleStruct.h"              
#include "nsTArray.h"                   
#include "nsThreadUtils.h"              
#include "nsTraceRefcnt.h"              

using namespace mozilla::css;

namespace mozilla {
namespace layers {







static float gTouchStartTolerance = 1.0f/16.0f;

static const float EPSILON = 0.0001;





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

  Preferences::AddIntVarCache(&gPanRepaintInterval, "gfx.azpc.pan_repaint_interval", gPanRepaintInterval);
  Preferences::AddIntVarCache(&gFlingRepaintInterval, "gfx.azpc.fling_repaint_interval", gFlingRepaintInterval);
  Preferences::AddFloatVarCache(&gMinSkateSpeed, "gfx.azpc.min_skate_speed", gMinSkateSpeed);
  Preferences::AddIntVarCache(&gTouchListenerTimeout, "gfx.azpc.touch_listener_timeout", gTouchListenerTimeout);
  Preferences::AddIntVarCache(&gNumPaintDurationSamples, "gfx.azpc.num_paint_duration_samples", gNumPaintDurationSamples);
  Preferences::AddFloatVarCache(&gTouchStartTolerance, "gfx.azpc.touch_start_tolerance", gTouchStartTolerance);
  Preferences::AddFloatVarCache(&gXSkateSizeMultiplier, "gfx.azpc.x_skate_size_multiplier", gXSkateSizeMultiplier);
  Preferences::AddFloatVarCache(&gYSkateSizeMultiplier, "gfx.azpc.y_skate_size_multiplier", gYSkateSizeMultiplier);
  Preferences::AddFloatVarCache(&gXStationarySizeMultiplier, "gfx.azpc.x_stationary_size_multiplier", gXStationarySizeMultiplier);
  Preferences::AddFloatVarCache(&gYStationarySizeMultiplier, "gfx.azpc.y_stationary_size_multiplier", gYStationarySizeMultiplier);
  Preferences::AddIntVarCache(&gAsyncScrollThrottleTime, "apzc.asyncscroll.throttle", gAsyncScrollThrottleTime);
  Preferences::AddIntVarCache(&gAsyncScrollTimeout, "apzc.asyncscroll.timeout", gAsyncScrollTimeout);
  Preferences::AddBoolVarCache(&gAsyncZoomDisabled, "apzc.asynczoom.disabled", gAsyncZoomDisabled);

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
     mDisableNextTouchBatch(false),
     mHandlingTouchQueue(false),
     mDelayPanning(false),
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

float
AsyncPanZoomController::GetTouchStartTolerance()
{
  return gTouchStartTolerance;
}

static CSSPoint
WidgetSpaceToCompensatedViewportSpace(const ScreenPoint& aPoint,
                                      const CSSToScreenScale& aCurrentZoom)
{
  
  
  
  
  
  
  

  return aPoint / aCurrentZoom;
}

nsEventStatus AsyncPanZoomController::ReceiveInputEvent(const InputData& aEvent) {
  
  
  
  
  
  
  if (mFrameMetrics.mMayHaveTouchListeners && aEvent.mInputType == MULTITOUCH_INPUT &&
      (mState == NOTHING || mState == TOUCHING || mState == PANNING)) {
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
    return nsEventStatus_eConsumeNoDefault;
  }

  return HandleInputEvent(aEvent);
}

nsEventStatus AsyncPanZoomController::HandleInputEvent(const InputData& aEvent) {
  nsEventStatus rv = nsEventStatus_eIgnore;

  nsRefPtr<GestureEventListener> listener = GetGestureEventListener();
  if (listener && !mDisableNextTouchBatch) {
    rv = listener->HandleInputEvent(aEvent);
    if (rv == nsEventStatus_eConsumeNoDefault)
      return rv;
  }

  if (mDelayPanning && aEvent.mInputType == MULTITOUCH_INPUT) {
    const MultiTouchInput& multiTouchInput = aEvent.AsMultiTouchInput();
    if (multiTouchInput.mType == MultiTouchInput::MULTITOUCH_MOVE) {
      
      SetState(WAITING_LISTENERS);
      mTouchQueue.AppendElement(multiTouchInput);

      if (!mTouchListenerTimeoutTask) {
        mTouchListenerTimeoutTask =
          NewRunnableMethod(this, &AsyncPanZoomController::TimeoutTouchListeners);

        PostDelayedTask(mTouchListenerTimeoutTask, gTouchListenerTimeout);
      }
      return nsEventStatus_eConsumeNoDefault;
    }
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
  SingleTouchData& touch = GetFirstSingleTouch(aEvent);

  ScreenIntPoint point = touch.mScreenPoint;

  switch (mState) {
    case ANIMATING_ZOOM:
      
      
      {
        ReentrantMonitorAutoEnter lock(mMonitor);
        
        SetZoomAndResolution(mFrameMetrics.mZoom);
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
  if (mDisableNextTouchBatch) {
    return nsEventStatus_eIgnore;
  }

  switch (mState) {
    case FLING:
    case NOTHING:
    case ANIMATING_ZOOM:
      
      
      return nsEventStatus_eIgnore;

    case TOUCHING: {
      float panThreshold = gTouchStartTolerance * APZCTreeManager::GetDPI();
      UpdateWithTouchAtDevicePoint(aEvent);

      if (PanDistance() < panThreshold) {
        return nsEventStatus_eIgnore;
      }

      StartPanning(aEvent);

      return nsEventStatus_eConsumeNoDefault;
    }

    case PANNING:
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
  if (mDisableNextTouchBatch) {
    mDisableNextTouchBatch = false;
    return nsEventStatus_eIgnore;
  }

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
    SetState(NOTHING);
    return nsEventStatus_eIgnore;

  case PANNING:
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
  SetState(NOTHING);
  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnScaleBegin(const PinchGestureInput& aEvent) {
  if (!mAllowZoom) {
    return nsEventStatus_eConsumeNoDefault;
  }

  SetState(PINCHING);
  mLastZoomFocus = aEvent.mFocusPoint;

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnScale(const PinchGestureInput& aEvent) {
  if (mState != PINCHING) {
    return nsEventStatus_eConsumeNoDefault;
  }

  float prevSpan = aEvent.mPreviousSpan;
  if (fabsf(prevSpan) <= EPSILON || fabsf(aEvent.mCurrentSpan) <= EPSILON) {
    
    return nsEventStatus_eConsumeNoDefault;
  }

  ScreenToScreenScale spanRatio(aEvent.mCurrentSpan / aEvent.mPreviousSpan);

  {
    ReentrantMonitorAutoEnter lock(mMonitor);

    CSSToScreenScale userZoom = mFrameMetrics.mZoom;
    ScreenPoint focusPoint = aEvent.mFocusPoint;

    CSSPoint focusChange = (mLastZoomFocus - focusPoint) / userZoom;
    
    
    if (mX.DisplacementWillOverscroll(focusChange.x) != Axis::OVERSCROLL_NONE) {
      focusChange.x -= mX.DisplacementWillOverscrollAmount(focusChange.x);
    }
    if (mY.DisplacementWillOverscroll(focusChange.y) != Axis::OVERSCROLL_NONE) {
      focusChange.y -= mY.DisplacementWillOverscrollAmount(focusChange.y);
    }
    ScrollBy(focusChange);

    
    
    
    gfx::Point neededDisplacement;

    bool doScale = (spanRatio > ScreenToScreenScale(1.0) && userZoom < mMaxZoom) ||
                   (spanRatio < ScreenToScreenScale(1.0) && userZoom > mMinZoom);

    if (doScale) {
      spanRatio.scale = clamped(spanRatio.scale,
                                mMinZoom.scale / userZoom.scale,
                                mMaxZoom.scale / userZoom.scale);

      switch (mX.ScaleWillOverscroll(spanRatio, focusPoint.x))
      {
        case Axis::OVERSCROLL_NONE:
          break;
        case Axis::OVERSCROLL_MINUS:
        case Axis::OVERSCROLL_PLUS:
          neededDisplacement.x = -mX.ScaleWillOverscrollAmount(spanRatio, focusPoint.x);
          break;
        case Axis::OVERSCROLL_BOTH:
          
          
          
          
          doScale = false;
          break;
      }
    }

    if (doScale) {
      switch (mY.ScaleWillOverscroll(spanRatio, focusPoint.y))
      {
        case Axis::OVERSCROLL_NONE:
          break;
        case Axis::OVERSCROLL_MINUS:
        case Axis::OVERSCROLL_PLUS:
          neededDisplacement.y = -mY.ScaleWillOverscrollAmount(spanRatio, focusPoint.y);
          break;
        case Axis::OVERSCROLL_BOTH:
          doScale = false;
          break;
      }
    }

    if (doScale) {
      ScaleWithFocus(userZoom * spanRatio, focusPoint);

      if (neededDisplacement != gfx::Point()) {
        ScrollBy(CSSPoint::FromUnknownPoint(neededDisplacement));
      }

      ScheduleComposite();
      
      
    }

    mLastZoomFocus = focusPoint;
  }

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnScaleEnd(const PinchGestureInput& aEvent) {
  SetState(PANNING);
  mX.StartTouch(aEvent.mFocusPoint.x);
  mY.StartTouch(aEvent.mFocusPoint.y);
  {
    ReentrantMonitorAutoEnter lock(mMonitor);
    ScheduleComposite();
    RequestContentRepaint();
  }

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnLongPress(const TapGestureInput& aEvent) {
  nsRefPtr<GeckoContentController> controller = GetGeckoContentController();
  if (controller) {
    ReentrantMonitorAutoEnter lock(mMonitor);

    CSSPoint point = WidgetSpaceToCompensatedViewportSpace(aEvent.mPoint, mFrameMetrics.mZoom);
    controller->HandleLongTap(gfx::RoundedToInt(point));
    return nsEventStatus_eConsumeNoDefault;
  }
  return nsEventStatus_eIgnore;
}

nsEventStatus AsyncPanZoomController::OnSingleTapUp(const TapGestureInput& aEvent) {
  return nsEventStatus_eIgnore;
}

nsEventStatus AsyncPanZoomController::OnSingleTapConfirmed(const TapGestureInput& aEvent) {
  nsRefPtr<GeckoContentController> controller = GetGeckoContentController();
  if (controller) {
    ReentrantMonitorAutoEnter lock(mMonitor);

    CSSPoint point = WidgetSpaceToCompensatedViewportSpace(aEvent.mPoint, mFrameMetrics.mZoom);
    controller->HandleSingleTap(gfx::RoundedToInt(point));
    return nsEventStatus_eConsumeNoDefault;
  }
  return nsEventStatus_eIgnore;
}

nsEventStatus AsyncPanZoomController::OnDoubleTap(const TapGestureInput& aEvent) {
  nsRefPtr<GeckoContentController> controller = GetGeckoContentController();
  if (controller) {
    ReentrantMonitorAutoEnter lock(mMonitor);

    if (mAllowZoom) {
      CSSPoint point = WidgetSpaceToCompensatedViewportSpace(aEvent.mPoint, mFrameMetrics.mZoom);
      controller->HandleDoubleTap(gfx::RoundedToInt(point));
    }

    return nsEventStatus_eConsumeNoDefault;
  }
  return nsEventStatus_eIgnore;
}

nsEventStatus AsyncPanZoomController::OnCancelTap(const TapGestureInput& aEvent) {
  
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

void AsyncPanZoomController::StartPanning(const MultiTouchInput& aEvent) {
  float dx = mX.PanDistance(),
        dy = mY.PanDistance();

  double angle = atan2(dy, dx); 
  angle = fabs(angle); 

  SetState(PANNING);
}

void AsyncPanZoomController::UpdateWithTouchAtDevicePoint(const MultiTouchInput& aEvent) {
  SingleTouchData& touch = GetFirstSingleTouch(aEvent);
  ScreenIntPoint point = touch.mScreenPoint;
  TimeDuration timeDelta = TimeDuration().FromMilliseconds(aEvent.mTime - mLastEventTime);

  
  if (timeDelta.ToMilliseconds() <= EPSILON) {
    return;
  }

  mX.UpdateWithTouchAtDevicePoint(point.x, timeDelta);
  mY.UpdateWithTouchAtDevicePoint(point.y, timeDelta);
}

void AsyncPanZoomController::AttemptScroll(const ScreenPoint& aStartPoint,
                                           const ScreenPoint& aEndPoint) {
  
  
  
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
    
    mTreeManager->HandleOverscroll(this, aEndPoint + overscroll, aEndPoint);
  }
}

void AsyncPanZoomController::TrackTouch(const MultiTouchInput& aEvent) {
  SingleTouchData& touch = GetFirstSingleTouch(aEvent);
  ScreenIntPoint prevTouchPoint(mX.GetPos(), mY.GetPos());
  ScreenIntPoint touchPoint = touch.mScreenPoint;
  TimeDuration timeDelta = TimeDuration().FromMilliseconds(aEvent.mTime - mLastEventTime);

  
  if (timeDelta.ToMilliseconds() <= EPSILON) {
    return;
  }

  UpdateWithTouchAtDevicePoint(aEvent);

  AttemptScroll(prevTouchPoint, touchPoint);
}

SingleTouchData& AsyncPanZoomController::GetFirstSingleTouch(const MultiTouchInput& aEvent) {
  return (SingleTouchData&)aEvent.mTouches[0];
}

bool AsyncPanZoomController::DoFling(const TimeDuration& aDelta) {
  if (mState != FLING) {
    return false;
  }

  bool shouldContinueFlingX = mX.FlingApplyFrictionOrCancel(aDelta),
       shouldContinueFlingY = mY.FlingApplyFrictionOrCancel(aDelta);
  
  if (!shouldContinueFlingX && !shouldContinueFlingY) {
    
    
    SetZoomAndResolution(mFrameMetrics.mZoom);
    SendAsyncScrollEvent();
    RequestContentRepaint();
    mState = NOTHING;
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
  ReentrantMonitorAutoEnter lock(mMonitor);
  mState = NOTHING;
}

void AsyncPanZoomController::SetCompositorParent(CompositorParent* aCompositorParent) {
  mCompositorParent = aCompositorParent;
}

void AsyncPanZoomController::ScrollBy(const CSSPoint& aOffset) {
  mFrameMetrics.mScrollOffset += aOffset;
}

void AsyncPanZoomController::ScaleWithFocus(const CSSToScreenScale& aZoom,
                                            const ScreenPoint& aFocus) {
  ScreenToScreenScale zoomFactor(aZoom.scale / mFrameMetrics.mZoom.scale);
  CSSToScreenScale resolution = mFrameMetrics.mZoom;

  SetZoomAndResolution(aZoom);

  
  
  if (resolution.scale >= 0.01f) {
    zoomFactor.scale -= 1.0;
    mFrameMetrics.mScrollOffset += aFocus * zoomFactor / resolution;
  }
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
      mFrameMetrics.mCumulativeResolution == mLastPaintRequestMetrics.mCumulativeResolution) {
    return;
  }

  SendAsyncScrollEvent();

  
  
  CSSToScreenScale actualZoom = mFrameMetrics.mZoom;
  
  float accelerationFactor =
    clamped(std::max(mX.GetAccelerationFactor(), mY.GetAccelerationFactor()),
            MIN_ZOOM.scale / 2.0f, MAX_ZOOM.scale);
  
  mFrameMetrics.mZoom.scale /= accelerationFactor;

  
  
  
  nsRefPtr<GeckoContentController> controller = GetGeckoContentController();
  if (controller) {
    mPaintThrottler.PostTask(
      FROM_HERE,
      NewRunnableMethod(controller.get(),
                        &GeckoContentController::RequestContentRepaint,
                        mFrameMetrics),
      GetFrameTime());
  }
  mFrameMetrics.mPresShellId = mLastContentPaintMetrics.mPresShellId;
  mLastPaintRequestMetrics = mFrameMetrics;

  
  mFrameMetrics.mZoom = actualZoom;
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

      mFrameMetrics.mZoom = CSSToScreenScale(
        mEndZoomToMetrics.mZoom.scale * sampledPosition +
          mStartZoomToMetrics.mZoom.scale * (1 - sampledPosition));

      mFrameMetrics.mScrollOffset = CSSPoint::FromUnknownPoint(gfx::Point(
        mEndZoomToMetrics.mScrollOffset.x * sampledPosition +
          mStartZoomToMetrics.mScrollOffset.x * (1 - sampledPosition),
        mEndZoomToMetrics.mScrollOffset.y * sampledPosition +
          mStartZoomToMetrics.mScrollOffset.y * (1 - sampledPosition)
      ));

      requestAnimationFrame = true;

      if (aSampleTime - mAnimationStartTime >= ZOOM_TO_DURATION) {
        
        SetZoomAndResolution(mFrameMetrics.mZoom);
        mState = NOTHING;
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

void AsyncPanZoomController::NotifyLayersUpdated(const FrameMetrics& aLayerMetrics, bool aIsFirstPaint) {
  ReentrantMonitorAutoEnter lock(mMonitor);

  mLastContentPaintMetrics = aLayerMetrics;

  bool isDefault = mFrameMetrics.IsDefault();
  mFrameMetrics.mMayHaveTouchListeners = aLayerMetrics.mMayHaveTouchListeners;

  mPaintThrottler.TaskComplete(GetFrameTime());
  bool needContentRepaint = false;
  if (aLayerMetrics.mCompositionBounds.width == mFrameMetrics.mCompositionBounds.width &&
      aLayerMetrics.mCompositionBounds.height == mFrameMetrics.mCompositionBounds.height) {
    
    
    CSSToScreenScale previousResolution = mFrameMetrics.CalculateIntrinsicScale();
    mFrameMetrics.mViewport = aLayerMetrics.mViewport;
    CSSToScreenScale newResolution = mFrameMetrics.CalculateIntrinsicScale();
    if (previousResolution != newResolution) {
      needContentRepaint = true;
      mFrameMetrics.mZoom.scale *= newResolution.scale / previousResolution.scale;
    }
  }

  if (aIsFirstPaint || isDefault) {
    mPaintThrottler.ClearHistory();
    mPaintThrottler.SetMaxDurations(gNumPaintDurationSamples);

    mX.CancelTouch();
    mY.CancelTouch();

    mFrameMetrics = aLayerMetrics;
    mState = NOTHING;
  } else if (!mFrameMetrics.mScrollableRect.IsEqualEdges(aLayerMetrics.mScrollableRect)) {
    mFrameMetrics.mScrollableRect = aLayerMetrics.mScrollableRect;
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
    ScreenToScreenScale adjustmentFactor(float(aCompositionBounds.width) / float(oldCompositionBounds.width));
    SetZoomAndResolution(mFrameMetrics.mZoom * adjustmentFactor);

    
    RequestContentRepaint();
  }
}

void AsyncPanZoomController::CancelDefaultPanZoom() {
  mDisableNextTouchBatch = true;
  nsRefPtr<GestureEventListener> listener = GetGestureEventListener();
  if (listener) {
    listener->CancelGesture();
  }
}

void AsyncPanZoomController::DetectScrollableSubframe() {
  mDelayPanning = true;
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
    mEndZoomToMetrics.mZoom = targetZoom;

    
    FrameMetrics metricsAfterZoom = mFrameMetrics;
    metricsAfterZoom.mZoom = mEndZoomToMetrics.mZoom;
    CSSRect rectAfterZoom = metricsAfterZoom.CalculateCompositedRectInCssPixels();

    
    
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

    mAnimationStartTime = GetFrameTime();

    ScheduleComposite();
  }
}

void AsyncPanZoomController::ContentReceivedTouch(bool aPreventDefault) {
  if (!mFrameMetrics.mMayHaveTouchListeners && !mDelayPanning) {
    mTouchQueue.Clear();
    return;
  }

  if (mTouchListenerTimeoutTask) {
    mTouchListenerTimeoutTask->Cancel();
    mTouchListenerTimeoutTask = nullptr;
  }

  if (mState == WAITING_LISTENERS) {
    if (!aPreventDefault) {
      
      if (mDelayPanning) {
        SetState(TOUCHING);
      } else {
        SetState(NOTHING);
      }
    }

    mHandlingTouchQueue = true;

    while (!mTouchQueue.IsEmpty()) {
      
      if (!aPreventDefault && mTouchQueue[0].mType == MultiTouchInput::MULTITOUCH_MOVE) {
        mDelayPanning = false;
      }
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
    if (oldState == PANNING && aNewState != PANNING) {
      mGeckoContentController->HandlePanEnd();
    } else if (oldState != PANNING && aNewState == PANNING) {
      mGeckoContentController->HandlePanBegin();
    }
  }
}

void AsyncPanZoomController::TimeoutTouchListeners() {
  mTouchListenerTimeoutTask = nullptr;
  ContentReceivedTouch(false);
}

void AsyncPanZoomController::SetZoomAndResolution(const CSSToScreenScale& aZoom) {
  mMonitor.AssertCurrentThreadIn();
  LayoutDeviceToParentLayerScale parentResolution = mFrameMetrics.GetParentResolution();
  mFrameMetrics.mZoom = aZoom;
  
  
  
  mFrameMetrics.mCumulativeResolution = aZoom / mFrameMetrics.mDevPixelsPerCSSPixel * ScreenToLayerScale(1);
  
  mFrameMetrics.mResolution = mFrameMetrics.mCumulativeResolution / parentResolution;
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

  FrameMetrics::ViewID scrollId;
  CSSRect contentRect;
  CSSSize scrollableSize;
  {
    ReentrantMonitorAutoEnter lock(mMonitor);

    scrollId = mFrameMetrics.mScrollId;
    scrollableSize = mFrameMetrics.mScrollableRect.Size();
    contentRect = mFrameMetrics.CalculateCompositedRectInCssPixels();
    contentRect.MoveTo(mCurrentAsyncScrollOffset);
  }

  controller->SendAsyncScrollDOMEvent(scrollId, contentRect, scrollableSize);
}

void AsyncPanZoomController::UpdateScrollOffset(const CSSPoint& aScrollOffset)
{
  ReentrantMonitorAutoEnter lock(mMonitor);
  mFrameMetrics.mScrollOffset = aScrollOffset;
}

bool AsyncPanZoomController::Matches(const ScrollableLayerGuid& aGuid)
{
  
  
  return aGuid.mLayersId == mLayersId && aGuid.mScrollId == mFrameMetrics.mScrollId;
}

}
}
