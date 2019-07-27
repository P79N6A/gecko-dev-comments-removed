





#include "GestureEventListener.h"
#include <math.h>                       
#include <stddef.h>                     
#include "AsyncPanZoomController.h"     
#include "base/task.h"                  
#include "gfxPrefs.h"                   
#include "nsDebug.h"                    
#include "nsMathUtils.h"                

#define GEL_LOG(...)


namespace mozilla {
namespace layers {






static const uint32_t MAX_TAP_TIME = 300;







static const float PINCH_START_THRESHOLD = 35.0f;

ScreenPoint GetCurrentFocus(const MultiTouchInput& aEvent)
{
  const ScreenIntPoint& firstTouch = aEvent.mTouches[0].mScreenPoint,
                       secondTouch = aEvent.mTouches[1].mScreenPoint;
  return ScreenPoint(firstTouch + secondTouch) / 2;
}

float GetCurrentSpan(const MultiTouchInput& aEvent)
{
  const ScreenIntPoint& firstTouch = aEvent.mTouches[0].mScreenPoint,
                       secondTouch = aEvent.mTouches[1].mScreenPoint;
  ScreenIntPoint delta = secondTouch - firstTouch;
  return float(NS_hypot(delta.x, delta.y));
}

GestureEventListener::GestureEventListener(AsyncPanZoomController* aAsyncPanZoomController)
  : mAsyncPanZoomController(aAsyncPanZoomController),
    mState(GESTURE_NONE),
    mSpanChange(0.0f),
    mPreviousSpan(0.0f),
    mLastTouchInput(MultiTouchInput::MULTITOUCH_START, 0, TimeStamp(), 0),
    mLongTapTimeoutTask(nullptr),
    mMaxTapTimeoutTask(nullptr)
{
}

GestureEventListener::~GestureEventListener()
{
}

nsEventStatus GestureEventListener::HandleInputEvent(const MultiTouchInput& aEvent)
{
  GEL_LOG("Receiving event type %d with %d touches in state %d\n", aEvent.mType, aEvent.mTouches.Length(), mState);

  nsEventStatus rv = nsEventStatus_eIgnore;

  
  
  mLastTouchInput = aEvent;

  switch (aEvent.mType) {
  case MultiTouchInput::MULTITOUCH_START:
    mTouches.Clear();
    for (size_t i = 0; i < aEvent.mTouches.Length(); i++) {
      mTouches.AppendElement(aEvent.mTouches[i]);
    }

    if (aEvent.mTouches.Length() == 1) {
      rv = HandleInputTouchSingleStart();
    } else {
      rv = HandleInputTouchMultiStart();
    }
    break;
  case MultiTouchInput::MULTITOUCH_MOVE:
    rv = HandleInputTouchMove();
    break;
  case MultiTouchInput::MULTITOUCH_END:
    for (size_t i = 0; i < aEvent.mTouches.Length(); i++) {
      for (size_t j = 0; j < mTouches.Length(); j++) {
        if (aEvent.mTouches[i].mIdentifier == mTouches[j].mIdentifier) {
          mTouches.RemoveElementAt(j);
          break;
        }
      }
    }

    rv = HandleInputTouchEnd();
    break;
  case MultiTouchInput::MULTITOUCH_CANCEL:
    mTouches.Clear();
    rv = HandleInputTouchCancel();
    break;
  }

  return rv;
}

int32_t GestureEventListener::GetLastTouchIdentifier() const
{
  if (mTouches.Length() != 1) {
    NS_WARNING("GetLastTouchIdentifier() called when last touch event "
               "did not have one touch");
  }
  return mTouches.IsEmpty() ? -1 : mTouches[0].mIdentifier;
}


nsEventStatus GestureEventListener::HandleInputTouchSingleStart()
{
  switch (mState) {
  case GESTURE_NONE:
    SetState(GESTURE_FIRST_SINGLE_TOUCH_DOWN);
    mTouchStartPosition = mLastTouchInput.mTouches[0].mScreenPoint;

    CreateLongTapTimeoutTask();
    CreateMaxTapTimeoutTask();
    break;
  case GESTURE_FIRST_SINGLE_TOUCH_UP:
    SetState(GESTURE_SECOND_SINGLE_TOUCH_DOWN);
    break;
  default:
    NS_WARNING("Unhandled state upon single touch start");
    SetState(GESTURE_NONE);
    break;
  }

  return nsEventStatus_eIgnore;
}

nsEventStatus GestureEventListener::HandleInputTouchMultiStart()
{
  nsEventStatus rv = nsEventStatus_eIgnore;

  switch (mState) {
  case GESTURE_NONE:
    SetState(GESTURE_MULTI_TOUCH_DOWN);
    break;
  case GESTURE_FIRST_SINGLE_TOUCH_DOWN:
    CancelLongTapTimeoutTask();
    CancelMaxTapTimeoutTask();
    SetState(GESTURE_MULTI_TOUCH_DOWN);
    
    rv = nsEventStatus_eConsumeNoDefault;
    break;
  case GESTURE_FIRST_SINGLE_TOUCH_MAX_TAP_DOWN:
    CancelLongTapTimeoutTask();
    SetState(GESTURE_MULTI_TOUCH_DOWN);
    
    rv = nsEventStatus_eConsumeNoDefault;
    break;
  case GESTURE_FIRST_SINGLE_TOUCH_UP:
    
    CancelMaxTapTimeoutTask();
    SetState(GESTURE_MULTI_TOUCH_DOWN);
    
    rv = nsEventStatus_eConsumeNoDefault;
    break;
  case GESTURE_SECOND_SINGLE_TOUCH_DOWN:
    
    CancelMaxTapTimeoutTask();
    SetState(GESTURE_MULTI_TOUCH_DOWN);
    
    rv = nsEventStatus_eConsumeNoDefault;
    break;
  case GESTURE_LONG_TOUCH_DOWN:
    SetState(GESTURE_MULTI_TOUCH_DOWN);
    break;
  case GESTURE_MULTI_TOUCH_DOWN:
  case GESTURE_PINCH:
    
    rv = nsEventStatus_eConsumeNoDefault;
    break;
  default:
    NS_WARNING("Unhandled state upon multitouch start");
    SetState(GESTURE_NONE);
    break;
  }

  return rv;
}

bool GestureEventListener::MoveDistanceIsLarge()
{
  const ScreenPoint start = mLastTouchInput.mTouches[0].mScreenPoint;
  ScreenPoint delta = start - mTouchStartPosition;
  mAsyncPanZoomController->ToGlobalScreenCoordinates(&delta, start);
  return (delta.Length() > AsyncPanZoomController::GetTouchStartTolerance());
}

nsEventStatus GestureEventListener::HandleInputTouchMove()
{
  nsEventStatus rv = nsEventStatus_eIgnore;

  switch (mState) {
  case GESTURE_NONE:
    
    break;

  case GESTURE_LONG_TOUCH_DOWN:
    if (MoveDistanceIsLarge()) {
      
      
      SetState(GESTURE_NONE);
    }
    break;

  case GESTURE_FIRST_SINGLE_TOUCH_DOWN:
  case GESTURE_FIRST_SINGLE_TOUCH_MAX_TAP_DOWN:
  case GESTURE_SECOND_SINGLE_TOUCH_DOWN: {
    
    if (MoveDistanceIsLarge()) {
      CancelLongTapTimeoutTask();
      CancelMaxTapTimeoutTask();
      SetState(GESTURE_NONE);
    }
    break;
  }

  case GESTURE_MULTI_TOUCH_DOWN: {
    if (mLastTouchInput.mTouches.Length() < 2) {
      NS_WARNING("Wrong input: less than 2 moving points in GESTURE_MULTI_TOUCH_DOWN state");
      break;
    }

    float currentSpan = GetCurrentSpan(mLastTouchInput);

    mSpanChange += fabsf(currentSpan - mPreviousSpan);
    if (mSpanChange > PINCH_START_THRESHOLD) {
      SetState(GESTURE_PINCH);
      PinchGestureInput pinchEvent(PinchGestureInput::PINCHGESTURE_START,
                                   mLastTouchInput.mTime,
                                   mLastTouchInput.mTimeStamp,
                                   GetCurrentFocus(mLastTouchInput),
                                   currentSpan,
                                   currentSpan,
                                   mLastTouchInput.modifiers);

      rv = mAsyncPanZoomController->HandleGestureEvent(pinchEvent);
    }

    mPreviousSpan = currentSpan;
    break;
  }

  case GESTURE_PINCH: {
    if (mLastTouchInput.mTouches.Length() < 2) {
      NS_WARNING("Wrong input: less than 2 moving points in GESTURE_PINCH state");
      
      rv = nsEventStatus_eConsumeNoDefault;
      break;
    }

    float currentSpan = GetCurrentSpan(mLastTouchInput);

    PinchGestureInput pinchEvent(PinchGestureInput::PINCHGESTURE_SCALE,
                                 mLastTouchInput.mTime,
                                 mLastTouchInput.mTimeStamp,
                                 GetCurrentFocus(mLastTouchInput),
                                 currentSpan,
                                 mPreviousSpan,
                                 mLastTouchInput.modifiers);

    rv = mAsyncPanZoomController->HandleGestureEvent(pinchEvent);
    mPreviousSpan = currentSpan;

    break;
  }

  default:
    NS_WARNING("Unhandled state upon touch move");
    SetState(GESTURE_NONE);
    break;
  }

  return rv;
}

nsEventStatus GestureEventListener::HandleInputTouchEnd()
{
  
  
  

  nsEventStatus rv = nsEventStatus_eIgnore;

  switch (mState) {
  case GESTURE_NONE:
    
    break;

  case GESTURE_FIRST_SINGLE_TOUCH_DOWN: {
    CancelLongTapTimeoutTask();
    CancelMaxTapTimeoutTask();
    TapGestureInput tapEvent(TapGestureInput::TAPGESTURE_UP,
                             mLastTouchInput.mTime,
                             mLastTouchInput.mTimeStamp,
                             mLastTouchInput.mTouches[0].mScreenPoint,
                             mLastTouchInput.modifiers);
    nsEventStatus tapupStatus = mAsyncPanZoomController->HandleGestureEvent(tapEvent);
    if (tapupStatus == nsEventStatus_eIgnore) {
      SetState(GESTURE_FIRST_SINGLE_TOUCH_UP);
      CreateMaxTapTimeoutTask();
    } else {
      
      SetState(GESTURE_NONE);
    }
    break;
  }

  case GESTURE_SECOND_SINGLE_TOUCH_DOWN: {
    CancelMaxTapTimeoutTask();
    SetState(GESTURE_NONE);
    TapGestureInput tapEvent(TapGestureInput::TAPGESTURE_DOUBLE,
                             mLastTouchInput.mTime,
                             mLastTouchInput.mTimeStamp,
                             mLastTouchInput.mTouches[0].mScreenPoint,
                             mLastTouchInput.modifiers);
    mAsyncPanZoomController->HandleGestureEvent(tapEvent);
    break;
  }

  case GESTURE_FIRST_SINGLE_TOUCH_MAX_TAP_DOWN:
    CancelLongTapTimeoutTask();
    SetState(GESTURE_NONE);
    TriggerSingleTapConfirmedEvent();
    break;

  case GESTURE_LONG_TOUCH_DOWN: {
    SetState(GESTURE_NONE);
    TapGestureInput tapEvent(TapGestureInput::TAPGESTURE_LONG_UP,
                             mLastTouchInput.mTime,
                             mLastTouchInput.mTimeStamp,
                             mLastTouchInput.mTouches[0].mScreenPoint,
                             mLastTouchInput.modifiers);
    mAsyncPanZoomController->HandleGestureEvent(tapEvent);
    break;
  }

  case GESTURE_MULTI_TOUCH_DOWN:
    if (mTouches.Length() < 2) {
      SetState(GESTURE_NONE);
    }
    break;

  case GESTURE_PINCH:
    if (mTouches.Length() < 2) {
      SetState(GESTURE_NONE);
      PinchGestureInput pinchEvent(PinchGestureInput::PINCHGESTURE_END,
                                   mLastTouchInput.mTime,
                                   mLastTouchInput.mTimeStamp,
                                   ScreenPoint(),
                                   1.0f,
                                   1.0f,
                                   mLastTouchInput.modifiers);
      mAsyncPanZoomController->HandleGestureEvent(pinchEvent);
    }

    rv = nsEventStatus_eConsumeNoDefault;

    break;

  default:
    NS_WARNING("Unhandled state upon touch end");
    SetState(GESTURE_NONE);
    break;
  }

  return rv;
}

nsEventStatus GestureEventListener::HandleInputTouchCancel()
{
  SetState(GESTURE_NONE);
  CancelMaxTapTimeoutTask();
  CancelLongTapTimeoutTask();
  return nsEventStatus_eIgnore;
}

void GestureEventListener::HandleInputTimeoutLongTap()
{
  GEL_LOG("Running long-tap timeout task in state %d\n", mState);

  mLongTapTimeoutTask = nullptr;

  switch (mState) {
  case GESTURE_FIRST_SINGLE_TOUCH_DOWN:
    
    
    CancelMaxTapTimeoutTask();
  case GESTURE_FIRST_SINGLE_TOUCH_MAX_TAP_DOWN: {
    SetState(GESTURE_LONG_TOUCH_DOWN);
    TapGestureInput tapEvent(TapGestureInput::TAPGESTURE_LONG,
                             mLastTouchInput.mTime,
                             mLastTouchInput.mTimeStamp,
                             mLastTouchInput.mTouches[0].mScreenPoint,
                             mLastTouchInput.modifiers);
    mAsyncPanZoomController->HandleGestureEvent(tapEvent);
    break;
  }
  default:
    NS_WARNING("Unhandled state upon long tap timeout");
    SetState(GESTURE_NONE);
    break;
  }
}

void GestureEventListener::HandleInputTimeoutMaxTap()
{
  GEL_LOG("Running max-tap timeout task in state %d\n", mState);

  mMaxTapTimeoutTask = nullptr;

  if (mState == GESTURE_FIRST_SINGLE_TOUCH_DOWN) {
    SetState(GESTURE_FIRST_SINGLE_TOUCH_MAX_TAP_DOWN);
  } else if (mState == GESTURE_FIRST_SINGLE_TOUCH_UP ||
             mState == GESTURE_SECOND_SINGLE_TOUCH_DOWN) {
    SetState(GESTURE_NONE);
    TriggerSingleTapConfirmedEvent();
  } else {
    NS_WARNING("Unhandled state upon MAX_TAP timeout");
    SetState(GESTURE_NONE);
  }
}

void GestureEventListener::TriggerSingleTapConfirmedEvent()
{
  TapGestureInput tapEvent(TapGestureInput::TAPGESTURE_CONFIRMED,
                           mLastTouchInput.mTime,
                           mLastTouchInput.mTimeStamp,
                           mLastTouchInput.mTouches[0].mScreenPoint,
                           mLastTouchInput.modifiers);
  mAsyncPanZoomController->HandleGestureEvent(tapEvent);
}

void GestureEventListener::SetState(GestureState aState)
{
  mState = aState;

  if (mState == GESTURE_NONE) {
    mSpanChange = 0.0f;
    mPreviousSpan = 0.0f;
  } else if (mState == GESTURE_MULTI_TOUCH_DOWN) {
    mPreviousSpan = GetCurrentSpan(mLastTouchInput);
  }
}

void GestureEventListener::CancelLongTapTimeoutTask()
{
  if (mState == GESTURE_SECOND_SINGLE_TOUCH_DOWN) {
    
    return;
  }

  if (mLongTapTimeoutTask) {
    mLongTapTimeoutTask->Cancel();
    mLongTapTimeoutTask = nullptr;
  }
}

void GestureEventListener::CreateLongTapTimeoutTask()
{
  mLongTapTimeoutTask =
    NewRunnableMethod(this, &GestureEventListener::HandleInputTimeoutLongTap);

  mAsyncPanZoomController->PostDelayedTask(
    mLongTapTimeoutTask,
    gfxPrefs::UiClickHoldContextMenusDelay());
}

void GestureEventListener::CancelMaxTapTimeoutTask()
{
  if (mState == GESTURE_FIRST_SINGLE_TOUCH_MAX_TAP_DOWN) {
    
    return;
  }

  if (mMaxTapTimeoutTask) {
    mMaxTapTimeoutTask->Cancel();
    mMaxTapTimeoutTask = nullptr;
  }
}

void GestureEventListener::CreateMaxTapTimeoutTask()
{
  mMaxTapTimeoutTask =
    NewRunnableMethod(this, &GestureEventListener::HandleInputTimeoutMaxTap);

  mAsyncPanZoomController->PostDelayedTask(
    mMaxTapTimeoutTask,
    MAX_TAP_TIME);
}

}
}
