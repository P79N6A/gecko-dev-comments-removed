





#ifndef mozilla_layers_AsyncPanZoomController_h
#define mozilla_layers_AsyncPanZoomController_h

#include "CrossProcessMutex.h"
#include "mozilla/layers/GeckoContentController.h"
#include "mozilla/Attributes.h"
#include "mozilla/EventForwards.h"
#include "mozilla/Monitor.h"
#include "mozilla/ReentrantMonitor.h"
#include "mozilla/RefPtr.h"
#include "mozilla/Atomics.h"
#include "InputData.h"
#include "Axis.h"
#include "TaskThrottler.h"
#include "gfx3DMatrix.h"

#include "base/message_loop.h"

namespace mozilla {

namespace ipc {

class SharedMemoryBasic;

}

namespace layers {

struct ScrollableLayerGuid;
class CompositorParent;
class GestureEventListener;
class ContainerLayer;
class PCompositorParent;
class ViewTransform;
class APZCTreeManager;
class AsyncPanZoomAnimation;
class FlingAnimation;





















class AsyncPanZoomController {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(AsyncPanZoomController)

  typedef mozilla::MonitorAutoLock MonitorAutoLock;
  typedef uint32_t TouchBehaviorFlags;

public:
  enum GestureBehavior {
    
    
    DEFAULT_GESTURES,
    
    
    USE_GESTURE_DETECTOR
  };

  





  static float GetTouchStartTolerance();

  AsyncPanZoomController(uint64_t aLayersId,
                         APZCTreeManager* aTreeManager,
                         GeckoContentController* aController,
                         GestureBehavior aGestures = DEFAULT_GESTURES);

  
  
  

  




  static void InitializeGlobalState();

  
  
  

  





  nsEventStatus ReceiveInputEvent(const InputData& aEvent);

  




  void ZoomToRect(CSSRect aRect);

  





  void ContentReceivedTouch(bool aPreventDefault);

  


  void UpdateZoomConstraints(const ZoomConstraints& aConstraints);

  



  ZoomConstraints GetZoomConstraints() const;

  



  void PostDelayedTask(Task* aTask, int aDelayMs);

  
  
  

  bool UpdateAnimation(const TimeStamp& aSampleTime,
                       Vector<Task*>* aOutDeferredTasks);

  












  bool SampleContentTransformForFrame(const TimeStamp& aSampleTime,
                                      ViewTransform* aNewTransform,
                                      ScreenPoint& aScrollOffset);

  






  void NotifyLayersUpdated(const FrameMetrics& aLayerMetrics, bool aIsFirstPaint);

  



  void SetCompositorParent(CompositorParent* aCompositorParent);

  




  void ShareFrameMetricsAcrossProcesses();

  
  
  

  



  void Destroy();

  


  bool IsDestroyed();

  





  ViewTransform GetCurrentAsyncTransform();

  




  gfx3DMatrix GetNontransientAsyncTransform();

  






  gfx3DMatrix GetTransformToLastDispatchedPaint();

  





  static const LayerMargin CalculatePendingDisplayPort(
    const FrameMetrics& aFrameMetrics,
    const ScreenPoint& aVelocity,
    double aEstimatedPaintDuration);

  



  void SendAsyncScrollEvent();

  



  nsEventStatus HandleInputEvent(const InputData& aEvent);

  






  nsEventStatus HandleGestureEvent(const InputData& aEvent);

  


  void GetGuid(ScrollableLayerGuid* aGuidOut) const;

  


  ScrollableLayerGuid GetGuid() const;

  


  bool Matches(const ScrollableLayerGuid& aGuid);

  void StartAnimation(AsyncPanZoomAnimation* aAnimation);

  




  void CancelAnimation();

  








  TouchBehaviorFlags GetAllowedTouchBehavior(ScreenIntPoint& aPoint);

  






  void SetAllowedTouchBehavior(const nsTArray<TouchBehaviorFlags>& aBehaviors);

  



  bool HasScrollgrab() const { return mFrameMetrics.mHasScrollgrab; }

  


  bool IsPannable() const;

protected:
  
  ~AsyncPanZoomController();

  



  nsEventStatus OnTouchStart(const MultiTouchInput& aEvent);

  


  nsEventStatus OnTouchMove(const MultiTouchInput& aEvent);

  



  nsEventStatus OnTouchEnd(const MultiTouchInput& aEvent);

  



  nsEventStatus OnTouchCancel(const MultiTouchInput& aEvent);

  




  nsEventStatus OnScaleBegin(const PinchGestureInput& aEvent);

  



  nsEventStatus OnScale(const PinchGestureInput& aEvent);

  



  nsEventStatus OnScaleEnd(const PinchGestureInput& aEvent);

  


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

  





  float PanDistance();

  


  const ScreenPoint GetVelocityVector();

  




  ScreenIntPoint& GetFirstTouchScreenPoint(const MultiTouchInput& aEvent);

  


  void HandlePanningWithTouchAction(double angle, TouchBehaviorFlags value);

  


  void HandlePanning(double angle);

  



  nsEventStatus StartPanning(const MultiTouchInput& aStartPoint);

  



  void UpdateWithTouchAtDevicePoint(const MultiTouchInput& aEvent);

  


  void TrackTouch(const MultiTouchInput& aEvent);

  







  void RequestContentRepaint();

  



  void RequestContentRepaint(FrameMetrics& aFrameMetrics);

  


  void DispatchRepaintRequest(const FrameMetrics& aFrameMetrics);

  



  const FrameMetrics& GetFrameMetrics() const;

  






  void SetContentResponseTimer();

  






  void TimeoutContentResponse();

  





  void FireAsyncScrollOnTimeout();

private:
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
    WAITING_CONTENT_RESPONSE, 



    SNAP_BACK,                
  };

  
  struct TouchBlockState {

    TouchBlockState()
      :  mAllowedTouchBehaviorSet(false),
         mPreventDefault(false),
         mPreventDefaultSet(false),
         mSingleTapOccurred(false)
    {}

    
    
    
    
    nsTArray<TouchBehaviorFlags> mAllowedTouchBehaviors;

    
    bool mAllowedTouchBehaviorSet;

    
    
    bool mPreventDefault;

    
    bool mPreventDefaultSet;

    
    bool mSingleTapOccurred;
  };

  


  bool TouchActionAllowPinchZoom();

  


  bool TouchActionAllowDoubleTapZoom();

  




  TouchBehaviorFlags GetTouchBehavior(uint32_t touchIndex);

  






  void CheckContentResponse();

  




  void SetState(PanZoomState aState);

  





  bool ConvertToGecko(const ScreenPoint& aPoint, CSSPoint* aOut);

  


  bool IsTransformingState(PanZoomState aState);
  bool IsPanningState(PanZoomState mState);

  enum AxisLockMode {
    FREE,     
    STANDARD, 
    STICKY,   
  };

  static AxisLockMode GetAxisLockMode();

  
  
  
  ParentLayerPoint ToParentLayerCoords(const ScreenPoint& aPoint);

  
  
  
  void UpdateTransformScale();

  
  nsEventStatus GenerateSingleTap(const ScreenIntPoint& aPoint, mozilla::Modifiers aModifiers);

  
  void OnTouchEndOrCancel();

  uint64_t mLayersId;
  nsRefPtr<CompositorParent> mCompositorParent;
  TaskThrottler mPaintThrottler;

  


  nsRefPtr<GeckoContentController> mGeckoContentController;
  nsRefPtr<GestureEventListener> mGestureEventListener;
  Monitor mRefPtrMonitor;

  
  already_AddRefed<GeckoContentController> GetGeckoContentController();
  already_AddRefed<GestureEventListener> GetGestureEventListener();

  
  bool mSharingFrameMetricsAcrossProcesses;
  

  PCompositorParent* GetSharedFrameMetricsCompositor();

protected:
  
  
  FrameMetrics mFrameMetrics;

  
  
  
  
  
  
  
  mutable ReentrantMonitor mMonitor;

  
  
  
  
  bool mTouchActionPropertyEnabled;

private:
  
  
  
  
  
  
  FrameMetrics mLastContentPaintMetrics;
  
  
  
  FrameMetrics mLastPaintRequestMetrics;
  
  
  
  
  FrameMetrics mLastDispatchedPaintMetrics;

  nsTArray<MultiTouchInput> mTouchQueue;

  CancelableTask* mContentResponseTimeoutTask;

  AxisX mX;
  AxisY mY;

  
  
  bool mPanDirRestricted;

  
  
  
  ZoomConstraints mZoomConstraints;

  
  
  TimeStamp mLastSampleTime;
  
  uint32_t mLastEventTime;

  
  
  ParentLayerPoint mLastZoomFocus;

  
  
  PanZoomState mState;

  
  
  TimeStamp mLastAsyncScrollTime;
  CSSPoint mLastAsyncScrollOffset;

  
  
  CSSPoint mCurrentAsyncScrollOffset;

  
  
  CancelableTask* mAsyncScrollTimeoutTask;

  
  
  
  
  bool mHandlingTouchQueue;

  
  TouchBlockState mTouchBlockState;

  nsRefPtr<AsyncPanZoomAnimation> mAnimation;

  friend class Axis;


  



public:
  






  bool TakeOverFling(ScreenPoint aVelocity);

private:
  friend class FlingAnimation;
  friend class OverscrollSnapBackAnimation;
  
  ScreenPoint mLastFlingVelocity;
  
  TimeStamp mLastFlingTime;

  
  
  
  
  
  void HandleFlingOverscroll(const ScreenPoint& aVelocity);

  
  void AcceptFling(const ScreenPoint& aVelocity, bool aAllowOverscroll);

  
  void StartSnapBack();


  





public:
  void SetLastChild(AsyncPanZoomController* child) {
    mLastChild = child;
    if (child) {
      child->mParent = this;
    }
  }

  void SetPrevSibling(AsyncPanZoomController* sibling) {
    mPrevSibling = sibling;
    if (sibling) {
      sibling->mParent = mParent;
    }
  }

  
  void MakeRoot() {
    mParent = nullptr;
  }

  AsyncPanZoomController* GetLastChild() const { return mLastChild; }
  AsyncPanZoomController* GetPrevSibling() const { return mPrevSibling; }
  AsyncPanZoomController* GetParent() const { return mParent; }

  


  bool IsRootForLayersId() const {
    return !mParent || (mParent->mLayersId != mLayersId);
  }

private:
  
  
  
  
  
  Atomic<APZCTreeManager*> mTreeManager;

  nsRefPtr<AsyncPanZoomController> mLastChild;
  nsRefPtr<AsyncPanZoomController> mPrevSibling;
  nsRefPtr<AsyncPanZoomController> mParent;


  



public:
  void SetScrollHandoffParentId(FrameMetrics::ViewID aScrollParentId) {
    mScrollParentId = aScrollParentId;
  }

  FrameMetrics::ViewID GetScrollHandoffParentId() const {
    return mScrollParentId;
  }

  



















  bool AttemptScroll(const ScreenPoint& aStartPoint, const ScreenPoint& aEndPoint,
                     uint32_t aOverscrollHandoffChainIndex = 0);

  void FlushRepaintForOverscrollHandoff();

private:
  FrameMetrics::ViewID mScrollParentId;

  




  bool CallDispatchScroll(const ScreenPoint& aStartPoint, const ScreenPoint& aEndPoint,
                          uint32_t aOverscrollHandoffChainIndex);

  





  bool OverscrollBy(const CSSPoint& aOverscroll);


  




public:
  void SetLayerHitTestData(const ParentLayerRect& aRect, const gfx3DMatrix& aTransformToLayer,
                           const gfx3DMatrix& aTransformForLayer) {
    mVisibleRect = aRect;
    mAncestorTransform = aTransformToLayer;
    mCSSTransform = aTransformForLayer;
    UpdateTransformScale();
  }

  gfx3DMatrix GetAncestorTransform() const {
    return mAncestorTransform;
  }

  gfx3DMatrix GetCSSTransform() const {
    return mCSSTransform;
  }

  bool VisibleRegionContains(const ParentLayerPoint& aPoint) const {
    return mVisibleRect.Contains(aPoint);
  }

  bool IsOverscrolled() const {
    return mX.IsOverscrolled() || mY.IsOverscrolled();
  }

private:
  


  ParentLayerRect mVisibleRect;
  

  gfx3DMatrix mAncestorTransform;
  
  gfx3DMatrix mCSSTransform;


  



private:
  

  const uint32_t mAPZCId;

  ipc::SharedMemoryBasic* mSharedFrameMetricsBuffer;
  CrossProcessMutex* mSharedLock;
  



  void UpdateSharedCompositorFrameMetrics();
  




  void ShareCompositorFrameMetrics();


  



public:
  




  static void SetFrameTime(const TimeStamp& aMilliseconds);
  




  static void SetThreadAssertionsEnabled(bool aEnabled);
  static bool GetThreadAssertionsEnabled();
  


  void SetTestAsyncScrollOffset(const CSSPoint& aPoint)
  {
    mTestAsyncScrollOffset = aPoint;
  }

private:
  
  CSSPoint mTestAsyncScrollOffset;
};

class AsyncPanZoomAnimation {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(AsyncPanZoomAnimation)

public:
  AsyncPanZoomAnimation(const TimeDuration& aRepaintInterval =
                        TimeDuration::Forever())
    : mRepaintInterval(aRepaintInterval)
  { }

  virtual bool Sample(FrameMetrics& aFrameMetrics,
                      const TimeDuration& aDelta) = 0;

  




  Vector<Task*> TakeDeferredTasks() {
    Vector<Task*> result;
    mDeferredTasks.swap(result);
    return result;
  }

  




  TimeDuration mRepaintInterval;

protected:
  
  virtual ~AsyncPanZoomAnimation()
  { }

  




  Vector<Task*> mDeferredTasks;
};

}
}

#endif
