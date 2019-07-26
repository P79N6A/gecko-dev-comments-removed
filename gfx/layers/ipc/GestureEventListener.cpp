





#include "GestureEventListener.h"
#include <math.h>                       
#include <stddef.h>                     
#include "AsyncPanZoomController.h"     
#include "mozilla/layers/APZCTreeManager.h"  
#include "base/task.h"                  
#include "mozilla/Preferences.h"        
#include "mozilla/gfx/BasePoint.h"      
#include "mozilla/mozalloc.h"           
#include "nsDebug.h"                    
#include "nsMathUtils.h"                

namespace mozilla {
namespace layers {






static const uint32_t MAX_TAP_TIME = 300;







static const float PINCH_START_THRESHOLD = 35.0f;

GestureEventListener::GestureEventListener(AsyncPanZoomController* aAsyncPanZoomController)
  : mAsyncPanZoomController(aAsyncPanZoomController),
    mState(GESTURE_NONE),
    mSpanChange(0.0f),
    mTapStartTime(0),
    mLastTapEndTime(0),
    mLastTouchInput(MultiTouchInput::MULTITOUCH_START, 0, 0)
{
}

GestureEventListener::~GestureEventListener()
{
}

nsEventStatus GestureEventListener::HandleInputEvent(const MultiTouchInput& aEvent)
{
  
  
  mLastTouchInput = aEvent;

  switch (aEvent.mType)
  {
  case MultiTouchInput::MULTITOUCH_START:
  case MultiTouchInput::MULTITOUCH_ENTER: {
    for (size_t i = 0; i < aEvent.mTouches.Length(); i++) {
      bool foundAlreadyExistingTouch = false;
      for (size_t j = 0; j < mTouches.Length(); j++) {
        if (mTouches[j].mIdentifier == aEvent.mTouches[i].mIdentifier) {
          foundAlreadyExistingTouch = true;
          break;
        }
      }

      
      if (!foundAlreadyExistingTouch) {
        mTouches.AppendElement(aEvent.mTouches[i]);
      }
    }

    size_t length = mTouches.Length();
    if (length == 1) {
      mTapStartTime = aEvent.mTime;
      mTouchStartPosition = aEvent.mTouches[0].mScreenPoint;
      if (mState == GESTURE_NONE) {
        mState = GESTURE_WAITING_SINGLE_TAP;

        mLongTapTimeoutTask =
          NewRunnableMethod(this, &GestureEventListener::TimeoutLongTap);

        mAsyncPanZoomController->PostDelayedTask(
          mLongTapTimeoutTask,
          Preferences::GetInt("ui.click_hold_context_menus.delay", 500));
      }
    } else if (length == 2) {
      
      HandleTapCancel(aEvent);
    }

    break;
  }
  case MultiTouchInput::MULTITOUCH_MOVE: {
    
    ScreenIntPoint delta = aEvent.mTouches[0].mScreenPoint - mTouchStartPosition;
    if (mTouches.Length() == 1 &&
        NS_hypot(delta.x, delta.y) > AsyncPanZoomController::GetTouchStartTolerance())
    {
      HandleTapCancel(aEvent);
    }

    size_t eventTouchesMatched = 0;
    for (size_t i = 0; i < mTouches.Length(); i++) {
      bool isTouchRemoved = true;
      for (size_t j = 0; j < aEvent.mTouches.Length(); j++) {
        if (mTouches[i].mIdentifier == aEvent.mTouches[j].mIdentifier) {
          eventTouchesMatched++;
          isTouchRemoved = false;
          mTouches[i] = aEvent.mTouches[j];
        }
      }
      if (isTouchRemoved) {
        
        mTouches.RemoveElementAt(i);
        i--;
      }
    }

    NS_WARN_IF_FALSE(eventTouchesMatched == aEvent.mTouches.Length(), "Touch moved, but not in list");

    break;
  }
  case MultiTouchInput::MULTITOUCH_END:
  case MultiTouchInput::MULTITOUCH_LEAVE: {
    for (size_t i = 0; i < aEvent.mTouches.Length(); i++) {
      bool foundAlreadyExistingTouch = false;
      for (size_t j = 0; j < mTouches.Length() && !foundAlreadyExistingTouch; j++) {
        if (aEvent.mTouches[i].mIdentifier == mTouches[j].mIdentifier) {
          foundAlreadyExistingTouch = true;
          mTouches.RemoveElementAt(j);
        }
      }
      NS_WARN_IF_FALSE(foundAlreadyExistingTouch, "Touch ended, but not in list");
    }

    if (mState == GESTURE_WAITING_DOUBLE_TAP) {
      CancelDoubleTapTimeoutTask();
      if (mTapStartTime - mLastTapEndTime > MAX_TAP_TIME ||
          aEvent.mTime - mTapStartTime > MAX_TAP_TIME) {
        
        
        TimeoutDoubleTap();
        mState = GESTURE_WAITING_SINGLE_TAP;
      } else {
        
        HandleDoubleTap(aEvent);
        mState = GESTURE_NONE;
      }
    }

    if (mState == GESTURE_LONG_TAP_UP) {
      HandleLongTapUpEvent(aEvent);
      mState = GESTURE_NONE;
    } else if (mState == GESTURE_WAITING_SINGLE_TAP &&
        aEvent.mTime - mTapStartTime > MAX_TAP_TIME) {
      
      CancelLongTapTimeoutTask();
      HandleSingleTapConfirmedEvent(aEvent);
      mState = GESTURE_NONE;
    } else if (mState == GESTURE_WAITING_SINGLE_TAP) {
      CancelLongTapTimeoutTask();
      nsEventStatus tapupEvent = HandleSingleTapUpEvent(aEvent);

      if (tapupEvent == nsEventStatus_eIgnore) {
        
        
        
        mState = GESTURE_WAITING_DOUBLE_TAP;

        mDoubleTapTimeoutTask =
          NewRunnableMethod(this, &GestureEventListener::TimeoutDoubleTap);

        mAsyncPanZoomController->PostDelayedTask(
          mDoubleTapTimeoutTask,
          MAX_TAP_TIME);

      } else if (tapupEvent == nsEventStatus_eConsumeNoDefault) {
        
        mState = GESTURE_NONE;
      }
    }

    mLastTapEndTime = aEvent.mTime;

    if (!mTouches.Length()) {
      mSpanChange = 0.0f;
    }

    break;
  }
  case MultiTouchInput::MULTITOUCH_CANCEL:
    
    break;
  }

  return HandlePinchGestureEvent(aEvent);
}

nsEventStatus GestureEventListener::HandlePinchGestureEvent(const MultiTouchInput& aEvent)
{
  nsEventStatus rv = nsEventStatus_eIgnore;

  if (aEvent.mType == MultiTouchInput::MULTITOUCH_CANCEL) {
    mTouches.Clear();
    mState = GESTURE_NONE;
    return rv;
  }

  if (mTouches.Length() > 1) {
    const ScreenIntPoint& firstTouch = mTouches[0].mScreenPoint,
                         secondTouch = mTouches[1].mScreenPoint;
    ScreenPoint focusPoint = ScreenPoint(firstTouch + secondTouch) / 2;
    ScreenIntPoint delta = secondTouch - firstTouch;
    float currentSpan = float(NS_hypot(delta.x, delta.y));

    switch (mState) {
    case GESTURE_NONE:
      mPreviousSpan = currentSpan;
      mState = GESTURE_WAITING_PINCH;
      
      
      
      
    case GESTURE_WAITING_PINCH: {
      mSpanChange += fabsf(currentSpan - mPreviousSpan);
      if (mSpanChange > PINCH_START_THRESHOLD) {
        PinchGestureInput pinchEvent(PinchGestureInput::PINCHGESTURE_START,
                                     aEvent.mTime,
                                     focusPoint,
                                     currentSpan,
                                     currentSpan,
                                     aEvent.modifiers);

        mAsyncPanZoomController->HandleInputEvent(pinchEvent);

        mState = GESTURE_PINCH;
      }

      break;
    }
    case GESTURE_PINCH: {
      PinchGestureInput pinchEvent(PinchGestureInput::PINCHGESTURE_SCALE,
                                   aEvent.mTime,
                                   focusPoint,
                                   currentSpan,
                                   mPreviousSpan,
                                   aEvent.modifiers);

      mAsyncPanZoomController->HandleInputEvent(pinchEvent);
      break;
    }
    default:
      
      break;
    }

    mPreviousSpan = currentSpan;

    rv = nsEventStatus_eConsumeNoDefault;
  } else if (mState == GESTURE_PINCH) {
    PinchGestureInput pinchEvent(PinchGestureInput::PINCHGESTURE_END,
                                 aEvent.mTime,
                                 ScreenPoint(),
                                 1.0f,
                                 1.0f,
                                 aEvent.modifiers);
    mAsyncPanZoomController->HandleInputEvent(pinchEvent);

    mState = GESTURE_NONE;

    
    
    if (mTouches.Length() == 1) {
      MultiTouchInput touchEvent(MultiTouchInput::MULTITOUCH_START,
                                 aEvent.mTime,
                                 aEvent.modifiers);
      touchEvent.mTouches.AppendElement(mTouches[0]);
      mAsyncPanZoomController->HandleInputEvent(touchEvent);

      
      
      
      mState = GESTURE_NONE;
    }

    rv = nsEventStatus_eConsumeNoDefault;
  } else if (mState == GESTURE_WAITING_PINCH) {
    mState = GESTURE_NONE;
  }

  return rv;
}

nsEventStatus GestureEventListener::HandleSingleTapUpEvent(const MultiTouchInput& aEvent)
{
  TapGestureInput tapEvent(TapGestureInput::TAPGESTURE_UP, aEvent.mTime,
      aEvent.mTouches[0].mScreenPoint, aEvent.modifiers);
  return mAsyncPanZoomController->HandleInputEvent(tapEvent);
}

nsEventStatus GestureEventListener::HandleSingleTapConfirmedEvent(const MultiTouchInput& aEvent)
{
  TapGestureInput tapEvent(TapGestureInput::TAPGESTURE_CONFIRMED, aEvent.mTime,
      aEvent.mTouches[0].mScreenPoint, aEvent.modifiers);
  return mAsyncPanZoomController->HandleInputEvent(tapEvent);
}

nsEventStatus GestureEventListener::HandleLongTapEvent(const MultiTouchInput& aEvent)
{
  TapGestureInput tapEvent(TapGestureInput::TAPGESTURE_LONG, aEvent.mTime,
      aEvent.mTouches[0].mScreenPoint, aEvent.modifiers);
  return mAsyncPanZoomController->HandleInputEvent(tapEvent);
}

nsEventStatus GestureEventListener::HandleLongTapUpEvent(const MultiTouchInput& aEvent)
{
  TapGestureInput tapEvent(TapGestureInput::TAPGESTURE_LONG_UP, aEvent.mTime,
      aEvent.mTouches[0].mScreenPoint, aEvent.modifiers);
  return mAsyncPanZoomController->HandleInputEvent(tapEvent);
}

nsEventStatus GestureEventListener::HandleTapCancel(const MultiTouchInput& aEvent)
{
  mTapStartTime = 0;

  switch (mState)
  {
  case GESTURE_WAITING_SINGLE_TAP:
    CancelLongTapTimeoutTask();
    mState = GESTURE_NONE;
    break;

  case GESTURE_WAITING_DOUBLE_TAP:
  case GESTURE_LONG_TAP_UP:
    mState = GESTURE_NONE;
    break;
  default:
    break;
  }

  return nsEventStatus_eConsumeDoDefault;
}

nsEventStatus GestureEventListener::HandleDoubleTap(const MultiTouchInput& aEvent)
{
  TapGestureInput tapEvent(TapGestureInput::TAPGESTURE_DOUBLE, aEvent.mTime,
      aEvent.mTouches[0].mScreenPoint, aEvent.modifiers);
  return mAsyncPanZoomController->HandleInputEvent(tapEvent);
}

void GestureEventListener::TimeoutDoubleTap()
{
  mDoubleTapTimeoutTask = nullptr;
  
  
  if (mState == GESTURE_WAITING_DOUBLE_TAP) {
    mState = GESTURE_NONE;

    HandleSingleTapConfirmedEvent(mLastTouchInput);
  }
}

void GestureEventListener::CancelDoubleTapTimeoutTask() {
  if (mDoubleTapTimeoutTask) {
    mDoubleTapTimeoutTask->Cancel();
    mDoubleTapTimeoutTask = nullptr;
  }
}

void GestureEventListener::TimeoutLongTap()
{
  mLongTapTimeoutTask = nullptr;
  
  if (mState == GESTURE_WAITING_SINGLE_TAP) {
    mState = GESTURE_LONG_TAP_UP;

    HandleLongTapEvent(mLastTouchInput);
  }
}

void GestureEventListener::CancelLongTapTimeoutTask() {
  if (mLongTapTimeoutTask) {
    mLongTapTimeoutTask->Cancel();
    mLongTapTimeoutTask = nullptr;
  }
}

AsyncPanZoomController* GestureEventListener::GetAsyncPanZoomController() {
  return mAsyncPanZoomController;
}

void GestureEventListener::CancelGesture() {
  mTouches.Clear();
  mState = GESTURE_NONE;
}

}
}
