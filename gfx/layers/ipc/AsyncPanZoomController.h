





#ifndef mozilla_layers_AsyncPanZoomController_h
#define mozilla_layers_AsyncPanZoomController_h

#include "GeckoContentController.h"
#include "mozilla/Attributes.h"
#include "mozilla/EventForwards.h"
#include "mozilla/Monitor.h"
#include "mozilla/ReentrantMonitor.h"
#include "mozilla/RefPtr.h"
#include "InputData.h"
#include "Axis.h"
#include "TaskThrottler.h"
#include "gfx3DMatrix.h"

#include "base/message_loop.h"

namespace mozilla {
namespace layers {

struct ScrollableLayerGuid;
class CompositorParent;
class GestureEventListener;
class ContainerLayer;
class ViewTransform;
class APZCTreeManager;





















class AsyncPanZoomController {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(AsyncPanZoomController)

  typedef mozilla::MonitorAutoLock MonitorAutoLock;

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
  ~AsyncPanZoomController();

  
  
  

  




  static void InitializeGlobalState();

  
  
  

  





  nsEventStatus ReceiveInputEvent(const InputData& aEvent);

  






  void UpdateCompositionBounds(const ScreenIntRect& aCompositionBounds);

  







  void CancelDefaultPanZoom();

  



  void DetectScrollableSubframe();

  




  void ZoomToRect(CSSRect aRect);

  





  void ContentReceivedTouch(bool aPreventDefault);

  




  void UpdateZoomConstraints(bool aAllowZoom,
                             const mozilla::CSSToScreenScale& aMinScale,
                             const mozilla::CSSToScreenScale& aMaxScale);

  



  void PostDelayedTask(Task* aTask, int aDelayMs);

  
  
  

  












  bool SampleContentTransformForFrame(const TimeStamp& aSampleTime,
                                      ViewTransform* aNewTransform,
                                      ScreenPoint& aScrollOffset);

  






  void NotifyLayersUpdated(const FrameMetrics& aLayerMetrics, bool aIsFirstPaint);

  



  void SetCompositorParent(CompositorParent* aCompositorParent);

  
  
  

  



  void Destroy();

  





  ViewTransform GetCurrentAsyncTransform();

  





  static const CSSRect CalculatePendingDisplayPort(
    const FrameMetrics& aFrameMetrics,
    const gfx::Point& aVelocity,
    const gfx::Point& aAcceleration,
    double aEstimatedPaintDuration);

  



  void SendAsyncScrollEvent();

  



  nsEventStatus HandleInputEvent(const InputData& aEvent);

  


  bool Matches(const ScrollableLayerGuid& aGuid);

  




  static void SetFrameTime(const TimeStamp& aMilliseconds);

  




  void UpdateScrollOffset(const CSSPoint& aScrollOffset);

  




  void CancelAnimation();

  








  void AttemptScroll(const ScreenPoint& aStartPoint, const ScreenPoint& aEndPoint);

protected:
  



  nsEventStatus OnTouchStart(const MultiTouchInput& aEvent);

  


  nsEventStatus OnTouchMove(const MultiTouchInput& aEvent);

  



  nsEventStatus OnTouchEnd(const MultiTouchInput& aEvent);

  



  nsEventStatus OnTouchCancel(const MultiTouchInput& aEvent);

  




  nsEventStatus OnScaleBegin(const PinchGestureInput& aEvent);

  



  nsEventStatus OnScale(const PinchGestureInput& aEvent);

  



  nsEventStatus OnScaleEnd(const PinchGestureInput& aEvent);

  




  nsEventStatus OnLongPress(const TapGestureInput& aEvent);

  




  nsEventStatus OnSingleTapUp(const TapGestureInput& aEvent);

  




  nsEventStatus OnSingleTapConfirmed(const TapGestureInput& aEvent);

  




  nsEventStatus OnDoubleTap(const TapGestureInput& aEvent);

  





  nsEventStatus OnCancelTap(const TapGestureInput& aEvent);

  


  void ScrollBy(const CSSPoint& aOffset);

  




  void ScaleWithFocus(float aScale,
                      const CSSPoint& aFocus);

  



  void ScheduleComposite();

  





  float PanDistance();

  


  const gfx::Point GetVelocityVector();

  


  const gfx::Point GetAccelerationVector();

  




  ScreenIntPoint& GetFirstTouchScreenPoint(const MultiTouchInput& aEvent);

  



  nsEventStatus StartPanning(const MultiTouchInput& aStartPoint);

  



  void UpdateWithTouchAtDevicePoint(const MultiTouchInput& aEvent);

  


  void TrackTouch(const MultiTouchInput& aEvent);

  







  static bool EnlargeDisplayPortAlongAxis(float aSkateSizeMultiplier,
                                          double aEstimatedPaintDuration,
                                          float aCompositionBounds,
                                          float aVelocity,
                                          float aAcceleration,
                                          float* aDisplayPortOffset,
                                          float* aDisplayPortLength);

  







  void RequestContentRepaint();

  





  bool DoFling(const TimeDuration& aDelta);

  



  const FrameMetrics& GetFrameMetrics();

  






  void TimeoutTouchListeners();

  





  void SetZoomAndResolution(const mozilla::CSSToScreenScale& aZoom);

  





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
    WAITING_LISTENERS, 


  };

  enum AxisLockMode {
    FREE,     
    STANDARD, 
    STICKY,   
  };

  static AxisLockMode GetAxisLockMode();

  




  void SetState(PanZoomState aState);

  bool IsPanningState(PanZoomState mState);

  uint64_t mLayersId;
  nsRefPtr<CompositorParent> mCompositorParent;
  TaskThrottler mPaintThrottler;

  


  nsRefPtr<GeckoContentController> mGeckoContentController;
  nsRefPtr<GestureEventListener> mGestureEventListener;
  Monitor mRefPtrMonitor;

  
  already_AddRefed<GeckoContentController> GetGeckoContentController();
  already_AddRefed<GestureEventListener> GetGestureEventListener();

protected:
  
  
  FrameMetrics mFrameMetrics;

  
  
  
  
  ReentrantMonitor mMonitor;

private:
  
  
  
  
  
  
  FrameMetrics mLastContentPaintMetrics;
  
  
  
  FrameMetrics mLastPaintRequestMetrics;

  
  
  
  
  FrameMetrics mStartZoomToMetrics;
  
  
  
  FrameMetrics mEndZoomToMetrics;

  nsTArray<MultiTouchInput> mTouchQueue;

  CancelableTask* mTouchListenerTimeoutTask;

  AxisX mX;
  AxisY mY;

  
  
  
  bool mAllowZoom;
  mozilla::CSSToScreenScale mMinZoom;
  mozilla::CSSToScreenScale mMaxZoom;

  
  
  TimeStamp mLastSampleTime;
  
  uint32_t mLastEventTime;

  
  
  TimeStamp mAnimationStartTime;

  
  
  ScreenPoint mLastZoomFocus;

  
  
  PanZoomState mState;

  
  
  TimeStamp mLastAsyncScrollTime;
  CSSPoint mLastAsyncScrollOffset;

  
  
  CSSPoint mCurrentAsyncScrollOffset;

  
  
  CancelableTask* mAsyncScrollTimeoutTask;

  
  
  bool mDisableNextTouchBatch;

  
  
  
  
  bool mHandlingTouchQueue;

  
  
  
  
  bool mDelayPanning;

  friend class Axis;

  




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

  AsyncPanZoomController* GetLastChild() const { return mLastChild; }
  AsyncPanZoomController* GetPrevSibling() const { return mPrevSibling; }
  AsyncPanZoomController* GetParent() const { return mParent; }

  


  bool IsRootForLayersId() const {
    return !mParent || (mParent->mLayersId != mLayersId);
  }

private:
  
  
  
  
  
  APZCTreeManager* mTreeManager;

  nsRefPtr<AsyncPanZoomController> mLastChild;
  nsRefPtr<AsyncPanZoomController> mPrevSibling;
  nsRefPtr<AsyncPanZoomController> mParent;

  



public:
  void SetLayerHitTestData(const LayerRect& aRect, const gfx3DMatrix& aTransformToLayer,
                           const gfx3DMatrix& aTransformForLayer) {
    mVisibleRect = aRect;
    mAncestorTransform = aTransformToLayer;
    mCSSTransform = aTransformForLayer;
  }

  gfx3DMatrix GetAncestorTransform() const {
    return mAncestorTransform;
  }

  gfx3DMatrix GetCSSTransform() const {
    return mCSSTransform;
  }

  bool VisibleRegionContains(const LayerPoint& aPoint) const {
    return mVisibleRect.Contains(aPoint);
  }

private:
  



  LayerRect mVisibleRect;
  

  gfx3DMatrix mAncestorTransform;
  
  gfx3DMatrix mCSSTransform;
};

}
}

#endif
