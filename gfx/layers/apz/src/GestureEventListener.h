





#ifndef mozilla_layers_GestureEventListener_h
#define mozilla_layers_GestureEventListener_h

#include "InputData.h"                  
#include "Units.h"                      
#include "mozilla/EventForwards.h"      
#include "nsAutoPtr.h"                  
#include "nsISupportsImpl.h"
#include "nsTArray.h"                   

class CancelableTask;

namespace mozilla {
namespace layers {

class AsyncPanZoomController;
















class GestureEventListener MOZ_FINAL {
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(GestureEventListener)

  explicit GestureEventListener(AsyncPanZoomController* aAsyncPanZoomController);

  
  
  

  




  nsEventStatus HandleInputEvent(const MultiTouchInput& aEvent);

  




  int32_t GetLastTouchIdentifier() const;

private:
  
  ~GestureEventListener();

  


  enum GestureState {
    
    
    
    
    GESTURE_NONE,

    
    
    
    
    
    
    GESTURE_FIRST_SINGLE_TOUCH_DOWN,

    
    
    
    
    
    
    GESTURE_FIRST_SINGLE_TOUCH_MAX_TAP_DOWN,

    
    
    
    
    GESTURE_FIRST_SINGLE_TOUCH_UP,

    
    
    
    
    GESTURE_SECOND_SINGLE_TOUCH_DOWN,

    
    
    
    GESTURE_LONG_TOUCH_DOWN,

    
    
    
    
    GESTURE_MULTI_TOUCH_DOWN,

    
    
    
    GESTURE_PINCH
  };

  



  nsEventStatus HandleInputTouchSingleStart();
  nsEventStatus HandleInputTouchMultiStart();
  nsEventStatus HandleInputTouchEnd();
  nsEventStatus HandleInputTouchMove();
  nsEventStatus HandleInputTouchCancel();
  void HandleInputTimeoutLongTap();
  void HandleInputTimeoutMaxTap();

  void TriggerSingleTapConfirmedEvent();

  bool MoveDistanceIsLarge();

  


  void SetState(GestureState aState);

  nsRefPtr<AsyncPanZoomController> mAsyncPanZoomController;

  






  nsTArray<SingleTouchData> mTouches;

  


  GestureState mState;

  





  float mSpanChange;

  



  float mPreviousSpan;

  


  MultiTouchInput mLastTouchInput;

  








  ScreenIntPoint mTouchStartPosition;

  










  CancelableTask *mLongTapTimeoutTask;
  void CancelLongTapTimeoutTask();
  void CreateLongTapTimeoutTask();

  








  CancelableTask *mMaxTapTimeoutTask;
  void CancelMaxTapTimeoutTask();
  void CreateMaxTapTimeoutTask();

};

}
}

#endif
