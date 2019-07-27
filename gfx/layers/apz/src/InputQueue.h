




#ifndef mozilla_layers_InputQueue_h
#define mozilla_layers_InputQueue_h

#include "mozilla/EventForwards.h"
#include "mozilla/UniquePtr.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"

namespace mozilla {

class InputData;
class MultiTouchInput;
class ScrollWheelInput;

namespace layers {

class AsyncPanZoomController;
class CancelableBlockState;
class TouchBlockState;
class WheelBlockState;






class InputQueue {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(InputQueue)

public:
  InputQueue();

  





  nsEventStatus ReceiveInputEvent(const nsRefPtr<AsyncPanZoomController>& aTarget,
                                  bool aTargetConfirmed,
                                  const InputData& aEvent,
                                  uint64_t* aOutInputBlockId);
  




  void ContentReceivedInputBlock(uint64_t aInputBlockId, bool aPreventDefault);
  






  void SetConfirmedTargetApzc(uint64_t aInputBlockId, const nsRefPtr<AsyncPanZoomController>& aTargetApzc);
  






  void SetAllowedTouchBehavior(uint64_t aInputBlockId, const nsTArray<TouchBehaviorFlags>& aBehaviors);
  






  uint64_t InjectNewTouchBlock(AsyncPanZoomController* aTarget);
  


  CancelableBlockState* CurrentBlock() const;
  



  TouchBlockState* CurrentTouchBlock() const;
  



  WheelBlockState* CurrentWheelBlock() const;
  



  bool HasReadyTouchBlock() const;
  



  WheelBlockState* GetCurrentWheelTransaction() const;
  


  void Clear();

private:
  ~InputQueue();

  TouchBlockState* StartNewTouchBlock(const nsRefPtr<AsyncPanZoomController>& aTarget,
                                      bool aTargetConfirmed,
                                      bool aCopyPropertiesFromCurrent);

  



  void CancelAnimationsForNewBlock(CancelableBlockState* aBlock);

  


  void MaybeRequestContentResponse(const nsRefPtr<AsyncPanZoomController>& aTarget,
                                   CancelableBlockState* aBlock);

  nsEventStatus ReceiveTouchInput(const nsRefPtr<AsyncPanZoomController>& aTarget,
                                  bool aTargetConfirmed,
                                  const MultiTouchInput& aEvent,
                                  uint64_t* aOutInputBlockId);
  nsEventStatus ReceiveScrollWheelInput(const nsRefPtr<AsyncPanZoomController>& aTarget,
                                        bool aTargetConfirmed,
                                        const ScrollWheelInput& aEvent,
                                        uint64_t* aOutInputBlockId);

  


  void SweepDepletedBlocks();

  



  bool MaybeHandleCurrentBlock(CancelableBlockState* block,
                               const InputData& aEvent);

  void ScheduleMainThreadTimeout(const nsRefPtr<AsyncPanZoomController>& aTarget, uint64_t aInputBlockId);
  void MainThreadTimeout(const uint64_t& aInputBlockId);
  void ProcessInputBlocks();

private:
  
  
  nsTArray<UniquePtr<CancelableBlockState>> mInputBlockQueue;
};

}
}

#endif 
