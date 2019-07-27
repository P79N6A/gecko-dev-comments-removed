





#ifndef mozilla_layers_AsyncPanZoomController_h
#define mozilla_layers_AsyncPanZoomController_h

#include "CrossProcessMutex.h"
#include "mozilla/layers/GeckoContentController.h"
#include "mozilla/layers/APZCTreeManager.h"
#include "mozilla/layers/AsyncPanZoomAnimation.h"
#include "mozilla/Attributes.h"
#include "mozilla/EventForwards.h"
#include "mozilla/Monitor.h"
#include "mozilla/ReentrantMonitor.h"
#include "mozilla/RefPtr.h"
#include "mozilla/UniquePtr.h"
#include "mozilla/Atomics.h"
#include "InputData.h"
#include "Axis.h"
#include "InputQueue.h"
#include "APZUtils.h"
#include "LayersTypes.h"
#include "TaskThrottler.h"
#include "mozilla/gfx/Matrix.h"
#include "nsRegion.h"

#include "base/message_loop.h"

namespace mozilla {

namespace ipc {

class SharedMemoryBasic;

}

namespace layers {

struct ScrollableLayerGuid;
class CompositorParent;
class GestureEventListener;
class PCompositorParent;
struct ViewTransform;
class AsyncPanZoomAnimation;
class FlingAnimation;
class InputBlockState;
class TouchBlockState;
class OverscrollHandoffChain;
class StateChangeNotificationBlocker;





















class AsyncPanZoomController {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(AsyncPanZoomController)

  typedef mozilla::MonitorAutoLock MonitorAutoLock;
  typedef mozilla::gfx::Matrix4x4 Matrix4x4;

public:
  enum GestureBehavior {
    
    
    DEFAULT_GESTURES,
    
    
    USE_GESTURE_DETECTOR
  };

  







  static ScreenCoord GetTouchStartTolerance();

  AsyncPanZoomController(uint64_t aLayersId,
                         APZCTreeManager* aTreeManager,
                         const nsRefPtr<InputQueue>& aInputQueue,
                         GeckoContentController* aController,
                         GestureBehavior aGestures = DEFAULT_GESTURES);

  
  
  

  




  static void InitializeGlobalState();

  
  
  

  




  void ZoomToRect(CSSRect aRect);

  


  void UpdateZoomConstraints(const ZoomConstraints& aConstraints);

  



  ZoomConstraints GetZoomConstraints() const;

  



  void PostDelayedTask(Task* aTask, int aDelayMs);

  
  
  

  






  bool AdvanceAnimations(const TimeStamp& aSampleTime);

  bool UpdateAnimation(const TimeStamp& aSampleTime,
                       Vector<Task*>* aOutDeferredTasks);

  





  void SampleContentTransformForFrame(ViewTransform* aOutTransform,
                                      ParentLayerPoint& aScrollOffset);

  



  Matrix4x4 GetOverscrollTransform() const;

  






  void NotifyLayersUpdated(const FrameMetrics& aLayerMetrics, bool aIsFirstPaint);

  



  void SetCompositorParent(CompositorParent* aCompositorParent);

  




  void ShareFrameMetricsAcrossProcesses();

  
  
  

  



  void Destroy();

  


  bool IsDestroyed() const;

  





  ViewTransform GetCurrentAsyncTransform() const;

  



  Matrix4x4 GetCurrentAsyncTransformWithOverscroll() const;

  






  Matrix4x4 GetTransformToLastDispatchedPaint() const;

  





  bool IsCurrentlyCheckerboarding() const;

  





  static const ScreenMargin CalculatePendingDisplayPort(
    const FrameMetrics& aFrameMetrics,
    const ParentLayerPoint& aVelocity,
    double aEstimatedPaintDuration);

  



  void SendAsyncScrollEvent();

  


  nsEventStatus HandleInputEvent(const InputData& aEvent,
                                 const Matrix4x4& aTransformToApzc);

  






  nsEventStatus HandleGestureEvent(const InputData& aEvent);

  


  void GetGuid(ScrollableLayerGuid* aGuidOut) const;

  


  ScrollableLayerGuid GetGuid() const;

  


  bool Matches(const ScrollableLayerGuid& aGuid);

  



  bool HasTreeManager(const APZCTreeManager* aTreeManager) const;

  void StartAnimation(AsyncPanZoomAnimation* aAnimation);

  




  void CancelAnimation(CancelAnimationFlags aFlags = Default);

  


  void ClearOverscroll();

  



  bool HasScrollgrab() const { return mFrameMetrics.GetHasScrollgrab(); }

  


  bool IsPannable() const;

  



  bool IsMovingFast() const;

  




  int32_t GetLastTouchIdentifier() const;

  





  Matrix4x4 GetTransformToThis() const;

  







  ScreenPoint ToScreenCoordinates(const ParentLayerPoint& aVector,
                                  const ParentLayerPoint& aAnchor) const;

  







  ParentLayerPoint ToParentLayerCoordinates(const ScreenPoint& aVector,
                                            const ScreenPoint& aAnchor) const;

  
  
  bool CanScroll(const ScrollWheelInput& aEvent) const;

  
  
  bool CanScroll(double aDeltaX, double aDeltaY) const;

  void NotifyMozMouseScrollEvent(const nsString& aString) const;

protected:
  
  ~AsyncPanZoomController();

  



  nsEventStatus OnTouchStart(const MultiTouchInput& aEvent);

  


  nsEventStatus OnTouchMove(const MultiTouchInput& aEvent);

  



  nsEventStatus OnTouchEnd(const MultiTouchInput& aEvent);

  



  nsEventStatus OnTouchCancel(const MultiTouchInput& aEvent);

  




  nsEventStatus OnScaleBegin(const PinchGestureInput& aEvent);

  



  nsEventStatus OnScale(const PinchGestureInput& aEvent);

  



  nsEventStatus OnScaleEnd(const PinchGestureInput& aEvent);

  


  nsEventStatus OnPanMayBegin(const PanGestureInput& aEvent);
  nsEventStatus OnPanCancelled(const PanGestureInput& aEvent);
  nsEventStatus OnPanBegin(const PanGestureInput& aEvent);
  nsEventStatus OnPan(const PanGestureInput& aEvent, ScrollSource aSource, bool aFingersOnTouchpad);
  nsEventStatus OnPanEnd(const PanGestureInput& aEvent);
  nsEventStatus OnPanMomentumStart(const PanGestureInput& aEvent);
  nsEventStatus OnPanMomentumEnd(const PanGestureInput& aEvent);

  


  nsEventStatus OnScrollWheel(const ScrollWheelInput& aEvent);

  void GetScrollWheelDelta(const ScrollWheelInput& aEvent,
                           double& aOutDeltaX,
                           double& aOutDeltaY) const;

  


  nsEventStatus OnLongPress(const TapGestureInput& aEvent);
  nsEventStatus OnLongPressUp(const TapGestureInput& aEvent);

  


  nsEventStatus OnSingleTapUp(const TapGestureInput& aEvent);

  


  nsEventStatus OnSingleTapConfirmed(const TapGestureInput& aEvent);

  


  nsEventStatus OnDoubleTap(const TapGestureInput& aEvent);

  





  nsEventStatus OnCancelTap(const TapGestureInput& aEvent);

  


  void ScrollBy(const CSSPoint& aOffset);

  




  void ScaleWithFocus(float aScale,
                      const CSSPoint& aFocus);

  



  void ScheduleComposite();

  



  void ScheduleCompositeAndMaybeRepaint();

  







  ScreenCoord PanDistance() const;

  





  ParentLayerPoint PanStart() const;

  


  const ParentLayerPoint GetVelocityVector() const;

  



  ParentLayerPoint GetFirstTouchPoint(const MultiTouchInput& aEvent);

  


  void HandlePanningWithTouchAction(double angle);

  


  void HandlePanning(double angle);

  


  void HandlePanningUpdate(const ScreenPoint& aDelta);

  



  nsEventStatus StartPanning(const MultiTouchInput& aStartPoint);

  



  void UpdateWithTouchAtDevicePoint(const MultiTouchInput& aEvent);

  


  void TrackTouch(const MultiTouchInput& aEvent);

  







  void RequestContentRepaint();

  






  void RequestContentRepaint(FrameMetrics& aFrameMetrics, bool aThrottled = true);

  


  void DispatchRepaintRequest(const FrameMetrics& aFrameMetrics);

  



  const FrameMetrics& GetFrameMetrics() const;

  




  APZCTreeManager* GetApzcTreeManager() const;

  


  const nsRefPtr<InputQueue>& GetInputQueue() const;

  





  void FireAsyncScrollOnTimeout();

  





  bool ConvertToGecko(const ParentLayerPoint& aPoint, CSSPoint* aOut);

  enum AxisLockMode {
    FREE,     
    STANDARD, 
    STICKY,   
  };

  static AxisLockMode GetAxisLockMode();

  
  nsEventStatus GenerateSingleTap(const ParentLayerPoint& aPoint, mozilla::Modifiers aModifiers);

  
  void OnTouchEndOrCancel();

  uint64_t mLayersId;
  nsRefPtr<CompositorParent> mCompositorParent;
  TaskThrottler mPaintThrottler;

  


  nsRefPtr<GeckoContentController> mGeckoContentController;
  nsRefPtr<GestureEventListener> mGestureEventListener;
  mutable Monitor mRefPtrMonitor;

  
  already_AddRefed<GeckoContentController> GetGeckoContentController() const;
  already_AddRefed<GestureEventListener> GetGestureEventListener() const;

  
  bool mSharingFrameMetricsAcrossProcesses;
  

  PCompositorParent* GetSharedFrameMetricsCompositor();

protected:
  
  
  FrameMetrics mFrameMetrics;

  
  
  
  
  
  
  
  mutable ReentrantMonitor mMonitor;

private:
  
  
  
  
  
  
  FrameMetrics mLastContentPaintMetrics;
  
  
  
  FrameMetrics mLastPaintRequestMetrics;
  
  
  
  
  FrameMetrics mLastDispatchedPaintMetrics;

  AxisX mX;
  AxisY mY;

  
  
  bool mPanDirRestricted;

  
  
  
  ZoomConstraints mZoomConstraints;

  
  
  TimeStamp mLastSampleTime;

  
  
  ParentLayerPoint mLastZoomFocus;

  
  
  TimeStamp mLastAsyncScrollTime;
  CSSPoint mLastAsyncScrollOffset;

  
  
  CSSPoint mCurrentAsyncScrollOffset;

  
  
  CancelableTask* mAsyncScrollTimeoutTask;

  nsRefPtr<AsyncPanZoomAnimation> mAnimation;

  friend class Axis;



  



protected:
  enum PanZoomState {
    NOTHING,                  
    FLING,                    
    TOUCHING,                 

    PANNING,                  
    PANNING_LOCKED_X,         
    PANNING_LOCKED_Y,         

    CROSS_SLIDING_X,          


    CROSS_SLIDING_Y,          

    PINCHING,                 
    ANIMATING_ZOOM,           
    OVERSCROLL_ANIMATION,     

    SMOOTH_SCROLL,            

  };

  
  
  PanZoomState mState;

private:
  friend class StateChangeNotificationBlocker;
  




  int mNotificationBlockers;

  




  void SetState(PanZoomState aState);
  



  void DispatchStateChangeNotification(PanZoomState aOldState, PanZoomState aNewState);
  


  static bool IsTransformingState(PanZoomState aState);
  bool IsInPanningState() const;



  




public:
  



  void FlushRepaintForNewInputBlock();

  





  bool ArePointerEventsConsumable(TouchBlockState* aBlock, uint32_t aTouchPoints);

  


  void ResetInputState();

private:
  nsRefPtr<InputQueue> mInputQueue;
  TouchBlockState* CurrentTouchBlock();
  bool HasReadyTouchBlock();


  




private:
  UniquePtr<InputBlockState> mPanGestureState;


  




public:
  








  bool AttemptFling(ParentLayerPoint aVelocity,
                    const nsRefPtr<const OverscrollHandoffChain>& aOverscrollHandoffChain,
                    bool aHandoff);

private:
  friend class FlingAnimation;
  friend class OverscrollAnimation;
  friend class SmoothScrollAnimation;
  
  ParentLayerPoint mLastFlingVelocity;
  
  TimeStamp mLastFlingTime;

  
  
  
  
  
  void HandleFlingOverscroll(const ParentLayerPoint& aVelocity,
                             const nsRefPtr<const OverscrollHandoffChain>& aOverscrollHandoffChain);

  void HandleSmoothScrollOverscroll(const ParentLayerPoint& aVelocity);

  
  void AcceptFling(const ParentLayerPoint& aVelocity,
                   const nsRefPtr<const OverscrollHandoffChain>& aOverscrollHandoffChain,
                   bool aHandoff);

  
  void StartOverscrollAnimation(const ParentLayerPoint& aVelocity);

  void StartSmoothScroll(ScrollSource aSource);

  
  bool AllowScrollHandoffInWheelTransaction() const;

  




public:
  void SetParent(AsyncPanZoomController* aParent) {
    mParent = aParent;
  }

  AsyncPanZoomController* GetParent() const {
    return mParent;
  }

  


  bool IsRootForLayersId() const {
    return !mParent || (mParent->mLayersId != mLayersId);
  }

private:
  
  
  
  
  
  Atomic<APZCTreeManager*> mTreeManager;

  nsRefPtr<AsyncPanZoomController> mParent;


  



public:
  FrameMetrics::ViewID GetScrollHandoffParentId() const {
    return mFrameMetrics.GetScrollParentId();
  }

  



















  bool AttemptScroll(const ParentLayerPoint& aStartPoint,
                     const ParentLayerPoint& aEndPoint,
                     OverscrollHandoffState& aOverscrollHandoffState);

  void FlushRepaintForOverscrollHandoff();

  



  bool SnapBackIfOverscrolled();

  


















  nsRefPtr<const OverscrollHandoffChain> BuildOverscrollHandoffChain();

private:
  




  bool CallDispatchScroll(const ParentLayerPoint& aStartPoint,
                          const ParentLayerPoint& aEndPoint,
                          OverscrollHandoffState& aOverscrollHandoffState);

  




  bool OverscrollForPanning(ParentLayerPoint aOverscroll,
                            const ScreenPoint& aPanDistance);

  





  bool OverscrollBy(const ParentLayerPoint& aOverscroll);


  




public:
  void SetAncestorTransform(const Matrix4x4& aTransformToLayer) {
    mAncestorTransform = aTransformToLayer;
  }

  Matrix4x4 GetAncestorTransform() const {
    return mAncestorTransform;
  }

  
  
  bool Contains(const ScreenIntPoint& aPoint) const;

  bool IsOverscrolled() const {
    return mX.IsOverscrolled() || mY.IsOverscrolled();
  }

private:
  

  Matrix4x4 mAncestorTransform;


  



private:
  

  const uint32_t mAPZCId;

  nsRefPtr<ipc::SharedMemoryBasic> mSharedFrameMetricsBuffer;
  CrossProcessMutex* mSharedLock;
  



  void UpdateSharedCompositorFrameMetrics();
  




  void ShareCompositorFrameMetrics();


  



public:
  




  static void SetFrameTime(const TimeStamp& aMilliseconds);
  


  void SetTestAsyncScrollOffset(const CSSPoint& aPoint)
  {
    mTestAsyncScrollOffset = aPoint;
  }

  void MarkAsyncTransformAppliedToContent()
  {
    mAsyncTransformAppliedToContent = true;
  }

  bool GetAsyncTransformAppliedToContent() const
  {
    return mAsyncTransformAppliedToContent;
  }

private:
  
  CSSPoint mTestAsyncScrollOffset;
  
  
  bool mAsyncTransformAppliedToContent;
};

}
}

#endif
