





#include "mozilla/layers/AsyncCompositionManager.h" 
#include "CompositorParent.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/Constants.h"
#include "mozilla/Util.h"
#include "mozilla/XPCOM.h"
#include "mozilla/Monitor.h"
#include "mozilla/StaticPtr.h"
#include "AsyncPanZoomController.h"
#include "GestureEventListener.h"
#include "nsIThreadManager.h"
#include "nsThreadUtils.h"
#include "Layers.h"
#include "AnimationCommon.h"
#include <algorithm>
#include "mozilla/layers/LayerManagerComposite.h"

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




static const float MAX_ZOOM = 8.0f;




static const float MIN_ZOOM = 0.125f;







static int gTouchListenerTimeout = 300;





static int gNumPaintDurationSamples = 3;







static float gXSkateSizeMultiplier = 3.0f;
static float gYSkateSizeMultiplier = 3.5f;






static float gXStationarySizeMultiplier = 1.5f;
static float gYStationarySizeMultiplier = 2.5f;

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

static void ReadAZPCPrefs()
{
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
}

class ReadAZPCPref MOZ_FINAL : public nsRunnable {
public:
  NS_IMETHOD Run()
  {
    ReadAZPCPrefs();
    return NS_OK;
  }
};

static void InitAZPCPrefs()
{
  static bool sInitialized = false;
  if (sInitialized)
    return;

  sInitialized = true;
  if (NS_IsMainThread()) {
    ReadAZPCPrefs();
  } else {
    
    NS_DispatchToMainThread(new ReadAZPCPref());
  }
}

AsyncPanZoomController::AsyncPanZoomController(GeckoContentController* aGeckoContentController,
                                               GestureBehavior aGestures)
  :  mGeckoContentController(aGeckoContentController),
     mTouchListenerTimeoutTask(nullptr),
     mX(this),
     mY(this),
     mAllowZoom(true),
     mMinZoom(MIN_ZOOM),
     mMaxZoom(MAX_ZOOM),
     mMonitor("AsyncPanZoomController"),
     mLastSampleTime(GetFrameTime()),
     mState(NOTHING),
     mPreviousPaintStartTime(GetFrameTime()),
     mLastAsyncScrollTime(GetFrameTime()),
     mLastAsyncScrollOffset(0, 0),
     mCurrentAsyncScrollOffset(0, 0),
     mAsyncScrollTimeoutTask(nullptr),
     mAsyncScrollThrottleTime(100),
     mAsyncScrollTimeout(300),
     mDPI(72),
     mWaitingForContentToPaint(false),
     mDisableNextTouchBatch(false),
     mHandlingTouchQueue(false),
     mDelayPanning(false)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_COUNT_CTOR(AsyncPanZoomController);

  InitAZPCPrefs();

  if (aGestures == USE_GESTURE_DETECTOR) {
    mGestureEventListener = new GestureEventListener(this);
  }

  SetDPI(mDPI);

  if (!gComputedTimingFunction) {
    gComputedTimingFunction = new ComputedTimingFunction();
    gComputedTimingFunction->Init(
      nsTimingFunction(NS_STYLE_TRANSITION_TIMING_FUNCTION_EASE));
    ClearOnShutdown(&gComputedTimingFunction);
  }

  Preferences::GetUint("apzc.asyncscroll.throttle", &mAsyncScrollThrottleTime);
  Preferences::GetUint("apzc.asyncscroll.timeout", &mAsyncScrollTimeout);
}

AsyncPanZoomController::~AsyncPanZoomController() {
  MOZ_COUNT_DTOR(AsyncPanZoomController);
}

void
AsyncPanZoomController::Destroy()
{
  
  mGeckoContentController = nullptr;
  mGestureEventListener = nullptr;
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

nsEventStatus
AsyncPanZoomController::ReceiveInputEvent(const nsInputEvent& aEvent,
                                          nsInputEvent* aOutEvent)
{
  CSSToScreenScale currentResolution;
  {
    MonitorAutoLock monitor(mMonitor);
    currentResolution = mFrameMetrics.CalculateResolution();
  }

  nsEventStatus status;
  switch (aEvent.eventStructType) {
  case NS_TOUCH_EVENT: {
    MultiTouchInput event(static_cast<const nsTouchEvent&>(aEvent));
    status = ReceiveInputEvent(event);
    break;
  }
  case NS_MOUSE_EVENT: {
    MultiTouchInput event(static_cast<const nsMouseEvent&>(aEvent));
    status = ReceiveInputEvent(event);
    break;
  }
  default:
    status = nsEventStatus_eIgnore;
    break;
  }

  switch (aEvent.eventStructType) {
  case NS_TOUCH_EVENT: {
    nsTouchEvent* touchEvent = static_cast<nsTouchEvent*>(aOutEvent);
    const nsTArray< nsRefPtr<dom::Touch> >& touches = touchEvent->touches;
    for (uint32_t i = 0; i < touches.Length(); ++i) {
      nsIDOMTouch* touch = touches[i];
      if (touch) {
        CSSPoint refPoint = WidgetSpaceToCompensatedViewportSpace(
          ScreenPoint::FromUnknownPoint(gfx::Point(
            touch->mRefPoint.x, touch->mRefPoint.y)),
          currentResolution);
        touch->mRefPoint = nsIntPoint(refPoint.x, refPoint.y);
      }
    }
    break;
  }
  default: {
    CSSPoint refPoint = WidgetSpaceToCompensatedViewportSpace(
      ScreenPoint::FromUnknownPoint(gfx::Point(
        aOutEvent->refPoint.x, aOutEvent->refPoint.y)),
      currentResolution);
    aOutEvent->refPoint = nsIntPoint(refPoint.x, refPoint.y);
    break;
  }
  }

  return status;
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

  if (mGestureEventListener && !mDisableNextTouchBatch) {
    rv = mGestureEventListener->HandleInputEvent(aEvent);
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
        MonitorAutoLock monitor(mMonitor);
        
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
      float panThreshold = gTouchStartTolerance * mDPI;
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
    MonitorAutoLock monitor(mMonitor);
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
      MonitorAutoLock monitor(mMonitor);
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

  float spanRatio = aEvent.mCurrentSpan / aEvent.mPreviousSpan;

  {
    MonitorAutoLock monitor(mMonitor);

    CSSToScreenScale resolution = mFrameMetrics.CalculateResolution();
    gfxFloat userZoom = mFrameMetrics.mZoom.scale;
    ScreenPoint focusPoint = aEvent.mFocusPoint;

    CSSPoint focusChange = (mLastZoomFocus - focusPoint) / resolution;
    
    
    if (mX.DisplacementWillOverscroll(focusChange.x) != Axis::OVERSCROLL_NONE) {
      focusChange.x -= mX.DisplacementWillOverscrollAmount(focusChange.x);
    }
    if (mY.DisplacementWillOverscroll(focusChange.y) != Axis::OVERSCROLL_NONE) {
      focusChange.y -= mY.DisplacementWillOverscrollAmount(focusChange.y);
    }
    ScrollBy(focusChange);

    
    
    
    gfx::Point neededDisplacement;

    float maxZoom = mMaxZoom / mFrameMetrics.CalculateIntrinsicScale().scale;
    float minZoom = mMinZoom / mFrameMetrics.CalculateIntrinsicScale().scale;

    bool doScale = (spanRatio > 1.0 && userZoom < maxZoom) ||
                   (spanRatio < 1.0 && userZoom > minZoom);

    if (doScale) {
      if (userZoom * spanRatio > maxZoom) {
        spanRatio = maxZoom / userZoom;
      } else if (userZoom * spanRatio < minZoom) {
        spanRatio = minZoom / userZoom;
      }

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
    MonitorAutoLock monitor(mMonitor);
    ScheduleComposite();
    RequestContentRepaint();
  }

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnLongPress(const TapGestureInput& aEvent) {
  if (mGeckoContentController) {
    MonitorAutoLock monitor(mMonitor);

    CSSToScreenScale resolution = mFrameMetrics.CalculateResolution();
    CSSPoint point = WidgetSpaceToCompensatedViewportSpace(
      ScreenPoint::FromUnknownPoint(gfx::Point(
        aEvent.mPoint.x, aEvent.mPoint.y)),
      resolution);
    mGeckoContentController->HandleLongTap(gfx::RoundedToInt(point));
    return nsEventStatus_eConsumeNoDefault;
  }
  return nsEventStatus_eIgnore;
}

nsEventStatus AsyncPanZoomController::OnSingleTapUp(const TapGestureInput& aEvent) {
  return nsEventStatus_eIgnore;
}

nsEventStatus AsyncPanZoomController::OnSingleTapConfirmed(const TapGestureInput& aEvent) {
  if (mGeckoContentController) {
    MonitorAutoLock monitor(mMonitor);

    CSSToScreenScale resolution = mFrameMetrics.CalculateResolution();
    CSSPoint point = WidgetSpaceToCompensatedViewportSpace(
      ScreenPoint::FromUnknownPoint(gfx::Point(
        aEvent.mPoint.x, aEvent.mPoint.y)),
      resolution);
    mGeckoContentController->HandleSingleTap(gfx::RoundedToInt(point));
    return nsEventStatus_eConsumeNoDefault;
  }
  return nsEventStatus_eIgnore;
}

nsEventStatus AsyncPanZoomController::OnDoubleTap(const TapGestureInput& aEvent) {
  if (mGeckoContentController) {
    MonitorAutoLock monitor(mMonitor);

    if (mAllowZoom) {
      CSSToScreenScale resolution = mFrameMetrics.CalculateResolution();
      CSSPoint point = WidgetSpaceToCompensatedViewportSpace(
        ScreenPoint::FromUnknownPoint(gfx::Point(
          aEvent.mPoint.x, aEvent.mPoint.y)),
        resolution);
      mGeckoContentController->HandleDoubleTap(gfx::RoundedToInt(point));
    }

    return nsEventStatus_eConsumeNoDefault;
  }
  return nsEventStatus_eIgnore;
}

nsEventStatus AsyncPanZoomController::OnCancelTap(const TapGestureInput& aEvent) {
  
  return nsEventStatus_eIgnore;
}

float AsyncPanZoomController::PanDistance() {
  MonitorAutoLock monitor(mMonitor);
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

void AsyncPanZoomController::TrackTouch(const MultiTouchInput& aEvent) {
  TimeDuration timeDelta = TimeDuration().FromMilliseconds(aEvent.mTime - mLastEventTime);

  
  if (timeDelta.ToMilliseconds() <= EPSILON) {
    return;
  }

  UpdateWithTouchAtDevicePoint(aEvent);

  {
    MonitorAutoLock monitor(mMonitor);

    
    
    ScreenToCSSScale inverseResolution = mFrameMetrics.CalculateResolution().Inverse();

    gfx::Point displacement(mX.GetDisplacementForDuration(inverseResolution.scale,
                                                          timeDelta),
                            mY.GetDisplacementForDuration(inverseResolution.scale,
                                                          timeDelta));
    if (fabs(displacement.x) <= EPSILON && fabs(displacement.y) <= EPSILON) {
      return;
    }

    ScrollBy(CSSPoint::FromUnknownPoint(displacement));
    ScheduleComposite();

    TimeDuration timePaintDelta = GetFrameTime() - mPreviousPaintStartTime;
    if (timePaintDelta.ToMilliseconds() > gPanRepaintInterval) {
      RequestContentRepaint();
    }
  }
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

  
  
  ScreenToCSSScale inverseResolution = mFrameMetrics.CalculateResolution().Inverse();

  ScrollBy(CSSPoint::FromUnknownPoint(gfx::Point(
    mX.GetDisplacementForDuration(inverseResolution.scale, aDelta),
    mY.GetDisplacementForDuration(inverseResolution.scale, aDelta)
  )));
  TimeDuration timePaintDelta = GetFrameTime() - mPreviousPaintStartTime;
  if (timePaintDelta.ToMilliseconds() > gFlingRepaintInterval) {
    RequestContentRepaint();
  }

  return true;
}

void AsyncPanZoomController::CancelAnimation() {
  mState = NOTHING;
}

void AsyncPanZoomController::SetCompositorParent(CompositorParent* aCompositorParent) {
  mCompositorParent = aCompositorParent;
}

void AsyncPanZoomController::ScrollBy(const CSSPoint& aOffset) {
  mFrameMetrics.mScrollOffset += aOffset;
}

void AsyncPanZoomController::ScaleWithFocus(float aZoom,
                                            const ScreenPoint& aFocus) {
  float zoomFactor = aZoom / mFrameMetrics.mZoom.scale;
  CSSToScreenScale resolution = mFrameMetrics.CalculateResolution();

  SetZoomAndResolution(ScreenToScreenScale(aZoom));

  
  
  if (resolution.scale >= 0.01f) {
    mFrameMetrics.mScrollOffset.x +=
      aFocus.x * (zoomFactor - 1.0) / resolution.scale;
    mFrameMetrics.mScrollOffset.y +=
      aFocus.y * (zoomFactor - 1.0) / resolution.scale;
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

  CSSToScreenScale resolution = aFrameMetrics.CalculateResolution();
  CSSIntRect compositionBounds = gfx::RoundedIn(aFrameMetrics.mCompositionBounds / resolution);
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

void AsyncPanZoomController::SetDPI(int aDPI) {
  mDPI = aDPI;
}

int AsyncPanZoomController::GetDPI() {
  return mDPI;
}

void AsyncPanZoomController::ScheduleComposite() {
  if (mCompositorParent) {
    mCompositorParent->ScheduleRenderOnCompositorThread();
  }
}

void AsyncPanZoomController::RequestContentRepaint() {
  mPreviousPaintStartTime = GetFrameTime();

  double estimatedPaintSum = 0.0;
  for (uint32_t i = 0; i < mPreviousPaintDurations.Length(); i++) {
    estimatedPaintSum += mPreviousPaintDurations[i].ToSeconds();
  }

  double estimatedPaintDuration = 0.0;
  if (estimatedPaintSum > EPSILON) {
    estimatedPaintDuration = estimatedPaintSum / mPreviousPaintDurations.Length();
  }

  mFrameMetrics.mDisplayPort =
    CalculatePendingDisplayPort(mFrameMetrics,
                                GetVelocityVector(),
                                GetAccelerationVector(),
                                estimatedPaintDuration);

  
  
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
      mFrameMetrics.mResolution == mLastPaintRequestMetrics.mResolution) {
    return;
  }

  SendAsyncScrollEvent();

  
  
  ScreenToScreenScale actualZoom = mFrameMetrics.mZoom;
  
  float accelerationFactor =
    clamped(std::max(mX.GetAccelerationFactor(), mY.GetAccelerationFactor()),
            MIN_ZOOM / 2.0f, MAX_ZOOM);
  
  mFrameMetrics.mZoom.scale /= accelerationFactor;

  
  
  
  mPaintThrottler.PostTask(
    FROM_HERE,
    NewRunnableMethod(mGeckoContentController.get(),
                      &GeckoContentController::RequestContentRepaint,
                      mFrameMetrics));
  mFrameMetrics.mPresShellId = mLastContentPaintMetrics.mPresShellId;
  mLastPaintRequestMetrics = mFrameMetrics;
  mWaitingForContentToPaint = true;

  
  mFrameMetrics.mZoom = actualZoom;
}

void
AsyncPanZoomController::FireAsyncScrollOnTimeout()
{
  if (mCurrentAsyncScrollOffset != mLastAsyncScrollOffset) {
    MonitorAutoLock monitor(mMonitor);
    SendAsyncScrollEvent();
  }
  mAsyncScrollTimeoutTask = nullptr;
}

bool AsyncPanZoomController::SampleContentTransformForFrame(const TimeStamp& aSampleTime,
                                                            ContainerLayer* aLayer,
                                                            ViewTransform* aNewTransform,
                                                            ScreenPoint& aScrollOffset) {
  
  
  
  
  
  bool requestAnimationFrame = false;

  LayerPoint metricsScrollOffset;
  CSSPoint scrollOffset;
  CSSToScreenScale localScale;
  const FrameMetrics& frame = aLayer->GetFrameMetrics();
  {
    MonitorAutoLock mon(mMonitor);

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

      ScreenToScreenScale startZoom = mStartZoomToMetrics.mZoom;
      ScreenToScreenScale endZoom = mEndZoomToMetrics.mZoom;
      mFrameMetrics.mZoom = ScreenToScreenScale(endZoom.scale * sampledPosition +
                                                startZoom.scale * (1 - sampledPosition));

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

    
    
    
    
    localScale = mFrameMetrics.CalculateResolution();

    if (frame.IsScrollable()) {
      metricsScrollOffset = frame.GetScrollOffsetInLayerPixels();
    }

    scrollOffset = mFrameMetrics.mScrollOffset;
    mCurrentAsyncScrollOffset = mFrameMetrics.mScrollOffset;
  }

  
  
  if (mAsyncScrollTimeoutTask) {
    mAsyncScrollTimeoutTask->Cancel();
    mAsyncScrollTimeoutTask = nullptr;
  }
  
  
  
  
  
  TimeDuration delta = aSampleTime - mLastAsyncScrollTime;
  if (delta.ToMilliseconds() > mAsyncScrollThrottleTime &&
      mCurrentAsyncScrollOffset != mLastAsyncScrollOffset) {
    MonitorAutoLock monitor(mMonitor);
    mLastAsyncScrollTime = aSampleTime;
    mLastAsyncScrollOffset = mCurrentAsyncScrollOffset;
    SendAsyncScrollEvent();
  }
  else {
    mAsyncScrollTimeoutTask =
      NewRunnableMethod(this, &AsyncPanZoomController::FireAsyncScrollOnTimeout);
    MessageLoop::current()->PostDelayedTask(FROM_HERE,
                                            mAsyncScrollTimeoutTask,
                                            mAsyncScrollTimeout);
  }

  CSSToLayerScale paintedScale = frame.mDevPixelsPerCSSPixel * frame.mResolution;
  LayerPoint translation = (scrollOffset * paintedScale) - metricsScrollOffset;
  *aNewTransform = ViewTransform(-translation, localScale / frame.mDevPixelsPerCSSPixel);
  aScrollOffset = scrollOffset * localScale;

  mLastSampleTime = aSampleTime;

  return requestAnimationFrame;
}

void AsyncPanZoomController::NotifyLayersUpdated(const FrameMetrics& aViewportFrame, bool aIsFirstPaint) {
  MonitorAutoLock monitor(mMonitor);

  mLastContentPaintMetrics = aViewportFrame;

  mFrameMetrics.mMayHaveTouchListeners = aViewportFrame.mMayHaveTouchListeners;
  if (mWaitingForContentToPaint) {
    
    
    if (mPreviousPaintDurations.Length() >= gNumPaintDurationSamples) {
      mPreviousPaintDurations.RemoveElementAt(0);
    }

    mPreviousPaintDurations.AppendElement(
      GetFrameTime() - mPreviousPaintStartTime);
  } else {
    
    
    
    
    
    
    switch (mState) {
    case NOTHING:
    case FLING:
    case TOUCHING:
    case WAITING_LISTENERS:
      mFrameMetrics.mScrollOffset = aViewportFrame.mScrollOffset;
      break;
    
    default:
      break;
    }
  }

  mWaitingForContentToPaint = mPaintThrottler.TaskComplete();
  bool needContentRepaint = false;
  if (aViewportFrame.mCompositionBounds.width == mFrameMetrics.mCompositionBounds.width &&
      aViewportFrame.mCompositionBounds.height == mFrameMetrics.mCompositionBounds.height) {
    
    
    CSSToScreenScale previousResolution = mFrameMetrics.CalculateResolution();
    mFrameMetrics.mViewport = aViewportFrame.mViewport;
    CSSToScreenScale newResolution = mFrameMetrics.CalculateResolution();
    needContentRepaint |= (previousResolution != newResolution);
  }

  if (aIsFirstPaint || mFrameMetrics.IsDefault()) {
    mPreviousPaintDurations.Clear();

    mX.CancelTouch();
    mY.CancelTouch();

    mFrameMetrics = aViewportFrame;

    mState = NOTHING;
  } else if (!mFrameMetrics.mScrollableRect.IsEqualEdges(aViewportFrame.mScrollableRect)) {
    mFrameMetrics.mScrollableRect = aViewportFrame.mScrollableRect;
  }

  if (needContentRepaint) {
    RequestContentRepaint();
  }
}

const FrameMetrics& AsyncPanZoomController::GetFrameMetrics() {
  mMonitor.AssertCurrentThreadOwns();
  return mFrameMetrics;
}

void AsyncPanZoomController::UpdateCompositionBounds(const ScreenIntRect& aCompositionBounds) {
  MonitorAutoLock mon(mMonitor);

  ScreenIntRect oldCompositionBounds = mFrameMetrics.mCompositionBounds;
  mFrameMetrics.mCompositionBounds = aCompositionBounds;

  
  
  
  
  if (aCompositionBounds.width && aCompositionBounds.height &&
      oldCompositionBounds.width && oldCompositionBounds.height) {
    SetZoomAndResolution(mFrameMetrics.mZoom);

    
    RequestContentRepaint();
  }
}

void AsyncPanZoomController::CancelDefaultPanZoom() {
  mDisableNextTouchBatch = true;
  if (mGestureEventListener) {
    mGestureEventListener->CancelGesture();
  }
}

void AsyncPanZoomController::DetectScrollableSubframe() {
  mDelayPanning = true;
}

void AsyncPanZoomController::ZoomToRect(const gfxRect& aRect) {
  CSSRect zoomToRect(aRect.x, aRect.y, aRect.width, aRect.height);

  SetState(ANIMATING_ZOOM);

  {
    MonitorAutoLock mon(mMonitor);

    ScreenIntRect compositionBounds = mFrameMetrics.mCompositionBounds;
    CSSRect cssPageRect = mFrameMetrics.mScrollableRect;
    CSSPoint scrollOffset = mFrameMetrics.mScrollOffset;
    float currentZoom = mFrameMetrics.mZoom.scale;
    float targetZoom;
    float intrinsicScale = mFrameMetrics.CalculateIntrinsicScale().scale;

    
    
    
    
    
    float localMinZoom = std::max(mMinZoom,
                         std::max(compositionBounds.width / cssPageRect.width,
                                  compositionBounds.height / cssPageRect.height))
                         / intrinsicScale;
    float localMaxZoom = mMaxZoom / intrinsicScale;

    if (!zoomToRect.IsEmpty()) {
      
      zoomToRect = zoomToRect.Intersect(cssPageRect);
      float targetResolution =
        std::min(compositionBounds.width / zoomToRect.width,
                 compositionBounds.height / zoomToRect.height);
      targetZoom = targetResolution / intrinsicScale;
    }
    
    
    
    
    if (zoomToRect.IsEmpty() ||
        (currentZoom == localMaxZoom && targetZoom >= localMaxZoom) ||
        (currentZoom == localMinZoom && targetZoom <= localMinZoom)) {
      CSSRect compositedRect = mFrameMetrics.CalculateCompositedRectInCssPixels();
      float y = scrollOffset.y;
      float newHeight =
        cssPageRect.width * (compositedRect.height / compositedRect.width);
      float dh = compositedRect.height - newHeight;

      zoomToRect = CSSRect(0.0f,
                           y + dh/2,
                           cssPageRect.width,
                           newHeight);
      zoomToRect = zoomToRect.Intersect(cssPageRect);
      float targetResolution =
        std::min(compositionBounds.width / zoomToRect.width,
                 compositionBounds.height / zoomToRect.height);
      targetZoom = targetResolution / intrinsicScale;
    }

    targetZoom = clamped(targetZoom, localMinZoom, localMaxZoom);
    mEndZoomToMetrics.mZoom = ScreenToScreenScale(targetZoom);

    
    FrameMetrics metricsAfterZoom = mFrameMetrics;
    metricsAfterZoom.mZoom = mEndZoomToMetrics.mZoom;
    CSSRect rectAfterZoom = metricsAfterZoom.CalculateCompositedRectInCssPixels();

    
    
    if (zoomToRect.y + rectAfterZoom.height > cssPageRect.height) {
      zoomToRect.y = cssPageRect.height - rectAfterZoom.height;
      zoomToRect.y = zoomToRect.y > 0 ? zoomToRect.y : 0;
    }
    if (zoomToRect.x + rectAfterZoom.width > cssPageRect.width) {
      zoomToRect.x = cssPageRect.width - rectAfterZoom.width;
      zoomToRect.x = zoomToRect.x > 0 ? zoomToRect.x : 0;
    }

    mStartZoomToMetrics = mFrameMetrics;
    mEndZoomToMetrics.mScrollOffset = zoomToRect.TopLeft();

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

void AsyncPanZoomController::SetState(PanZoomState aState) {
  MonitorAutoLock monitor(mMonitor);
  mState = aState;
}

void AsyncPanZoomController::TimeoutTouchListeners() {
  mTouchListenerTimeoutTask = nullptr;
  ContentReceivedTouch(false);
}

void AsyncPanZoomController::SetZoomAndResolution(const ScreenToScreenScale& aZoom) {
  mMonitor.AssertCurrentThreadOwns();
  mFrameMetrics.mZoom = aZoom;
  CSSToScreenScale resolution = mFrameMetrics.CalculateResolution();
  
  
  
  mFrameMetrics.mResolution = resolution / mFrameMetrics.mDevPixelsPerCSSPixel * ScreenToLayerScale(1);
}

void AsyncPanZoomController::UpdateZoomConstraints(bool aAllowZoom,
                                                   float aMinZoom,
                                                   float aMaxZoom) {
  mAllowZoom = aAllowZoom;
  mMinZoom = std::max(MIN_ZOOM, aMinZoom);
  mMaxZoom = std::min(MAX_ZOOM, aMaxZoom);
}

void AsyncPanZoomController::PostDelayedTask(Task* aTask, int aDelayMs) {
  if (!mGeckoContentController) {
    return;
  }

  mGeckoContentController->PostDelayedTask(aTask, aDelayMs);
}

void AsyncPanZoomController::SendAsyncScrollEvent() {
  if (!mGeckoContentController) {
    return;
  }

  CSSRect contentRect;
  CSSSize scrollableSize;
  {
    scrollableSize = mFrameMetrics.mScrollableRect.Size();
    contentRect = mFrameMetrics.CalculateCompositedRectInCssPixels();
    contentRect.MoveTo(mCurrentAsyncScrollOffset);
  }

  mGeckoContentController->SendAsyncScrollDOMEvent(contentRect, scrollableSize);
}

static void GetAPZCAtPointOnSubtree(const ContainerLayer& aLayerIn,
                    const gfxPoint& aPoint,
                    AsyncPanZoomController** aApzcOut,
                    LayerIntPoint* aRelativePointOut)
{
  
  
  
  ContainerLayer& aLayer = const_cast<ContainerLayer&>(aLayerIn);
  gfx3DMatrix transform = aLayer.GetLocalTransform().Inverse();
  gfxPoint layerPoint = transform.Transform(aPoint);

  
  Layer* currLayer = aLayer.GetLastChild();
  while (currLayer) {
    if (currLayer->AsContainerLayer()) {
      GetAPZCAtPointOnSubtree(*currLayer->AsContainerLayer(), layerPoint, aApzcOut, aRelativePointOut);
    }
    if (*aApzcOut) {
        return;
    }
    currLayer = currLayer->GetPrevSibling();
  }

  bool intersect = aLayer.GetVisibleRegion().Contains(nsIntRect(layerPoint.x, layerPoint.y, 1, 1));

  if (intersect) {
    if (aLayer.GetFrameMetrics().IsScrollable()) {
      *aApzcOut = aLayer.GetAsyncPanZoomController();
      *aRelativePointOut = LayerIntPoint(NS_lround(layerPoint.x), NS_lround(layerPoint.y));
    }
  }

}

void AsyncPanZoomController::GetAPZCAtPoint(const ContainerLayer& aLayerTree,
                    const ScreenIntPoint& aPoint,
                    AsyncPanZoomController** aApzcOut,
                    LayerIntPoint* aRelativePointOut)
{
  *aApzcOut = nullptr;

  gfxPoint point(aPoint.x, aPoint.y);

  GetAPZCAtPointOnSubtree(aLayerTree, point, aApzcOut, aRelativePointOut);
}

}
}
