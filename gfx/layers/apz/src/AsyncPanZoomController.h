





#ifndef mozilla_layers_AsyncPanZoomController_h
#define mozilla_layers_AsyncPanZoomController_h

#include "CrossProcessMutex.h"
#include "mozilla/layers/GeckoContentController.h"
#include "mozilla/layers/APZCTreeManager.h"
#include "mozilla/Attributes.h"
#include "mozilla/EventForwards.h"
#include "mozilla/Monitor.h"
#include "mozilla/ReentrantMonitor.h"
#include "mozilla/RefPtr.h"
#include "mozilla/UniquePtr.h"
#include "mozilla/Atomics.h"
#include "InputData.h"
#include "Axis.h"
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





















class AsyncPanZoomController {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(AsyncPanZoomController)

  typedef mozilla::MonitorAutoLock MonitorAutoLock;
  typedef mozilla::gfx::Matrix4x4 Matrix4x4;
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

  


  void UpdateZoomConstraints(const ZoomConstraints& aConstraints);

  



  ZoomConstraints GetZoomConstraints() const;

  



  void PostDelayedTask(Task* aTask, int aDelayMs);

  
  
  

  






  bool AdvanceAnimations(const TimeStamp& aSampleTime);

  bool UpdateAnimation(const TimeStamp& aSampleTime,
                       Vector<Task*>* aOutDeferredTasks);

  











  void SampleContentTransformForFrame(ViewTransform* aOutTransform,
                                      ScreenPoint& aScrollOffset,
                                      Matrix4x4* aOutOverscrollTransform);

  






  void NotifyLayersUpdated(const FrameMetrics& aLayerMetrics, bool aIsFirstPaint);

  



  void SetCompositorParent(CompositorParent* aCompositorParent);

  




  void ShareFrameMetricsAcrossProcesses();

  
  
  

  



  void Destroy();

  


  bool IsDestroyed();

  





  ViewTransform GetCurrentAsyncTransform();

  




  Matrix4x4 GetNontransientAsyncTransform();

  






  Matrix4x4 GetTransformToLastDispatchedPaint();

  





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

  


  void ClearOverscroll();

  








  TouchBehaviorFlags GetAllowedTouchBehavior(ScreenIntPoint& aPoint);

  



  bool HasScrollgrab() const { return mFrameMetrics.GetHasScrollgrab(); }

  


  bool IsPannable() const;

  




  int32_t GetLastTouchIdentifier() const;

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
    SNAP_BACK,                
  };

  
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
  nsEventStatus OnPan(const PanGestureInput& aEvent, bool aFingersOnTouchpad);
  nsEventStatus OnPanEnd(const PanGestureInput& aEvent);
  nsEventStatus OnPanMomentumStart(const PanGestureInput& aEvent);
  nsEventStatus OnPanMomentumEnd(const PanGestureInput& aEvent);

  


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

  



  ScreenPoint GetFirstTouchScreenPoint(const MultiTouchInput& aEvent);

  


  void HandlePanningWithTouchAction(double angle);

  


  void HandlePanning(double angle);

  


  void HandlePanningUpdate(float aDX, float aDY);

  



  nsEventStatus StartPanning(const MultiTouchInput& aStartPoint);

  



  void UpdateWithTouchAtDevicePoint(const MultiTouchInput& aEvent);

  


  void TrackTouch(const MultiTouchInput& aEvent);

  







  void RequestContentRepaint();

  



  void RequestContentRepaint(FrameMetrics& aFrameMetrics);

  


  void DispatchRepaintRequest(const FrameMetrics& aFrameMetrics);

  



  const FrameMetrics& GetFrameMetrics() const;

  





  void FireAsyncScrollOnTimeout();

private:
  





  bool ArePointerEventsConsumable(TouchBlockState* aBlock, uint32_t aTouchPoints);

  




  void SetState(PanZoomState aState);

  





  bool ConvertToGecko(const ScreenPoint& aPoint, CSSPoint* aOut);

  


  static bool IsTransformingState(PanZoomState aState);
  bool IsInPanningState() const;

  



  void GetOverscrollTransform(Matrix4x4* aTransform) const;

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
  mutable Monitor mRefPtrMonitor;

  
  already_AddRefed<GeckoContentController> GetGeckoContentController() const;
  already_AddRefed<GestureEventListener> GetGestureEventListener() const;

  
  bool mSharingFrameMetricsAcrossProcesses;
  

  PCompositorParent* GetSharedFrameMetricsCompositor();

protected:
  
  
  FrameMetrics mFrameMetrics;

  
  
  
  
  
  
  
  mutable ReentrantMonitor mMonitor;

  
  
  PanZoomState mState;

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

  




public:
  






  void ContentReceivedTouch(bool aPreventDefault);

  






  void SetAllowedTouchBehavior(const nsTArray<TouchBehaviorFlags>& aBehaviors);

private:
  void ScheduleContentResponseTimeout();
  void ContentResponseTimeout();
  



  void ProcessPendingInputBlocks();
  TouchBlockState* StartNewTouchBlock(bool aCopyAllowedTouchBehaviorFromCurrent);
  TouchBlockState* CurrentTouchBlock();
  bool HasReadyTouchBlock();

private:
  
  
  nsTArray<UniquePtr<TouchBlockState>> mTouchBlockQueue;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  int32_t mTouchBlockBalance;


  




private:
  UniquePtr<InputBlockState> mPanGestureState;


  



public:
  








  bool AttemptFling(ScreenPoint aVelocity,
                    const nsRefPtr<const OverscrollHandoffChain>& aOverscrollHandoffChain,
                    bool aHandoff);

private:
  friend class FlingAnimation;
  friend class OverscrollSnapBackAnimation;
  
  ScreenPoint mLastFlingVelocity;
  
  TimeStamp mLastFlingTime;

  
  
  
  
  
  void HandleFlingOverscroll(const ScreenPoint& aVelocity,
                             const nsRefPtr<const OverscrollHandoffChain>& aOverscrollHandoffChain);

  
  void AcceptFling(const ScreenPoint& aVelocity,
                   const nsRefPtr<const OverscrollHandoffChain>& aOverscrollHandoffChain,
                   bool aHandoff,
                   bool aAllowOverscroll);

  
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
  FrameMetrics::ViewID GetScrollHandoffParentId() const {
    return mFrameMetrics.GetScrollParentId();
  }

  

















  bool AttemptScroll(const ScreenPoint& aStartPoint, const ScreenPoint& aEndPoint,
                     const OverscrollHandoffChain& aOverscrollHandoffChain,
                     uint32_t aOverscrollHandoffChainIndex = 0);

  void FlushRepaintForOverscrollHandoff();

  



  bool SnapBackIfOverscrolled();

private:
  




  bool CallDispatchScroll(const ScreenPoint& aStartPoint,
                          const ScreenPoint& aEndPoint,
                          const OverscrollHandoffChain& aOverscrollHandoffChain,
                          uint32_t aOverscrollHandoffChainIndex);

  





  bool OverscrollBy(const CSSPoint& aOverscroll);

  


















  nsRefPtr<const OverscrollHandoffChain> BuildOverscrollHandoffChain();

  




public:
  void SetLayerHitTestData(const nsIntRegion& aRegion, const Matrix4x4& aTransformToLayer) {
    mVisibleRegion = aRegion;
    mAncestorTransform = aTransformToLayer;
  }

  void AddHitTestRegion(const nsIntRegion& aRegion) {
    mVisibleRegion.OrWith(aRegion);
  }

  Matrix4x4 GetAncestorTransform() const {
    return mAncestorTransform;
  }

  bool VisibleRegionContains(const ParentLayerPoint& aPoint) const {
    ParentLayerIntPoint point = RoundedToInt(aPoint);
    return mVisibleRegion.Contains(point.x, point.y);
  }

  bool IsOverscrolled() const {
    return mX.IsOverscrolled() || mY.IsOverscrolled();
  }

private:
  



  nsIntRegion mVisibleRegion;
  

  Matrix4x4 mAncestorTransform;


  



private:
  

  const uint32_t mAPZCId;

  nsRefPtr<ipc::SharedMemoryBasic> mSharedFrameMetricsBuffer;
  CrossProcessMutex* mSharedLock;
  



  void UpdateSharedCompositorFrameMetrics();
  




  void ShareCompositorFrameMetrics();


  



public:
  




  static void SetFrameTime(const TimeStamp& aMilliseconds);
  




  static void SetThreadAssertionsEnabled(bool aEnabled);
  static bool GetThreadAssertionsEnabled();
  




  static void AssertOnControllerThread();
  


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
  explicit AsyncPanZoomAnimation(const TimeDuration& aRepaintInterval =
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
