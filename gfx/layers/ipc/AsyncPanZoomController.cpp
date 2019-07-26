





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
     mAllowZoom(true),
     mMinZoom(MIN_ZOOM),
     mMaxZoom(MAX_ZOOM),
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
    currentZoom = mFrameMetrics.mZoom.width;
    currentScrollOffset = gfx::Point(mFrameMetrics.mScrollOffset.x,
                                     mFrameMetrics.mScrollOffset.y);
    lastScrollOffset = gfx::Point(mLastContentPaintMetrics.mScrollOffset.x,
                                  mLastContentPaintMetrics.mScrollOffset.y);
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
      
      
      {
        MonitorAutoLock monitor(mMonitor);
        
        SetZoomAndResolution(mFrameMetrics.mZoom.width);
        RequestContentRepaint();
        ScheduleComposite();
      }
      
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

    float scale = mFrameMetrics.mZoom.width;

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

    
    bool doScale = (scale < mMaxZoom && spanRatio > 1.0f) || (scale > mMinZoom && spanRatio < 1.0f);

    
    
    if (scale * spanRatio > mMaxZoom) {
      spanRatio = scale / mMaxZoom;
    } else if (scale * spanRatio < mMinZoom) {
      spanRatio = scale / mMinZoom;
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
  if (mGeckoContentController) {
    MonitorAutoLock monitor(mMonitor);

    gfx::Point point = WidgetSpaceToCompensatedViewportSpace(
      gfx::Point(aEvent.mPoint.x, aEvent.mPoint.y),
      mFrameMetrics.mZoom.width);
    mGeckoContentController->HandleSingleTap(nsIntPoint(NS_lround(point.x), NS_lround(point.y)));
    return nsEventStatus_eConsumeNoDefault;
  }
  return nsEventStatus_eIgnore;
}

nsEventStatus AsyncPanZoomController::OnSingleTapConfirmed(const TapGestureInput& aEvent) {
  
  return nsEventStatus_eIgnore;
}

nsEventStatus AsyncPanZoomController::OnDoubleTap(const TapGestureInput& aEvent) {
  if (mGeckoContentController) {
    MonitorAutoLock monitor(mMonitor);

    if (mAllowZoom) {
      gfx::Point point = WidgetSpaceToCompensatedViewportSpace(
        gfx::Point(aEvent.mPoint.x, aEvent.mPoint.y),
        mFrameMetrics.mZoom.width);
      mGeckoContentController->HandleDoubleTap(nsIntPoint(NS_lround(point.x), NS_lround(point.y)));
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

    
    
    float inverseScale = 1 / mFrameMetrics.mZoom.width;

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

  
  
  float inverseScale = 1 / mFrameMetrics.mZoom.width;

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
  gfx::Point newOffset(mFrameMetrics.mScrollOffset.x + aOffset.x,
                       mFrameMetrics.mScrollOffset.y + aOffset.y);
  FrameMetrics metrics(mFrameMetrics);
  metrics.mScrollOffset = newOffset;
  mFrameMetrics = metrics;
}

void AsyncPanZoomController::SetPageRect(const gfx::Rect& aCSSPageRect) {
  FrameMetrics metrics = mFrameMetrics;
  gfx::Rect pageSize = aCSSPageRect;
  float scale = mFrameMetrics.mZoom.width;

  
  pageSize.ScaleInverseRoundOut(scale);

  
  
  metrics.mContentRect = nsIntRect(pageSize.x, pageSize.y, pageSize.width, pageSize.height);
  metrics.mScrollableRect = aCSSPageRect;

  mFrameMetrics = metrics;
}

void AsyncPanZoomController::ScaleWithFocus(float aScale, const nsIntPoint& aFocus) {
  float scaleFactor = aScale / mFrameMetrics.mZoom.width,
        oldScale = mFrameMetrics.mZoom.width;

  SetZoomAndResolution(aScale);

  
  
  SetPageRect(mFrameMetrics.mScrollableRect);

  mFrameMetrics.mScrollOffset.x += float(aFocus.x) * (scaleFactor - 1.0f) / oldScale;
  mFrameMetrics.mScrollOffset.y += float(aFocus.y) * (scaleFactor - 1.0f) / oldScale;
}

bool AsyncPanZoomController::EnlargeDisplayPortAlongAxis(float aCompositionBounds,
                                                         float aVelocity,
                                                         float* aDisplayPortOffset,
                                                         float* aDisplayPortLength)
{
  const float MIN_SKATE_SIZE_MULTIPLIER = 2.0f;
  const float MAX_SKATE_SIZE_MULTIPLIER = 4.0f;

  if (fabsf(aVelocity) > MIN_SKATE_SPEED) {
    *aDisplayPortLength = aCompositionBounds * clamped(fabsf(aVelocity),
      MIN_SKATE_SIZE_MULTIPLIER, MAX_SKATE_SIZE_MULTIPLIER);
    *aDisplayPortOffset = aVelocity > 0 ? 0 : aCompositionBounds - *aDisplayPortLength;
    return true;
  }
  return false;
}

const gfx::Rect AsyncPanZoomController::CalculatePendingDisplayPort(
  const FrameMetrics& aFrameMetrics,
  const gfx::Point& aVelocity)
{
  float scale = aFrameMetrics.mZoom.width;
  nsIntRect compositionBounds = aFrameMetrics.mCompositionBounds;
  compositionBounds.ScaleInverseRoundIn(scale);

  gfx::Point scrollOffset = aFrameMetrics.mScrollOffset;

  const float STATIONARY_SIZE_MULTIPLIER = 2.0f;
  gfx::Rect displayPort(0, 0,
                        compositionBounds.width * STATIONARY_SIZE_MULTIPLIER,
                        compositionBounds.height * STATIONARY_SIZE_MULTIPLIER);

  
  
  
  bool enlargedX = EnlargeDisplayPortAlongAxis(
    compositionBounds.width, aVelocity.x, &displayPort.x, &displayPort.width);
  bool enlargedY = EnlargeDisplayPortAlongAxis(
    compositionBounds.height, aVelocity.y, &displayPort.y, &displayPort.height);

  if (!enlargedX && !enlargedY) {
    displayPort.x = -displayPort.width / 4;
    displayPort.y = -displayPort.height / 4;
  } else if (!enlargedX) {
    displayPort.width = compositionBounds.width;
  } else if (!enlargedY) {
    displayPort.height = compositionBounds.height;
  }

  gfx::Rect shiftedDisplayPort = displayPort;
  shiftedDisplayPort.MoveBy(scrollOffset.x, scrollOffset.y);
  displayPort = shiftedDisplayPort.Intersect(aFrameMetrics.mScrollableRect);
  displayPort.MoveBy(-scrollOffset.x, -scrollOffset.y);

  return displayPort;
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
  mFrameMetrics.mDisplayPort =
    CalculatePendingDisplayPort(mFrameMetrics, GetVelocityVector());

  gfx::Point oldScrollOffset = mLastPaintRequestMetrics.mScrollOffset,
             newScrollOffset = mFrameMetrics.mScrollOffset;

  
  
  gfx::Rect oldDisplayPort = mLastPaintRequestMetrics.mDisplayPort;
  gfx::Rect newDisplayPort = mFrameMetrics.mDisplayPort;

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

      mFrameMetrics.mZoom.width = mFrameMetrics.mZoom.height =
        mEndZoomToMetrics.mZoom.width * sampledPosition +
          mStartZoomToMetrics.mZoom.width * (1 - sampledPosition);

      mFrameMetrics.mScrollOffset = gfx::Point(
        mEndZoomToMetrics.mScrollOffset.x * sampledPosition +
          mStartZoomToMetrics.mScrollOffset.x * (1 - sampledPosition),
        mEndZoomToMetrics.mScrollOffset.y * sampledPosition +
          mStartZoomToMetrics.mScrollOffset.y * (1 - sampledPosition)
      );

      requestAnimationFrame = true;

      if (aSampleTime - mAnimationStartTime >= ZOOM_TO_DURATION) {
        
        SetZoomAndResolution(mFrameMetrics.mZoom.width);
        mState = NOTHING;
        RequestContentRepaint();
      }

      break;
    }
    default:
      break;
    }

    
    
    
    
    localScaleX = mFrameMetrics.mZoom.width;
    localScaleY = mFrameMetrics.mZoom.height;

    if (frame.IsScrollable()) {
      metricsScrollOffset = frame.GetScrollOffsetInLayerPixels();
    }

    scrollOffset = mFrameMetrics.mScrollOffset;
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

  if (aIsFirstPaint || mFrameMetrics.IsDefault()) {
    mContentPainterStatus = CONTENT_IDLE;

    mX.CancelTouch();
    mY.CancelTouch();

    
    
    nsIntRect compositionBounds = mFrameMetrics.mCompositionBounds;
    mFrameMetrics = aViewportFrame;
    mFrameMetrics.mCompositionBounds = compositionBounds;

    
    mFrameMetrics.mZoom = mFrameMetrics.mResolution;
    SetPageRect(mFrameMetrics.mScrollableRect);

    mState = NOTHING;
  } else if (!mFrameMetrics.mScrollableRect.IsEqualEdges(aViewportFrame.mScrollableRect)) {
    mFrameMetrics.mScrollableRect = aViewportFrame.mScrollableRect;
    SetPageRect(mFrameMetrics.mScrollableRect);
  }
}

const FrameMetrics& AsyncPanZoomController::GetFrameMetrics() {
  mMonitor.AssertCurrentThreadOwns();
  return mFrameMetrics;
}

void AsyncPanZoomController::UpdateCompositionBounds(const nsIntRect& aCompositionBounds) {
  MonitorAutoLock mon(mMonitor);

  nsIntRect oldCompositionBounds = mFrameMetrics.mCompositionBounds;
  mFrameMetrics.mCompositionBounds = aCompositionBounds;

  
  
  
  
  if (aCompositionBounds.width && aCompositionBounds.height &&
      oldCompositionBounds.width && oldCompositionBounds.height) {
    
    
    SetZoomAndResolution(mFrameMetrics.mResolution.width *
                         aCompositionBounds.width /
                         oldCompositionBounds.width);

    
    RequestContentRepaint();
  }
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

    nsIntRect compositionBounds = mFrameMetrics.mCompositionBounds;
    gfx::Rect cssPageRect = mFrameMetrics.mScrollableRect;
    gfx::Point scrollOffset = mFrameMetrics.mScrollOffset;

    
    
    if (zoomToRect.IsEmpty()) {
      
      nsIntRect cssCompositionBounds = compositionBounds;
      cssCompositionBounds.ScaleInverseRoundIn(mFrameMetrics.mZoom.width);
      cssCompositionBounds.MoveBy(scrollOffset.x, scrollOffset.y);

      float y = mFrameMetrics.mScrollOffset.y;
      float newHeight =
        cssCompositionBounds.height * cssPageRect.width / cssCompositionBounds.width;
      float dh = cssCompositionBounds.height - newHeight;

      zoomToRect = gfx::Rect(0.0f,
                             y + dh/2,
                             cssPageRect.width,
                             y + dh/2 + newHeight);
    } else {
      float targetRatio = float(compositionBounds.width) / float(compositionBounds.height);
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

    mEndZoomToMetrics.mZoom.width = mEndZoomToMetrics.mZoom.height =
      NS_MIN(compositionBounds.width / zoomToRect.width, compositionBounds.height / zoomToRect.height);

    mEndZoomToMetrics.mZoom.width = mEndZoomToMetrics.mZoom.height =
      clamped(float(mEndZoomToMetrics.mZoom.width),
              mMinZoom,
              mMaxZoom);

    
    zoomToRect.width = compositionBounds.width / mEndZoomToMetrics.mZoom.width;
    zoomToRect.height = compositionBounds.height / mEndZoomToMetrics.mZoom.height;

    
    zoomToRect = zoomToRect.Intersect(cssPageRect);

    
    mEndZoomToMetrics.mZoom.width = mEndZoomToMetrics.mZoom.height =
      NS_MAX(compositionBounds.width / zoomToRect.width, compositionBounds.height / zoomToRect.height);

    mStartZoomToMetrics = mFrameMetrics;
    mEndZoomToMetrics.mScrollOffset =
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

void AsyncPanZoomController::SetZoomAndResolution(float aScale) {
  mMonitor.AssertCurrentThreadOwns();
  mFrameMetrics.mResolution.width = mFrameMetrics.mResolution.height =
  mFrameMetrics.mZoom.width = mFrameMetrics.mZoom.height = aScale;
}

void AsyncPanZoomController::UpdateZoomConstraints(bool aAllowZoom,
                                                   float aMinZoom,
                                                   float aMaxZoom) {
  mAllowZoom = aAllowZoom;
  mMinZoom = aMinZoom;
  mMaxZoom = aMaxZoom;
}

}
}
