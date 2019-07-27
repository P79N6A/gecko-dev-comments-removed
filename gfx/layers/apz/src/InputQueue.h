




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
class OverscrollHandoffChain;
class CancelableBlockState;
class TouchBlockState;






class InputQueue {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(InputQueue)

public:
  typedef uint32_t TouchBehaviorFlags;

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
  



  bool HasReadyTouchBlock() const;

private:
  ~InputQueue();

  TouchBlockState* StartNewTouchBlock(const nsRefPtr<AsyncPanZoomController>& aTarget,
                                      bool aTargetConfirmed,
                                      bool aCopyAllowedTouchBehaviorFromCurrent);

  



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

  


  bool MaybeHandleCurrentBlock(const nsRefPtr<AsyncPanZoomController>& aTarget,
                                      CancelableBlockState* block,
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
