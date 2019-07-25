





#include "CompositorParent.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/Constants.h"
#include "mozilla/Util.h"
#include "mozilla/XPCOM.h"
#include "mozilla/Monitor.h"
#include "AsyncPanZoomController.h"
#include "GestureEventListener.h"
#include "nsIThreadManager.h"
#include "nsThreadUtils.h"
#include "Layers.h"

namespace mozilla {
namespace layers {

static const float EPSILON = 0.0001;





static const PRInt32 PAN_REPAINT_INTERVAL = 250;





static const PRInt32 FLING_REPAINT_INTERVAL = 75;





static const float MIN_SKATE_SPEED = 0.5f;




static const float AXIS_LOCK_ANGLE = M_PI / 6.0;

AsyncPanZoomController::AsyncPanZoomController(GeckoContentController* aGeckoContentController,
                                               GestureBehavior aGestures)
  :  mGeckoContentController(aGeckoContentController),
     mX(this),
     mY(this),
     mMonitor("AsyncPanZoomController"),
     mLastSampleTime(TimeStamp::Now()),
     mState(NOTHING),
     mDPI(72),
     mContentPainterStatus(CONTENT_IDLE),
     mMayHaveTouchListeners(false),
     mDisableNextTouchBatch(false)
{
  if (aGestures == USE_GESTURE_DETECTOR) {
    mGestureEventListener = new GestureEventListener(this);
  }

  SetDPI(mDPI);
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
AsyncPanZoomController::HandleInputEvent(const nsInputEvent& aEvent,
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
    status = HandleInputEvent(event);
    break;
  }
  case NS_MOUSE_EVENT: {
    MultiTouchInput event(static_cast<const nsMouseEvent&>(aEvent));
    status = HandleInputEvent(event);
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
    for (PRUint32 i = 0; i < touches.Length(); ++i) {
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

nsEventStatus AsyncPanZoomController::HandleInputEvent(const InputData& aEvent) {
  nsEventStatus rv = nsEventStatus_eIgnore;

  if (mGestureEventListener) {
    nsEventStatus rv = mGestureEventListener->HandleInputEvent(aEvent);
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
  PRInt32 xPos = point.x, yPos = point.y;

  switch (mState) {
    case FLING:
      CancelAnimation();
      
    case NOTHING:
      mX.StartTouch(xPos);
      mY.StartTouch(yPos);
      mState = TOUCHING;
      break;
    case TOUCHING:
    case PANNING:
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
  if (mDisableNextTouchBatch) {
    return nsEventStatus_eIgnore;
  }

  switch (mState) {
    case FLING:
    case NOTHING:
      
      
      return nsEventStatus_eIgnore;

    case TOUCHING: {
      float panThreshold = 1.0f/2.0f * mDPI;
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
    
  case NOTHING:
    
    
    return nsEventStatus_eIgnore;

  case TOUCHING:
    mState = NOTHING;
    return nsEventStatus_eIgnore;

  case PANNING:
    {
      MonitorAutoLock monitor(mMonitor);
      ScheduleComposite();
      RequestContentRepaint();
    }
    mState = FLING;
    return nsEventStatus_eConsumeNoDefault;
  case PINCHING:
    mState = NOTHING;
    
    NS_WARNING("Gesture listener should have handled pinching in OnTouchEnd.");
    return nsEventStatus_eIgnore;
  }

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnTouchCancel(const MultiTouchInput& aEvent) {
  mState = NOTHING;
  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnScaleBegin(const PinchGestureInput& aEvent) {
  mState = PINCHING;
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
    PRInt32 xFocusChange = (mLastZoomFocus.x - focusPoint.x) / scale, yFocusChange = (mLastZoomFocus.y - focusPoint.y) / scale;
    
    
    if (mX.DisplacementWillOverscroll(xFocusChange) != Axis::OVERSCROLL_NONE) {
      xFocusChange -= mX.DisplacementWillOverscrollAmount(xFocusChange);
    }
    if (mY.DisplacementWillOverscroll(yFocusChange) != Axis::OVERSCROLL_NONE) {
      yFocusChange -= mY.DisplacementWillOverscrollAmount(yFocusChange);
    }
    ScrollBy(nsIntPoint(xFocusChange, yFocusChange));

    
    
    
    PRInt32 neededDisplacementX = 0, neededDisplacementY = 0;

    
    bool doScale = (scale < 8.0f && spanRatio > 1.0f) || (scale > 0.125f && spanRatio < 1.0f);

    
    
    if (scale * spanRatio > 8.0f) {
      spanRatio = scale / 8.0f;
    } else if (scale * spanRatio < 0.125f) {
      spanRatio = scale / 0.125f;
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
        ScrollBy(nsIntPoint(neededDisplacementX, neededDisplacementY));
      }

      ScheduleComposite();
      
      
    }

    mLastZoomFocus = focusPoint;
  }

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnScaleEnd(const PinchGestureInput& aEvent) {
  mState = PANNING;
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
  
  return nsEventStatus_eIgnore;
}

nsEventStatus AsyncPanZoomController::OnCancelTap(const TapGestureInput& aEvent) {
  
  return nsEventStatus_eIgnore;
}

float AsyncPanZoomController::PanDistance() {
  return NS_hypot(mX.PanDistance(), mY.PanDistance()) * mFrameMetrics.mResolution.width;
}

const nsPoint AsyncPanZoomController::GetVelocityVector() {
  return nsPoint(
    mX.GetVelocity(),
    mY.GetVelocity()
  );
}

void AsyncPanZoomController::StartPanning(const MultiTouchInput& aEvent) {
  SingleTouchData& touch = GetFirstSingleTouch(aEvent);

  float dx = mX.PanDistance(),
        dy = mY.PanDistance();

  double angle = atan2(dy, dx); 
  angle = fabs(angle); 

  mX.StartTouch(touch.mScreenPoint.x);
  mY.StartTouch(touch.mScreenPoint.y);
  mState = PANNING;

  if (angle < AXIS_LOCK_ANGLE || angle > (M_PI - AXIS_LOCK_ANGLE)) {
    mY.LockPanning();
  } else if (fabsf(angle - M_PI / 2) < AXIS_LOCK_ANGLE) {
    mX.LockPanning();
  }
}

void AsyncPanZoomController::UpdateWithTouchAtDevicePoint(const MultiTouchInput& aEvent) {
  SingleTouchData& touch = GetFirstSingleTouch(aEvent);
  nsIntPoint point = touch.mScreenPoint;
  PRInt32 xPos = point.x, yPos = point.y;
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

    PRInt32 xDisplacement = mX.GetDisplacementForDuration(inverseScale, timeDelta);
    PRInt32 yDisplacement = mY.GetDisplacementForDuration(inverseScale, timeDelta);
    if (!xDisplacement && !yDisplacement) {
      return;
    }

    ScrollBy(nsIntPoint(xDisplacement, yDisplacement));
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

  ScrollBy(nsIntPoint(
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

void AsyncPanZoomController::ScrollBy(const nsIntPoint& aOffset) {
  nsIntPoint newOffset(mFrameMetrics.mViewportScrollOffset.x + aOffset.x,
                       mFrameMetrics.mViewportScrollOffset.y + aOffset.y);
  FrameMetrics metrics(mFrameMetrics);
  metrics.mViewportScrollOffset = newOffset;
  mFrameMetrics = metrics;
}

void AsyncPanZoomController::SetPageRect(const gfx::Rect& aCSSPageRect) {
  FrameMetrics metrics = mFrameMetrics;
  gfx::Rect pageSize = aCSSPageRect;
  float scale = mFrameMetrics.mResolution.width;

  
  pageSize.ScaleRoundOut(scale);

  
  
  metrics.mContentRect = nsIntRect(pageSize.x, pageSize.y, pageSize.width, pageSize.height);
  metrics.mCSSContentRect = aCSSPageRect;

  mFrameMetrics = metrics;
}

void AsyncPanZoomController::ScaleWithFocus(float aScale, const nsIntPoint& aFocus) {
  FrameMetrics metrics(mFrameMetrics);

  
  float scaleFactor = aScale / metrics.mResolution.width;

  metrics.mResolution.width = metrics.mResolution.height = aScale;

  
  
  SetPageRect(mFrameMetrics.mCSSContentRect);

  nsIntPoint scrollOffset = metrics.mViewportScrollOffset;

  scrollOffset.x += aFocus.x * (scaleFactor - 1.0f);
  scrollOffset.y += aFocus.y * (scaleFactor - 1.0f);

  metrics.mViewportScrollOffset = scrollOffset;

  mFrameMetrics = metrics;
}

const nsIntRect AsyncPanZoomController::CalculatePendingDisplayPort() {
  float scale = mFrameMetrics.mResolution.width;
  nsIntRect viewport = mFrameMetrics.mViewport;
  viewport.ScaleRoundIn(1 / scale);

  nsIntPoint scrollOffset = mFrameMetrics.mViewportScrollOffset;
  nsPoint velocity = GetVelocityVector();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  const float STATIONARY_SIZE_MULTIPLIER = 2.0f;
  const float SKATE_SIZE_MULTIPLIER = 3.0f;
  gfx::Rect displayPort(0, 0,
                        viewport.width * STATIONARY_SIZE_MULTIPLIER,
                        viewport.height * STATIONARY_SIZE_MULTIPLIER);

  
  
  
  
  
  
  if (fabsf(velocity.x) > MIN_SKATE_SPEED && fabsf(velocity.y) < MIN_SKATE_SPEED) {
    displayPort.height = viewport.height;
    displayPort.width = viewport.width * SKATE_SIZE_MULTIPLIER;
    displayPort.x = velocity.x > 0 ? 0 : viewport.width - displayPort.width;
  } else if (fabsf(velocity.x) < MIN_SKATE_SPEED && fabsf(velocity.y) > MIN_SKATE_SPEED) {
    displayPort.width = viewport.width;
    displayPort.height = viewport.height * SKATE_SIZE_MULTIPLIER;
    displayPort.y = velocity.y > 0 ? 0 : viewport.height - displayPort.height;
  } else {
    displayPort.x = -displayPort.width / 4;
    displayPort.y = -displayPort.height / 4;
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

void AsyncPanZoomController::ScheduleComposite() {
  if (mCompositorParent) {
    mCompositorParent->ScheduleRenderOnCompositorThread();
  }
}

void AsyncPanZoomController::RequestContentRepaint() {
  mFrameMetrics.mDisplayPort = CalculatePendingDisplayPort();

  
  
  nsIntRect oldDisplayPort = mLastPaintRequestMetrics.mDisplayPort,
            newDisplayPort = mFrameMetrics.mDisplayPort;
  oldDisplayPort.MoveBy(mLastPaintRequestMetrics.mViewportScrollOffset);
  newDisplayPort.MoveBy(mFrameMetrics.mViewportScrollOffset);

  if (oldDisplayPort.IsEqualEdges(newDisplayPort) &&
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

  nsIntPoint metricsScrollOffset(0, 0);
  nsIntPoint scrollOffset;
  float localScaleX, localScaleY;
  const FrameMetrics& frame = aLayer->GetFrameMetrics();
  {
    MonitorAutoLock mon(mMonitor);

    
    
    requestAnimationFrame = requestAnimationFrame || DoFling(aSampleTime - mLastSampleTime);

    
    
    
    
    localScaleX = mFrameMetrics.mResolution.width;
    localScaleY = mFrameMetrics.mResolution.height;

    if (frame.IsScrollable()) {
      metricsScrollOffset = frame.mViewportScrollOffset;
    }

    scrollOffset = mFrameMetrics.mViewportScrollOffset;
  }

  nsIntPoint scrollCompensation(
    (scrollOffset.x / rootScaleX - metricsScrollOffset.x) * localScaleX,
    (scrollOffset.y / rootScaleY - metricsScrollOffset.y) * localScaleY);

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

    mX.StopTouch();
    mY.StopTouch();
    mFrameMetrics = aViewportFrame;
    mFrameMetrics.mResolution.width = 1 / mFrameMetrics.mResolution.width;
    mFrameMetrics.mResolution.height = 1 / mFrameMetrics.mResolution.height;
    SetPageRect(mFrameMetrics.mCSSContentRect);

    
    
    
    RequestContentRepaint();

    
    
    mMayHaveTouchListeners = false;
  } else if (!mFrameMetrics.mContentRect.IsEqualEdges(aViewportFrame.mContentRect)) {
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

void AsyncPanZoomController::NotifyDOMTouchListenerAdded() {
  mMayHaveTouchListeners = true;
}

void AsyncPanZoomController::CancelDefaultPanZoom() {
  mDisableNextTouchBatch = true;
}

}
}
