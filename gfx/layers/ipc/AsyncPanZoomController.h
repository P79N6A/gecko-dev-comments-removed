





#ifndef mozilla_layers_AsyncPanZoomController_h
#define mozilla_layers_AsyncPanZoomController_h

#include "GeckoContentController.h"
#include "mozilla/Attributes.h"
#include "mozilla/Monitor.h"
#include "mozilla/RefPtr.h"
#include "InputData.h"
#include "Axis.h"
#include "TaskThrottler.h"
#include "mozilla/layers/APZCTreeManager.h"

#include "base/message_loop.h"

namespace mozilla {
namespace layers {

struct ScrollableLayerGuid;
class CompositorParent;
class GestureEventListener;
class ContainerLayer;
class ViewTransform;





















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
                         GeckoContentController* aController,
                         GestureBehavior aGestures = DEFAULT_GESTURES);
  ~AsyncPanZoomController();

  
  
  

  




  static void InitializeGlobalState();

  
  
  

  





  nsEventStatus ReceiveInputEvent(const InputData& aEvent);

  









  nsEventStatus ReceiveInputEvent(const nsInputEvent& aEvent,
                                  nsInputEvent* aOutEvent);

  






  void UpdateCompositionBounds(const ScreenIntRect& aCompositionBounds);

  







  void CancelDefaultPanZoom();

  



  void DetectScrollableSubframe();

  




  void ZoomToRect(CSSRect aRect);

  





  void ContentReceivedTouch(bool aPreventDefault);

  




  void UpdateZoomConstraints(bool aAllowZoom, float aMinScale, float aMaxScale);

  



  void PostDelayedTask(Task* aTask, int aDelayMs);

  
  
  

  












  bool SampleContentTransformForFrame(const TimeStamp& aSampleTime,
                                      ContainerLayer* aLayer,
                                      ViewTransform* aNewTransform,
                                      ScreenPoint& aScrollOffset);

  






  void NotifyLayersUpdated(const FrameMetrics& aLayerMetrics, bool aIsFirstPaint);

  



  void SetCompositorParent(CompositorParent* aCompositorParent);

  
  
  

  



  void Destroy();

  




  void SetDPI(int aDPI);

  



  int GetDPI();

  





  static const CSSRect CalculatePendingDisplayPort(
    const FrameMetrics& aFrameMetrics,
    const gfx::Point& aVelocity,
    const gfx::Point& aAcceleration,
    double aEstimatedPaintDuration);

  



  void SendAsyncScrollEvent();

  



  nsEventStatus HandleInputEvent(const InputData& aEvent);

  


  bool Matches(const ScrollableLayerGuid& aGuid);

  




  static void SetFrameTime(const TimeStamp& aMilliseconds);

  





  static void GetAPZCAtPoint(const ContainerLayer& aLayerTree,
                             const ScreenIntPoint& aPoint,
                             AsyncPanZoomController** aApzcOut,
                             LayerIntPoint* aRelativePointOut);

  




  void UpdateScrollOffset(const CSSPoint& aScrollOffset);

  




  void CancelAnimation();

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

  






  void ScaleWithFocus(float aScale, const ScreenPoint& aFocus);

  



  void ScheduleComposite();

  





  float PanDistance();

  


  const gfx::Point GetVelocityVector();

  


  const gfx::Point GetAccelerationVector();

  




  SingleTouchData& GetFirstSingleTouch(const MultiTouchInput& aEvent);

  



  void StartPanning(const MultiTouchInput& aStartPoint);

  



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

  





  void SetZoomAndResolution(const ScreenToScreenScale& aZoom);

  





  void FireAsyncScrollOnTimeout();

private:
  enum PanZoomState {
    NOTHING,        
    FLING,          
    TOUCHING,       
    PANNING,        
    PINCHING,       
    ANIMATING_ZOOM, 
    WAITING_LISTENERS, 


  };

  




  void SetState(PanZoomState aState);

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

  
  
  
  
  Monitor mMonitor;

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
  float mMinZoom;
  float mMaxZoom;

  
  
  TimeStamp mLastSampleTime;
  
  uint32_t mLastEventTime;

  
  
  TimeStamp mAnimationStartTime;

  
  
  ScreenPoint mLastZoomFocus;

  
  
  PanZoomState mState;

  
  
  TimeStamp mLastAsyncScrollTime;
  CSSPoint mLastAsyncScrollOffset;

  
  
  CSSPoint mCurrentAsyncScrollOffset;

  
  
  CancelableTask* mAsyncScrollTimeoutTask;

  int mDPI;

  
  
  bool mDisableNextTouchBatch;

  
  
  
  
  bool mHandlingTouchQueue;

  
  
  
  
  bool mDelayPanning;

  friend class Axis;

  




public:
  void SetLastChild(AsyncPanZoomController* child) { mLastChild = child; }
  void SetPrevSibling(AsyncPanZoomController* sibling) { mPrevSibling = sibling; }
  AsyncPanZoomController* GetLastChild() const { return mLastChild; }
  AsyncPanZoomController* GetPrevSibling() const { return mPrevSibling; }
private:
  nsRefPtr<AsyncPanZoomController> mLastChild;
  nsRefPtr<AsyncPanZoomController> mPrevSibling;
};

}
}

#endif
