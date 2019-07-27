




#ifndef mozilla_layers_InputQueue_h
#define mozilla_layers_InputQueue_h

#include "mozilla/EventForwards.h"
#include "mozilla/UniquePtr.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"

namespace mozilla {

class InputData;

namespace layers {

class AsyncPanZoomController;
class OverscrollHandoffChain;
class TouchBlockState;






class InputQueue {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(InputQueue)

public:
  typedef uint32_t TouchBehaviorFlags;

public:
  InputQueue();

  





  nsEventStatus ReceiveInputEvent(const nsRefPtr<AsyncPanZoomController>& aTarget, const InputData& aEvent);
  







  void ContentReceivedTouch(bool aPreventDefault);
  








  void SetAllowedTouchBehavior(const nsTArray<TouchBehaviorFlags>& aBehaviors);
  





  void InjectNewTouchBlock(AsyncPanZoomController* aTarget);
  


  TouchBlockState* CurrentTouchBlock() const;
  



  bool HasReadyTouchBlock() const;

private:
  ~InputQueue();
  TouchBlockState* StartNewTouchBlock(const nsRefPtr<AsyncPanZoomController>& aTarget, bool aCopyAllowedTouchBehaviorFromCurrent);
  void ScheduleContentResponseTimeout(const nsRefPtr<AsyncPanZoomController>& aTarget);
  void ContentResponseTimeout();
  void ProcessPendingInputBlocks();

private:
  
  
  nsTArray<UniquePtr<TouchBlockState>> mTouchBlockQueue;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  int32_t mTouchBlockBalance;
};

}
}

#endif 
