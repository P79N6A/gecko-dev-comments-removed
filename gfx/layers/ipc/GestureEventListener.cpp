





#include "base/basictypes.h"
#include "base/thread.h"

#include "GestureEventListener.h"
#include "AsyncPanZoomController.h"

namespace mozilla {
namespace layers {






static const int MAX_TAP_TIME = 300;

GestureEventListener::GestureEventListener(AsyncPanZoomController* aAsyncPanZoomController)
  : mAsyncPanZoomController(aAsyncPanZoomController),
    mState(GESTURE_NONE),
    mLastTouchInput(MultiTouchInput::MULTITOUCH_START, 0)
{
}

GestureEventListener::~GestureEventListener()
{
}

nsEventStatus GestureEventListener::HandleInputEvent(const InputData& aEvent)
{
  if (aEvent.mInputType != MULTITOUCH_INPUT) {
    return nsEventStatus_eIgnore;
  }

  const MultiTouchInput& event = static_cast<const MultiTouchInput&>(aEvent);
  switch (event.mType)
  {
  case MultiTouchInput::MULTITOUCH_START:
  case MultiTouchInput::MULTITOUCH_ENTER: {
    for (size_t i = 0; i < event.mTouches.Length(); i++) {
      bool foundAlreadyExistingTouch = false;
      for (size_t j = 0; j < mTouches.Length(); j++) {
        if (mTouches[j].mIdentifier == event.mTouches[i].mIdentifier) {
          foundAlreadyExistingTouch = true;
        }
      }

      NS_WARN_IF_FALSE(!foundAlreadyExistingTouch, "Tried to add a touch that already exists");

      
      
      
      if (!foundAlreadyExistingTouch) {
        mTouches.AppendElement(event.mTouches[i]);
      }
    }

    size_t length = mTouches.Length();
    if (length == 1) {
      mTapStartTime = event.mTime;
      if (mState == GESTURE_NONE) {
        mState = GESTURE_WAITING_SINGLE_TAP;
      }
    } else if (length == 2) {
      
      HandleTapCancel(event);
    }

    break;
  }
  case MultiTouchInput::MULTITOUCH_MOVE: {
    
    
    HandleTapCancel(event);

    bool foundAlreadyExistingTouch = false;
    for (size_t i = 0; i < mTouches.Length(); i++) {
      for (size_t j = 0; j < event.mTouches.Length(); j++) {
        if (mTouches[i].mIdentifier == event.mTouches[j].mIdentifier) {
          foundAlreadyExistingTouch = true;
          mTouches[i] = event.mTouches[j];
        }
      }
    }

    NS_WARN_IF_FALSE(foundAlreadyExistingTouch, "Touch moved, but not in list");

    break;
  }
  case MultiTouchInput::MULTITOUCH_END:
  case MultiTouchInput::MULTITOUCH_LEAVE: {
    bool foundAlreadyExistingTouch = false;
    for (size_t i = 0; i < event.mTouches.Length() && !foundAlreadyExistingTouch; i++) {
      for (size_t j = 0; j < mTouches.Length() && !foundAlreadyExistingTouch; j++) {
        if (event.mTouches[i].mIdentifier == mTouches[j].mIdentifier) {
          foundAlreadyExistingTouch = true;
          mTouches.RemoveElementAt(j);
        }
      }
    }

    NS_WARN_IF_FALSE(foundAlreadyExistingTouch, "Touch ended, but not in list");

    if (event.mTime - mTapStartTime <= MAX_TAP_TIME) {
      if (mState == GESTURE_WAITING_DOUBLE_TAP) {
        mDoubleTapTimeoutTask->Cancel();

        
        HandleDoubleTap(event);
        mState = GESTURE_NONE;
      } else if (mState == GESTURE_WAITING_SINGLE_TAP) {
        HandleSingleTapUpEvent(event);

        
        
        
        mState = GESTURE_WAITING_DOUBLE_TAP;

        
        
        mLastTouchInput = event;

        mDoubleTapTimeoutTask =
          NewRunnableMethod(this, &GestureEventListener::TimeoutDoubleTap);

        MessageLoop::current()->PostDelayedTask(
          FROM_HERE,
          mDoubleTapTimeoutTask,
          MAX_TAP_TIME);
      }
    }

    if (mState == GESTURE_WAITING_SINGLE_TAP) {
      mState = GESTURE_NONE;
    }

    break;
  }
  case MultiTouchInput::MULTITOUCH_CANCEL:
    
    
    
    HandlePinchGestureEvent(event, true);
    break;
  }

  return HandlePinchGestureEvent(event, false);
}

nsEventStatus GestureEventListener::HandlePinchGestureEvent(const MultiTouchInput& aEvent, bool aClearTouches)
{
  nsEventStatus rv = nsEventStatus_eIgnore;

  if (mTouches.Length() > 1 && !aClearTouches) {
    const nsIntPoint& firstTouch = mTouches[0].mScreenPoint,
                      secondTouch = mTouches[mTouches.Length() - 1].mScreenPoint;
    nsIntPoint focusPoint =
      nsIntPoint((firstTouch.x + secondTouch.x)/2,
                 (firstTouch.y + secondTouch.y)/2);
    float currentSpan =
      float(NS_hypot(firstTouch.x - secondTouch.x,
                     firstTouch.y - secondTouch.y));

    if (mState == GESTURE_NONE) {
      PinchGestureInput pinchEvent(PinchGestureInput::PINCHGESTURE_START,
                                   aEvent.mTime,
                                   focusPoint,
                                   currentSpan,
                                   currentSpan);

      mAsyncPanZoomController->HandleInputEvent(pinchEvent);

      mState = GESTURE_PINCH;
    } else {
      PinchGestureInput pinchEvent(PinchGestureInput::PINCHGESTURE_SCALE,
                                   aEvent.mTime,
                                   focusPoint,
                                   currentSpan,
                                   mPreviousSpan);

      mAsyncPanZoomController->HandleInputEvent(pinchEvent);
    }

    mPreviousSpan = currentSpan;

    rv = nsEventStatus_eConsumeNoDefault;
  } else if (mState == GESTURE_PINCH) {
    PinchGestureInput pinchEvent(PinchGestureInput::PINCHGESTURE_END,
                                 aEvent.mTime,
                                 mTouches[0].mScreenPoint,
                                 1.0f,
                                 1.0f);

    mAsyncPanZoomController->HandleInputEvent(pinchEvent);

    mState = GESTURE_NONE;

    rv = nsEventStatus_eConsumeNoDefault;
  }

  if (aClearTouches) {
    mTouches.Clear();
  }

  return rv;
}

nsEventStatus GestureEventListener::HandleSingleTapUpEvent(const MultiTouchInput& aEvent)
{
  TapGestureInput tapEvent(TapGestureInput::TAPGESTURE_UP, aEvent.mTime, aEvent.mTouches[0].mScreenPoint);
  return mAsyncPanZoomController->HandleInputEvent(tapEvent);
}

nsEventStatus GestureEventListener::HandleSingleTapConfirmedEvent(const MultiTouchInput& aEvent)
{
  TapGestureInput tapEvent(TapGestureInput::TAPGESTURE_CONFIRMED, aEvent.mTime, aEvent.mTouches[0].mScreenPoint);
  return mAsyncPanZoomController->HandleInputEvent(tapEvent);
}

nsEventStatus GestureEventListener::HandleTapCancel(const MultiTouchInput& aEvent)
{
  mTapStartTime = 0;

  switch (mState)
  {
  case GESTURE_WAITING_SINGLE_TAP:
  case GESTURE_WAITING_DOUBLE_TAP:
    mState = GESTURE_NONE;
    break;
  default:
    break;
  }

  return nsEventStatus_eConsumeDoDefault;
}

nsEventStatus GestureEventListener::HandleDoubleTap(const MultiTouchInput& aEvent)
{
  TapGestureInput tapEvent(TapGestureInput::TAPGESTURE_DOUBLE, aEvent.mTime, aEvent.mTouches[0].mScreenPoint);
  return mAsyncPanZoomController->HandleInputEvent(tapEvent);
}

void GestureEventListener::TimeoutDoubleTap()
{
  
  
  if (mState == GESTURE_WAITING_DOUBLE_TAP) {
    mState = GESTURE_NONE;

    HandleSingleTapConfirmedEvent(mLastTouchInput);
  }
}

AsyncPanZoomController* GestureEventListener::GetAsyncPanZoomController() {
  return mAsyncPanZoomController;
}

}
}
