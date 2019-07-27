




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

  





  nsEventStatus ReceiveInputEvent(const nsRefPtr<AsyncPanZoomController>& aTarget, const InputData& aEvent, uint64_t* aOutInputBlockId);
  




  void ContentReceivedTouch(uint64_t aInputBlockId, bool aPreventDefault);
  






  void SetAllowedTouchBehavior(uint64_t aInputBlockId, const nsTArray<TouchBehaviorFlags>& aBehaviors);
  






  uint64_t InjectNewTouchBlock(AsyncPanZoomController* aTarget);
  


  TouchBlockState* CurrentTouchBlock() const;
  



  bool HasReadyTouchBlock() const;

private:
  ~InputQueue();
  TouchBlockState* StartNewTouchBlock(const nsRefPtr<AsyncPanZoomController>& aTarget, bool aCopyAllowedTouchBehaviorFromCurrent);
  void ScheduleContentResponseTimeout(const nsRefPtr<AsyncPanZoomController>& aTarget, uint64_t aInputBlockId);
  void ContentResponseTimeout(const uint64_t& aInputBlockId);
  void ProcessPendingInputBlocks();

private:
  
  
  nsTArray<UniquePtr<TouchBlockState>> mTouchBlockQueue;
};

}
}

#endif 
