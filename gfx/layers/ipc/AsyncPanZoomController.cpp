





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

using namespace mozilla::css;

namespace mozilla {
namespace layers {

const float AsyncPanZoomController::TOUCH_START_TOLERANCE = 1.0f/16.0f;

static const float EPSILON = 0.0001;





static const int32_t PAN_REPAINT_INTERVAL = 250;





static const int32_t FLING_REPAINT_INTERVAL = 75;





static const float MIN_SKATE_SPEED = 0.5f;




static const float AXIS_LOCK_ANGLE = M_PI / 9.0;




static const TimeDuration ZOOM_TO_DURATION = TimeDuration::FromSeconds(0.25);




StaticAutoPtr<ComputedTimingFunction> gComputedTimingFunction;




static const double MAX_ZOOM = 8.0;




static const double MIN_ZOOM = 0.125;







static const int TOUCH_LISTENER_TIMEOUT = 300;

AsyncPanZoomController::AsyncPanZoomController(GeckoContentController* aGeckoContentController,
                                               GestureBehavior aGestures)
  :  mGeckoContentController(aGeckoContentController),
     mTouchListenerTimeoutTask(nullptr),
     mX(this),
     mY(this),
     mMonitor("AsyncPanZoomController"),
     mLastSampleTime(TimeStamp::Now()),
     mState(NOTHING),
     mDPI(72),
     mContentPainterStatus(CONTENT_IDLE),
     mDisableNextTouchBatch(false),
     mHandlingTouchQueue(false)
{
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
}

AsyncPanZoomController::~AsyncPanZoomController() {

}

static gfx::Point
WidgetSpaceToCompensatedViewportSpace(const gfx::Point& aPoint,
                                      gfxFloat aCurrentZoom)
{
  
  
  gfx::Point pt(aPoint);
  pt = pt / aCurrentZoom;

  
  
  
  
  

  return pt;
}

nsEventStatus
AsyncPanZoomController::ReceiveInputEvent(const nsInputEvent& aEvent,
                                          nsInputEvent* aOutEvent)
{
  float currentZoom;
  gfx::Point currentScrollOffset, lastScrollOffset;
  {
    MonitorAutoLock monitor(mMonitor);
    currentZoom = mFrameMetrics.mResolution.width;
    currentScrollOffset = gfx::Point(mFrameMetrics.mViewportScrollOffset.x,
                                     mFrameMetrics.mViewportScrollOffset.y);
    lastScrollOffset = gfx::Point(mLastContentPaintMetrics.mViewportScrollOffset.x,
                                  mLastContentPaintMetrics.mViewportScrollOffset.y);
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
    const nsTArray<nsCOMPtr<nsIDOMTouch> >& touches = touchEvent->touches;
    for (uint32_t i = 0; i < touches.Length(); ++i) {
      nsIDOMTouch* touch = touches[i];
      if (touch) {
        gfx::Point refPoint = WidgetSpaceToCompensatedViewportSpace(
          gfx::Point(touch->mRefPoint.x, touch->mRefPoint.y),
          currentZoom);
        touch->mRefPoint = nsIntPoint(refPoint.x, refPoint.y);
      }
    }
    break;
  }
  default: {
    gfx::Point refPoint = WidgetSpaceToCompensatedViewportSpace(
      gfx::Point(aOutEvent->refPoint.x, aOutEvent->refPoint.y),
      currentZoom);
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

        MessageLoop::current()->PostDelayedTask(
          FROM_HERE,
          mTouchListenerTimeoutTask,
          TOUCH_LISTENER_TIMEOUT);
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

  nsIntPoint point = touch.mScreenPoint;
  int32_t xPos = point.x, yPos = point.y;

  switch (mState) {
    case ANIMATING_ZOOM:
      
      
      RequestContentRepaint();
      ScheduleComposite();
      
    case FLING:
      CancelAnimation();
      
    case NOTHING:
      mX.StartTouch(xPos);
      mY.StartTouch(yPos);
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
      float panThreshold = TOUCH_START_TOLERANCE * mDPI;
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
  SetState(PINCHING);
  mLastZoomFocus = aEvent.mFocusPoint;

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnScale(const PinchGestureInput& aEvent) {
  float prevSpan = aEvent.mPreviousSpan;
  if (fabsf(prevSpan) <= EPSILON || fabsf(aEvent.mCurrentSpan) <= EPSILON) {
    
    return nsEventStatus_eConsumeNoDefault;
  }

  float spanRatio = aEvent.mCurrentSpan / aEvent.mPreviousSpan;

  {
    MonitorAutoLock monitor(mMonitor);

    float scale = mFrameMetrics.mResolution.width;

    nsIntPoint focusPoint = aEvent.mFocusPoint;
    float xFocusChange = (mLastZoomFocus.x - focusPoint.x) / scale, yFocusChange = (mLastZoomFocus.y - focusPoint.y) / scale;
    
    
    if (mX.DisplacementWillOverscroll(xFocusChange) != Axis::OVERSCROLL_NONE) {
      xFocusChange -= mX.DisplacementWillOverscrollAmount(xFocusChange);
    }
    if (mY.DisplacementWillOverscroll(yFocusChange) != Axis::OVERSCROLL_NONE) {
      yFocusChange -= mY.DisplacementWillOverscrollAmount(yFocusChange);
    }
    ScrollBy(gfx::Point(xFocusChange, yFocusChange));

    
    
    
    float neededDisplacementX = 0, neededDisplacementY = 0;

    
    bool doScale = (scale < MAX_ZOOM && spanRatio > 1.0f) || (scale > MIN_ZOOM && spanRatio < 1.0f);

    
    
    if (scale * spanRatio > MAX_ZOOM) {
      spanRatio = scale / MAX_ZOOM;
    } else if (scale * spanRatio < MIN_ZOOM) {
      spanRatio = scale / MIN_ZOOM;
    }

    if (doScale) {
      switch (mX.ScaleWillOverscroll(spanRatio, focusPoint.x))
      {
        case Axis::OVERSCROLL_NONE:
          break;
        case Axis::OVERSCROLL_MINUS:
        case Axis::OVERSCROLL_PLUS:
          neededDisplacementX = -mX.ScaleWillOverscrollAmount(spanRatio, focusPoint.x);
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
          neededDisplacementY = -mY.ScaleWillOverscrollAmount(spanRatio, focusPoint.y);
          break;
        case Axis::OVERSCROLL_BOTH:
          doScale = false;
          break;
      }
    }

    if (doScale) {
      ScaleWithFocus(scale * spanRatio,
                     focusPoint);

      if (neededDisplacementX != 0 || neededDisplacementY != 0) {
        ScrollBy(gfx::Point(neededDisplacementX, neededDisplacementY));
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
  
  return nsEventStatus_eIgnore;
}

nsEventStatus AsyncPanZoomController::OnSingleTapUp(const TapGestureInput& aEvent) {
  
  return nsEventStatus_eIgnore;
}

nsEventStatus AsyncPanZoomController::OnSingleTapConfirmed(const TapGestureInput& aEvent) {
  
  return nsEventStatus_eIgnore;
}

nsEventStatus AsyncPanZoomController::OnDoubleTap(const TapGestureInput& aEvent) {
  if (mGeckoContentController) {
    MonitorAutoLock monitor(mMonitor);

    gfx::Point point = WidgetSpaceToCompensatedViewportSpace(
      gfx::Point(aEvent.mPoint.x, aEvent.mPoint.y),
      mFrameMetrics.mResolution.width);
    mGeckoContentController->HandleDoubleTap(nsIntPoint(NS_lround(point.x), NS_lround(point.y)));
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

void AsyncPanZoomController::StartPanning(const MultiTouchInput& aEvent) {
  float dx = mX.PanDistance(),
        dy = mY.PanDistance();

  double angle = atan2(dy, dx); 
  angle = fabs(angle); 

  SetState(PANNING);

  if (angle < AXIS_LOCK_ANGLE || angle > (M_PI - AXIS_LOCK_ANGLE)) {
    mY.LockPanning();
  } else if (fabsf(angle - M_PI / 2) < AXIS_LOCK_ANGLE) {
    mX.LockPanning();
  }
}

void AsyncPanZoomController::UpdateWithTouchAtDevicePoint(const MultiTouchInput& aEvent) {
  SingleTouchData& touch = GetFirstSingleTouch(aEvent);
  nsIntPoint point = touch.mScreenPoint;
  int32_t xPos = point.x, yPos = point.y;
  TimeDuration timeDelta = TimeDuration().FromMilliseconds(aEvent.mTime - mLastEventTime);

  
  if (timeDelta.ToMilliseconds() <= EPSILON) {
    return;
  }

  mX.UpdateWithTouchAtDevicePoint(xPos, timeDelta);
  mY.UpdateWithTouchAtDevicePoint(yPos, timeDelta);
}

void AsyncPanZoomController::TrackTouch(const MultiTouchInput& aEvent) {
  TimeDuration timeDelta = TimeDuration().FromMilliseconds(aEvent.mTime - mLastEventTime);

  
  if (timeDelta.ToMilliseconds() <= EPSILON) {
    return;
  }

  UpdateWithTouchAtDevicePoint(aEvent);

  {
    MonitorAutoLock monitor(mMonitor);

    
    
    float inverseScale = 1 / mFrameMetrics.mResolution.width;

    int32_t xDisplacement = mX.GetDisplacementForDuration(inverseScale, timeDelta);
    int32_t yDisplacement = mY.GetDisplacementForDuration(inverseScale, timeDelta);
    if (!xDisplacement && !yDisplacement) {
      return;
    }

    ScrollBy(gfx::Point(xDisplacement, yDisplacement));
    ScheduleComposite();

    RequestContentRepaint();
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
    RequestContentRepaint();
    mState = NOTHING;
    return false;
  }

  
  
  float inverseScale = 1 / mFrameMetrics.mResolution.width;

  ScrollBy(gfx::Point(
    mX.GetDisplacementForDuration(inverseScale, aDelta),
    mY.GetDisplacementForDuration(inverseScale, aDelta)
  ));
  RequestContentRepaint();

  return true;
}

void AsyncPanZoomController::CancelAnimation() {
  mState = NOTHING;
}

void AsyncPanZoomController::SetCompositorParent(CompositorParent* aCompositorParent) {
  mCompositorParent = aCompositorParent;
}

void AsyncPanZoomController::ScrollBy(const gfx::Point& aOffset) {
  gfx::Point newOffset(mFrameMetrics.mViewportScrollOffset.x + aOffset.x,
                       mFrameMetrics.mViewportScrollOffset.y + aOffset.y);
  FrameMetrics metrics(mFrameMetrics);
  metrics.mViewportScrollOffset = newOffset;
  mFrameMetrics = metrics;
}

void AsyncPanZoomController::SetPageRect(const gfx::Rect& aCSSPageRect) {
  FrameMetrics metrics = mFrameMetrics;
  gfx::Rect pageSize = aCSSPageRect;
  float scale = mFrameMetrics.mResolution.width;

  
  pageSize.ScaleRoundOut(1 / scale);

  
  
  metrics.mContentRect = nsIntRect(pageSize.x, pageSize.y, pageSize.width, pageSize.height);
  metrics.mCSSContentRect = aCSSPageRect;

  mFrameMetrics = metrics;
}

void AsyncPanZoomController::ScaleWithFocus(float aScale, const nsIntPoint& aFocus) {
  FrameMetrics metrics(mFrameMetrics);

  
  float scaleFactor = aScale / metrics.mResolution.width,
        oldScale = metrics.mResolution.width;

  metrics.mResolution.width = metrics.mResolution.height = aScale;

  
  
  SetPageRect(mFrameMetrics.mCSSContentRect);

  gfx::Point scrollOffset = metrics.mViewportScrollOffset;

  scrollOffset.x += float(aFocus.x) * (scaleFactor - 1.0f) / oldScale;
  scrollOffset.y += float(aFocus.y) * (scaleFactor - 1.0f) / oldScale;

  metrics.mViewportScrollOffset = scrollOffset;

  mFrameMetrics = metrics;
}

bool AsyncPanZoomController::EnlargeDisplayPortAlongAxis(float aViewport,
                                                         float aVelocity,
                                                         float* aDisplayPortOffset,
                                                         float* aDisplayPortLength)
{
  const float MIN_SKATE_SIZE_MULTIPLIER = 2.0f;
  const float MAX_SKATE_SIZE_MULTIPLIER = 4.0f;

  if (fabsf(aVelocity) > MIN_SKATE_SPEED) {
    *aDisplayPortLength = aViewport * clamped(fabsf(aVelocity),
      MIN_SKATE_SIZE_MULTIPLIER, MAX_SKATE_SIZE_MULTIPLIER);
    *aDisplayPortOffset = aVelocity > 0 ? 0 : aViewport - *aDisplayPortLength;
    return true;
  }
  return false;
}

const nsIntRect AsyncPanZoomController::CalculatePendingDisplayPort() {
  float scale = mFrameMetrics.mResolution.width;
  nsIntRect viewport = mFrameMetrics.mViewport;
  viewport.ScaleRoundIn(1 / scale);

  gfx::Point scrollOffset = mFrameMetrics.mViewportScrollOffset;
  gfx::Point velocity = GetVelocityVector();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  const float STATIONARY_SIZE_MULTIPLIER = 2.0f;
  gfx::Rect displayPort(0, 0,
                        viewport.width * STATIONARY_SIZE_MULTIPLIER,
                        viewport.height * STATIONARY_SIZE_MULTIPLIER);

  
  
  
  bool enlargedX = EnlargeDisplayPortAlongAxis(
    viewport.width, velocity.x, &displayPort.x, &displayPort.width);
  bool enlargedY = EnlargeDisplayPortAlongAxis(
    viewport.height, velocity.y, &displayPort.y, &displayPort.height);

  if (!enlargedX && !enlargedY) {
    displayPort.x = -displayPort.width / 4;
    displayPort.y = -displayPort.height / 4;
  } else if (!enlargedX) {
    displayPort.width = viewport.width;
  } else if (!enlargedY) {
    displayPort.height = viewport.height;
  }

  gfx::Rect shiftedDisplayPort = displayPort;
  shiftedDisplayPort.MoveBy(scrollOffset.x, scrollOffset.y);
  displayPort = shiftedDisplayPort.Intersect(mFrameMetrics.mCSSContentRect);
  displayPort.MoveBy(-scrollOffset.x, -scrollOffset.y);

  
  
  displayPort.Round();
  return nsIntRect(displayPort.x, displayPort.y, displayPort.width, displayPort.height);
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
  mFrameMetrics.mDisplayPort = CalculatePendingDisplayPort();

  gfx::Point oldScrollOffset = mLastPaintRequestMetrics.mViewportScrollOffset,
             newScrollOffset = mFrameMetrics.mViewportScrollOffset;

  
  
  nsRect oldDisplayPort = nsRect(
    mLastPaintRequestMetrics.mDisplayPort.x,
    mLastPaintRequestMetrics.mDisplayPort.y,
    mLastPaintRequestMetrics.mDisplayPort.width,
    mLastPaintRequestMetrics.mDisplayPort.height);

  gfx::Rect newDisplayPort = gfx::Rect(
    mFrameMetrics.mDisplayPort.x,
    mFrameMetrics.mDisplayPort.y,
    mFrameMetrics.mDisplayPort.width,
    mFrameMetrics.mDisplayPort.height);

  oldDisplayPort.MoveBy(oldScrollOffset.x, oldScrollOffset.y);
  newDisplayPort.MoveBy(newScrollOffset.x, newScrollOffset.y);

  if (fabsf(oldDisplayPort.x - newDisplayPort.x) < EPSILON &&
      fabsf(oldDisplayPort.y - newDisplayPort.y) < EPSILON &&
      fabsf(oldDisplayPort.width - newDisplayPort.width) < EPSILON &&
      fabsf(oldDisplayPort.height - newDisplayPort.height) < EPSILON &&
      mFrameMetrics.mResolution.width == mLastPaintRequestMetrics.mResolution.width) {
    return;
  }

  if (mContentPainterStatus == CONTENT_IDLE) {
    mContentPainterStatus = CONTENT_PAINTING;
    mLastPaintRequestMetrics = mFrameMetrics;
    mGeckoContentController->RequestContentRepaint(mFrameMetrics);
  } else {
    mContentPainterStatus = CONTENT_PAINTING_AND_PAINT_PENDING;
  }
}

bool AsyncPanZoomController::SampleContentTransformForFrame(const TimeStamp& aSampleTime,
                                                            ContainerLayer* aLayer,
                                                            gfx3DMatrix* aNewTransform) {
  
  
  
  
  
  bool requestAnimationFrame = false;

  const gfx3DMatrix& currentTransform = aLayer->GetTransform();

  
  float rootScaleX = currentTransform.GetXScale(),
        rootScaleY = currentTransform.GetYScale();

  gfx::Point metricsScrollOffset(0, 0);
  gfx::Point scrollOffset;
  float localScaleX, localScaleY;
  const FrameMetrics& frame = aLayer->GetFrameMetrics();
  {
    MonitorAutoLock mon(mMonitor);

    switch (mState)
    {
    case FLING:
      
      
      requestAnimationFrame |= DoFling(aSampleTime - mLastSampleTime);
      break;
    case ANIMATING_ZOOM: {
      double animPosition = (aSampleTime - mAnimationStartTime) / ZOOM_TO_DURATION;
      if (animPosition > 1.0) {
        animPosition = 1.0;
      }
      double sampledPosition = gComputedTimingFunction->GetValue(animPosition);

      mFrameMetrics.mResolution.width = mFrameMetrics.mResolution.height =
        mEndZoomToMetrics.mResolution.width * sampledPosition +
          mStartZoomToMetrics.mResolution.width * (1 - sampledPosition);

      mFrameMetrics.mViewportScrollOffset = gfx::Point(
        mEndZoomToMetrics.mViewportScrollOffset.x * sampledPosition +
          mStartZoomToMetrics.mViewportScrollOffset.x * (1 - sampledPosition),
        mEndZoomToMetrics.mViewportScrollOffset.y * sampledPosition +
          mStartZoomToMetrics.mViewportScrollOffset.y * (1 - sampledPosition)
      );

      requestAnimationFrame = true;

      if (aSampleTime - mAnimationStartTime >= ZOOM_TO_DURATION) {
        mState = NOTHING;
        RequestContentRepaint();
      }

      break;
    }
    default:
      break;
    }

    
    
    
    
    localScaleX = mFrameMetrics.mResolution.width;
    localScaleY = mFrameMetrics.mResolution.height;

    if (frame.IsScrollable()) {
      metricsScrollOffset = frame.mViewportScrollOffset;
    }

    scrollOffset = mFrameMetrics.mViewportScrollOffset;
  }

  nsIntPoint scrollCompensation(
    NS_lround((scrollOffset.x / rootScaleX - metricsScrollOffset.x) * localScaleX),
    NS_lround((scrollOffset.y / rootScaleY - metricsScrollOffset.y) * localScaleY));

  ViewTransform treeTransform(-scrollCompensation, localScaleX, localScaleY);
  *aNewTransform = gfx3DMatrix(treeTransform) * currentTransform;

  
  
  
  aNewTransform->Scale(1.0f/aLayer->GetPreXScale(),
                       1.0f/aLayer->GetPreYScale(),
                       1);
  aNewTransform->ScalePost(1.0f/aLayer->GetPostXScale(),
                           1.0f/aLayer->GetPostYScale(),
                           1);

  mLastSampleTime = aSampleTime;

  return requestAnimationFrame;
}

void AsyncPanZoomController::NotifyLayersUpdated(const FrameMetrics& aViewportFrame, bool aIsFirstPaint) {
  MonitorAutoLock monitor(mMonitor);

  mLastContentPaintMetrics = aViewportFrame;

  if (mContentPainterStatus != CONTENT_IDLE) {
    if (mContentPainterStatus == CONTENT_PAINTING_AND_PAINT_PENDING) {
      mContentPainterStatus = CONTENT_IDLE;
      RequestContentRepaint();
    } else {
      mContentPainterStatus = CONTENT_IDLE;
    }
  }

  if (aIsFirstPaint || mFrameMetrics.IsDefault()) {
    mContentPainterStatus = CONTENT_IDLE;

    mX.CancelTouch();
    mY.CancelTouch();
    mFrameMetrics = aViewportFrame;
    mFrameMetrics.mResolution.width = 1 / mFrameMetrics.mResolution.width;
    mFrameMetrics.mResolution.height = 1 / mFrameMetrics.mResolution.height;
    SetPageRect(mFrameMetrics.mCSSContentRect);

    
    
    
    RequestContentRepaint();
  } else if (!mFrameMetrics.mCSSContentRect.IsEqualEdges(aViewportFrame.mCSSContentRect)) {
    mFrameMetrics.mCSSContentRect = aViewportFrame.mCSSContentRect;
    SetPageRect(mFrameMetrics.mCSSContentRect);
  }
}

const FrameMetrics& AsyncPanZoomController::GetFrameMetrics() {
  mMonitor.AssertCurrentThreadOwns();
  return mFrameMetrics;
}

void AsyncPanZoomController::UpdateViewportSize(int aWidth, int aHeight) {
  MonitorAutoLock mon(mMonitor);
  FrameMetrics metrics = GetFrameMetrics();
  metrics.mViewport = nsIntRect(0, 0, aWidth, aHeight);
  mFrameMetrics = metrics;
}

void AsyncPanZoomController::CancelDefaultPanZoom() {
  mDisableNextTouchBatch = true;
  if (mGestureEventListener) {
    mGestureEventListener->CancelGesture();
  }
}

void AsyncPanZoomController::ZoomToRect(const gfxRect& aRect) {
  gfx::Rect zoomToRect(gfx::Rect(aRect.x, aRect.y, aRect.width, aRect.height));

  SetState(ANIMATING_ZOOM);

  {
    MonitorAutoLock mon(mMonitor);

    nsIntRect viewport = mFrameMetrics.mViewport;
    gfx::Rect cssPageRect = mFrameMetrics.mCSSContentRect;
    gfx::Point scrollOffset = mFrameMetrics.mViewportScrollOffset;

    
    
    if (zoomToRect.IsEmpty()) {
      nsIntRect cssViewport = viewport;
      cssViewport.ScaleRoundIn(1 / mFrameMetrics.mResolution.width);
      cssViewport.MoveBy(nsIntPoint(NS_lround(scrollOffset.x), NS_lround(scrollOffset.y)));

      float y = mFrameMetrics.mViewportScrollOffset.y;
      float newHeight = cssViewport.height * cssPageRect.width / cssViewport.width;
      float dh = cssViewport.height - newHeight;

      zoomToRect = gfx::Rect(0.0f,
                             y + dh/2,
                             cssPageRect.width,
                             y + dh/2 + newHeight);
    } else {
      float targetRatio = float(viewport.width) / float(viewport.height);
      float rectRatio = zoomToRect.width / zoomToRect.height;

      if (fabsf(targetRatio - rectRatio) < EPSILON) {
        
      } else if (targetRatio < rectRatio) {
        
        float newHeight = zoomToRect.height / targetRatio;
        zoomToRect.y -= (newHeight - zoomToRect.height) / 2;
        zoomToRect.height = newHeight;
      } else { 
        
        float newWidth = targetRatio * zoomToRect.width;
        zoomToRect.x -= (newWidth - zoomToRect.width) / 2;
        zoomToRect.width = newWidth;
      }

      zoomToRect = zoomToRect.Intersect(cssPageRect);
    }

    mEndZoomToMetrics.mResolution.width = mEndZoomToMetrics.mResolution.height =
      NS_MIN(viewport.width / zoomToRect.width, viewport.height / zoomToRect.height);

    mEndZoomToMetrics.mResolution.width = mEndZoomToMetrics.mResolution.height =
      clamped(mEndZoomToMetrics.mResolution.width, MIN_ZOOM, MAX_ZOOM);

    
    zoomToRect.width = viewport.width / mEndZoomToMetrics.mResolution.width;
    zoomToRect.height = viewport.height / mEndZoomToMetrics.mResolution.height;

    
    zoomToRect = zoomToRect.Intersect(cssPageRect);

    
    mEndZoomToMetrics.mResolution.width = mEndZoomToMetrics.mResolution.height =
      NS_MAX(viewport.width / zoomToRect.width, viewport.height / zoomToRect.height);

    mStartZoomToMetrics = mFrameMetrics;
    mEndZoomToMetrics.mViewportScrollOffset =
      gfx::Point(zoomToRect.x, zoomToRect.y);

    mAnimationStartTime = TimeStamp::Now();

    ScheduleComposite();
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

void AsyncPanZoomController::SetState(PanZoomState aState) {
  MonitorAutoLock monitor(mMonitor);
  mState = aState;
}

void AsyncPanZoomController::TimeoutTouchListeners() {
  ContentReceivedTouch(false);
}

}
}
