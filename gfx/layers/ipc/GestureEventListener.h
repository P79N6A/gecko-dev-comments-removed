





#ifndef mozilla_layers_GestureEventListener_h
#define mozilla_layers_GestureEventListener_h

#include "mozilla/RefPtr.h"
#include "InputData.h"
#include "Axis.h"

namespace mozilla {
namespace layers {
















class GestureEventListener {
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(GestureEventListener)

  GestureEventListener(AsyncPanZoomController* aAsyncPanZoomController);
  ~GestureEventListener();

  
  
  

  




  nsEventStatus HandleInputEvent(const InputData& aEvent);

  



  AsyncPanZoomController* GetAsyncPanZoomController();

protected:
  enum GestureState {
    NoGesture = 0,
    InPinchGesture
  };

  



  enum { MAX_TAP_TIME = 500 };

  






  nsEventStatus HandlePinchGestureEvent(const MultiTouchInput& aEvent, bool aClearTouches);

  






  nsEventStatus HandleSingleTapUpEvent(const MultiTouchInput& aEvent);

  





  nsEventStatus HandleSingleTapConfirmedEvent(const MultiTouchInput& aEvent);

  






  nsEventStatus HandleTapCancel(const MultiTouchInput& aEvent);

  nsRefPtr<AsyncPanZoomController> mAsyncPanZoomController;

  



  nsTArray<SingleTouchData> mTouches;
  GestureState mState;

  



  float mPreviousSpan;

  



  PRUint64 mTouchStartTime;
};

}
}

#endif
