





#include "GestureEventListener.h"
#include "AsyncPanZoomController.h"

namespace mozilla {
namespace layers {

GestureEventListener::GestureEventListener(AsyncPanZoomController* aAsyncPanZoomController)
  : mAsyncPanZoomController(aAsyncPanZoomController),
    mState(NoGesture)
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

    if (mTouches.Length() == 2) {
      
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

    if (event.mTime - mTouchStartTime <= MAX_TAP_TIME) {
      
      
      
      
      if (HandleSingleTapUpEvent(event) == nsEventStatus_eConsumeNoDefault) {
        return nsEventStatus_eConsumeNoDefault;
      }

      if (HandleSingleTapConfirmedEvent(event) == nsEventStatus_eConsumeNoDefault) {
        return nsEventStatus_eConsumeNoDefault;
      }
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

    if (mState == NoGesture) {
      PinchGestureInput pinchEvent(PinchGestureInput::PINCHGESTURE_START,
                                   aEvent.mTime,
                                   focusPoint,
                                   currentSpan,
                                   currentSpan);

      mAsyncPanZoomController->HandleInputEvent(pinchEvent);

      mState = InPinchGesture;
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
  } else if (mState == InPinchGesture) {
    PinchGestureInput pinchEvent(PinchGestureInput::PINCHGESTURE_END,
                                 aEvent.mTime,
                                 mTouches[0].mScreenPoint,
                                 1.0f,
                                 1.0f);
 
    mAsyncPanZoomController->HandleInputEvent(pinchEvent);

    mState = NoGesture;

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
  mAsyncPanZoomController->HandleInputEvent(tapEvent);

  return nsEventStatus_eConsumeDoDefault;
}

nsEventStatus GestureEventListener::HandleSingleTapConfirmedEvent(const MultiTouchInput& aEvent)
{
  TapGestureInput tapEvent(TapGestureInput::TAPGESTURE_CONFIRMED, aEvent.mTime, aEvent.mTouches[0].mScreenPoint);
  mAsyncPanZoomController->HandleInputEvent(tapEvent);

  return nsEventStatus_eConsumeDoDefault;
}

nsEventStatus GestureEventListener::HandleTapCancel(const MultiTouchInput& aEvent)
{
  
  
  
  
  mTouchStartTime = 0;
  return nsEventStatus_eConsumeDoDefault;
}

AsyncPanZoomController* GestureEventListener::GetAsyncPanZoomController() {
  return mAsyncPanZoomController;
}

}
}
