





#ifndef mozilla_layers_InputBlockState_h
#define mozilla_layers_InputBlockState_h

#include "nsTArray.h"                       
#include "InputData.h"                      
#include "nsAutoPtr.h"

namespace mozilla {
namespace layers {

class AsyncPanZoomController;
class OverscrollHandoffChain;





class InputBlockState
{
public:
  static const uint64_t NO_BLOCK_ID = 0;

  explicit InputBlockState(const nsRefPtr<AsyncPanZoomController>& aTargetApzc,
                           bool aTargetConfirmed);

  bool SetConfirmedTargetApzc(const nsRefPtr<AsyncPanZoomController>& aTargetApzc);
  const nsRefPtr<AsyncPanZoomController>& GetTargetApzc() const;
  const nsRefPtr<const OverscrollHandoffChain>& GetOverscrollHandoffChain() const;
  uint64_t GetBlockId() const;

protected:
  bool IsTargetConfirmed() const;
private:
  nsRefPtr<AsyncPanZoomController> mTargetApzc;
  nsRefPtr<const OverscrollHandoffChain> mOverscrollHandoffChain;
  bool mTargetConfirmed;
  const uint64_t mBlockId;
};
































class TouchBlockState : public InputBlockState
{
public:
  typedef uint32_t TouchBehaviorFlags;

  explicit TouchBlockState(const nsRefPtr<AsyncPanZoomController>& aTargetApzc,
                           bool aTargetConfirmed);

  





  bool SetContentResponse(bool aPreventDefault);
  



  bool TimeoutContentResponse();
  



  bool SetAllowedTouchBehaviors(const nsTArray<TouchBehaviorFlags>& aBehaviors);
  



  bool CopyAllowedTouchBehaviorsFrom(const TouchBlockState& aOther);

  



  bool IsReadyForHandling() const;
  


  bool IsDefaultPrevented() const;

  


  void DisallowSingleTap();
  



  bool SetSingleTapOccurred();
  


  bool SingleTapOccurred() const;

  


  bool HasEvents() const;
  


  void AddEvent(const MultiTouchInput& aEvent);
  


  void DropEvents();
  



  MultiTouchInput RemoveFirstEvent();

  



  bool TouchActionAllowsPinchZoom() const;
  



  bool TouchActionAllowsDoubleTapZoom() const;
  



  bool TouchActionAllowsPanningX() const;
  bool TouchActionAllowsPanningY() const;
  bool TouchActionAllowsPanningXY() const;

private:
  nsTArray<TouchBehaviorFlags> mAllowedTouchBehaviors;
  bool mAllowedTouchBehaviorSet;
  bool mPreventDefault;
  bool mContentResponded;
  bool mContentResponseTimerExpired;
  bool mSingleTapDisallowed;
  bool mSingleTapOccurred;
  nsTArray<MultiTouchInput> mEvents;
};

} 
} 

#endif 
