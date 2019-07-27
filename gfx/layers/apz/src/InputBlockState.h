





#ifndef mozilla_layers_InputBlockState_h
#define mozilla_layers_InputBlockState_h

#include "InputData.h"                      
#include "mozilla/gfx/Matrix.h"             
#include "nsAutoPtr.h"                      
#include "nsTArray.h"                       

namespace mozilla {
namespace layers {

class AsyncPanZoomController;
class OverscrollHandoffChain;
class CancelableBlockState;
class TouchBlockState;
class WheelBlockState;








class InputBlockState
{
public:
  static const uint64_t NO_BLOCK_ID = 0;

  explicit InputBlockState(const nsRefPtr<AsyncPanZoomController>& aTargetApzc,
                           bool aTargetConfirmed);
  virtual ~InputBlockState()
  {}

  bool SetConfirmedTargetApzc(const nsRefPtr<AsyncPanZoomController>& aTargetApzc);
  const nsRefPtr<AsyncPanZoomController>& GetTargetApzc() const;
  const nsRefPtr<const OverscrollHandoffChain>& GetOverscrollHandoffChain() const;
  uint64_t GetBlockId() const;

  bool IsTargetConfirmed() const;

private:
  nsRefPtr<AsyncPanZoomController> mTargetApzc;
  nsRefPtr<const OverscrollHandoffChain> mOverscrollHandoffChain;
  bool mTargetConfirmed;
  const uint64_t mBlockId;
protected:
  
  
  
  gfx::Matrix4x4 mTransformToApzc;
};













class CancelableBlockState : public InputBlockState
{
public:
  CancelableBlockState(const nsRefPtr<AsyncPanZoomController>& aTargetApzc,
                       bool aTargetConfirmed);

  virtual TouchBlockState *AsTouchBlock() {
    return nullptr;
  }
  virtual WheelBlockState *AsWheelBlock() {
    return nullptr;
  }

  





  bool SetContentResponse(bool aPreventDefault);

  



  bool TimeoutContentResponse();

  


  bool IsDefaultPrevented() const;

  




  void DispatchImmediate(const InputData& aEvent) const;

  



  virtual bool IsReadyForHandling() const;

  


  virtual bool HasEvents() const = 0;

  


  virtual void DropEvents() = 0;

  



  virtual void HandleEvents() = 0;

  



  virtual bool MustStayActive() = 0;

  


  virtual const char* Type() = 0;

private:
  bool mPreventDefault;
  bool mContentResponded;
  bool mContentResponseTimerExpired;
};




class WheelBlockState : public CancelableBlockState
{
public:
  WheelBlockState(const nsRefPtr<AsyncPanZoomController>& aTargetApzc,
                  bool aTargetConfirmed);

  bool IsReadyForHandling() const MOZ_OVERRIDE;
  bool HasEvents() const MOZ_OVERRIDE;
  void DropEvents() MOZ_OVERRIDE;
  void HandleEvents() MOZ_OVERRIDE;
  bool MustStayActive() MOZ_OVERRIDE;
  const char* Type() MOZ_OVERRIDE;

  void AddEvent(const ScrollWheelInput& aEvent);

  WheelBlockState *AsWheelBlock() MOZ_OVERRIDE {
    return this;
  }

private:
  nsTArray<ScrollWheelInput> mEvents;
};
























class TouchBlockState : public CancelableBlockState
{
public:
  explicit TouchBlockState(const nsRefPtr<AsyncPanZoomController>& aTargetApzc,
                           bool aTargetConfirmed);

  TouchBlockState *AsTouchBlock() MOZ_OVERRIDE {
    return this;
  }

  



  bool SetAllowedTouchBehaviors(const nsTArray<TouchBehaviorFlags>& aBehaviors);
  


  void CopyPropertiesFrom(const TouchBlockState& aOther);

  



  bool IsReadyForHandling() const MOZ_OVERRIDE;

  




  void SetDuringFastMotion();
  


  bool IsDuringFastMotion() const;
  





  bool SetSingleTapOccurred();
  


  bool SingleTapOccurred() const;

  


  void AddEvent(const MultiTouchInput& aEvent);

  



  bool TouchActionAllowsPinchZoom() const;
  



  bool TouchActionAllowsDoubleTapZoom() const;
  



  bool TouchActionAllowsPanningX() const;
  bool TouchActionAllowsPanningY() const;
  bool TouchActionAllowsPanningXY() const;

  bool HasEvents() const MOZ_OVERRIDE;
  void DropEvents() MOZ_OVERRIDE;
  void HandleEvents() MOZ_OVERRIDE;
  bool MustStayActive() MOZ_OVERRIDE;
  const char* Type() MOZ_OVERRIDE;

private:
  nsTArray<TouchBehaviorFlags> mAllowedTouchBehaviors;
  bool mAllowedTouchBehaviorSet;
  bool mDuringFastMotion;
  bool mSingleTapOccurred;
  nsTArray<MultiTouchInput> mEvents;
};

} 
} 

#endif 
