





#ifndef mozilla_layers_AsyncPanZoomController_h
#define mozilla_layers_AsyncPanZoomController_h

#include "GeckoContentController.h"
#include "mozilla/Attributes.h"
#include "mozilla/Monitor.h"
#include "mozilla/RefPtr.h"
#include "mozilla/TimeStamp.h"
#include "InputData.h"
#include "Axis.h"
#include "nsContentUtils.h"

#include "base/message_loop.h"

namespace mozilla {
namespace layers {

class CompositorParent;
class GestureEventListener;
class ContainerLayer;





















class AsyncPanZoomController MOZ_FINAL {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(AsyncPanZoomController)

  typedef mozilla::MonitorAutoLock MonitorAutoLock;

public:
  enum GestureBehavior {
    
    
    DEFAULT_GESTURES,
    
    
    USE_GESTURE_DETECTOR
  };

  





  static const float TOUCH_START_TOLERANCE;

  AsyncPanZoomController(GeckoContentController* aController,
                         GestureBehavior aGestures = DEFAULT_GESTURES);
  ~AsyncPanZoomController();

  
  
  

  





  nsEventStatus ReceiveInputEvent(const InputData& aEvent);

  









  nsEventStatus ReceiveInputEvent(const nsInputEvent& aEvent,
                                  nsInputEvent* aOutEvent);

  






  void UpdateCompositionBounds(const nsIntRect& aCompositionBounds);

  







  void CancelDefaultPanZoom();

  




  void ZoomToRect(const gfxRect& aRect);

  





  void ContentReceivedTouch(bool aPreventDefault);

  




  void UpdateZoomConstraints(bool aAllowZoom, float aMinScale, float aMaxScale);

  
  
  

  













  bool SampleContentTransformForFrame(const TimeStamp& aSampleTime,
                                      ContainerLayer* aLayer,
                                      gfx3DMatrix* aNewTransform);

  





  void NotifyLayersUpdated(const FrameMetrics& aViewportFrame, bool aIsFirstPaint);

  



  void SetCompositorParent(CompositorParent* aCompositorParent);

  
  
  

  



  void SetPageRect(const gfx::Rect& aCSSPageRect);

  




  void SetDPI(int aDPI);

  



  int GetDPI();

  





  static const gfx::Rect CalculatePendingDisplayPort(
    const FrameMetrics& aFrameMetrics,
    const gfx::Point& aVelocity,
    const gfx::Point& aAcceleration,
    double aEstimatedPaintDuration);

protected:
  


  nsEventStatus HandleInputEvent(const InputData& aEvent);

  



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

  


  void ScrollBy(const gfx::Point& aOffset);

  






  void ScaleWithFocus(float aScale, const nsIntPoint& aFocus);

  



  void ScheduleComposite();

  






  void CancelAnimation();

  





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

  





  void SetZoomAndResolution(float aScale);

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

  nsRefPtr<CompositorParent> mCompositorParent;
  nsRefPtr<GeckoContentController> mGeckoContentController;
  nsRefPtr<GestureEventListener> mGestureEventListener;

  
  
  FrameMetrics mFrameMetrics;
  
  
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

  
  
  
  
  
  
  Monitor mMonitor;

  
  
  TimeStamp mLastSampleTime;
  
  int32_t mLastEventTime;

  
  
  TimeStamp mAnimationStartTime;

  
  
  nsIntPoint mLastZoomFocus;

  
  
  PanZoomState mState;

  
  nsTArray<TimeDuration> mPreviousPaintDurations;

  
  
  TimeStamp mPreviousPaintStartTime;

  int mDPI;

  
  
  
  
  bool mWaitingForContentToPaint;

  
  
  bool mDisableNextTouchBatch;

  
  
  
  
  bool mHandlingTouchQueue;

  friend class Axis;
};

}
}

#endif
