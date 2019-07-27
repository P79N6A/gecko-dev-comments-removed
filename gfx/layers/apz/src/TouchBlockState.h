





#ifndef mozilla_layers_TouchBlockState_h
#define mozilla_layers_TouchBlockState_h

#include "nsTArray.h"                       
#include "InputData.h"                      

namespace mozilla {
namespace layers {
































class TouchBlockState
{
public:
  typedef uint32_t TouchBehaviorFlags;

  TouchBlockState();

  





  bool SetContentResponse(bool aPreventDefault);
  



  bool TimeoutContentResponse();
  



  bool SetAllowedTouchBehaviors(const nsTArray<TouchBehaviorFlags>& aBehaviors);
  



  bool CopyAllowedTouchBehaviorsFrom(const TouchBlockState& aOther);

  



  bool IsReadyForHandling() const;
  


  bool IsDefaultPrevented() const;

  


  void SetSingleTapOccurred();
  


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
  bool mSingleTapOccurred;
  nsTArray<MultiTouchInput> mEvents;
};

} 
} 

#endif 
