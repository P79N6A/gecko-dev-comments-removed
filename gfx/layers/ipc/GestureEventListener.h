





#ifndef mozilla_layers_GestureEventListener_h
#define mozilla_layers_GestureEventListener_h

#include <stdint.h>                     
#include "InputData.h"                  
#include "Units.h"                      
#include "mozilla/Assertions.h"         
#include "mozilla/EventForwards.h"      
#include "nsAutoPtr.h"                  
#include "nsISupportsImpl.h"
#include "nsTArray.h"                   

class CancelableTask;

namespace mozilla {
namespace layers {

class AsyncPanZoomController;
















class GestureEventListener {
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(GestureEventListener)

  GestureEventListener(AsyncPanZoomController* aAsyncPanZoomController);
  ~GestureEventListener();

  
  
  

  




  nsEventStatus HandleInputEvent(const InputData& aEvent);

  




  void CancelGesture();

  



  AsyncPanZoomController* GetAsyncPanZoomController();

protected:
  enum GestureState {
    
    GESTURE_NONE,
    
    
    
    GESTURE_WAITING_PINCH,
    
    
    GESTURE_PINCH,
    
    
    
    GESTURE_WAITING_SINGLE_TAP,
    
    GESTURE_WAITING_DOUBLE_TAP
  };

  






  nsEventStatus HandlePinchGestureEvent(const MultiTouchInput& aEvent, bool aClearTouches);

  






  nsEventStatus HandleSingleTapUpEvent(const MultiTouchInput& aEvent);

  





  nsEventStatus HandleSingleTapConfirmedEvent(const MultiTouchInput& aEvent);

  



  nsEventStatus HandleLongTapEvent(const MultiTouchInput& aEvent);

  






  nsEventStatus HandleTapCancel(const MultiTouchInput& aEvent);

  






  nsEventStatus HandleDoubleTap(const MultiTouchInput& aEvent);

  






  void TimeoutDoubleTap();
  



  void TimeoutLongTap();

  nsRefPtr<AsyncPanZoomController> mAsyncPanZoomController;

  



  nsTArray<SingleTouchData> mTouches;

  


  GestureState mState;

  





  float mSpanChange;

  



  float mPreviousSpan;

  






  uint64_t mTapStartTime;

  




  uint64_t mLastTapEndTime;

  






  MultiTouchInput mLastTouchInput;

  






  CancelableTask *mDoubleTapTimeoutTask;
  inline void CancelDoubleTapTimeoutTask();

  






  CancelableTask *mLongTapTimeoutTask;
  inline void CancelLongTapTimeoutTask();

  




  ScreenIntPoint mTouchStartPosition;
};

}
}

#endif
