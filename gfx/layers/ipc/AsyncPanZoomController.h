





#ifndef mozilla_layers_AsyncPanZoomController_h
#define mozilla_layers_AsyncPanZoomController_h

#include "GeckoContentController.h"
#include "mozilla/Attributes.h"
#include "mozilla/Monitor.h"
#include "mozilla/RefPtr.h"
#include "mozilla/TimeStamp.h"
#include "InputData.h"
#include "Axis.h"

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

  
  
  

  





  nsEventStatus HandleInputEvent(const InputData& aEvent);

  









  nsEventStatus HandleInputEvent(const nsInputEvent& aEvent,
                                 nsInputEvent* aOutEvent);

  










  void UpdateViewportSize(int aWidth, int aHeight);

  







  void NotifyDOMTouchListenerAdded();

  







  void CancelDefaultPanZoom();

  




  void ZoomToRect(const gfxRect& aRect);

  
  
  

  













  bool SampleContentTransformForFrame(const TimeStamp& aSampleTime,
                                      ContainerLayer* aLayer,
                                      gfx3DMatrix* aNewTransform);

  





  void NotifyLayersUpdated(const FrameMetrics& aViewportFrame, bool aIsFirstPaint);

  



  void SetCompositorParent(CompositorParent* aCompositorParent);

  
  
  

  



  void SetPageRect(const gfx::Rect& aCSSPageRect);

  




  void SetDPI(int aDPI);

  



  int GetDPI();

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

  


  void ScrollBy(const nsIntPoint& aOffset);

  






  void ScaleWithFocus(float aScale, const nsIntPoint& aFocus);

  



  void ScheduleComposite();

  






  void CancelAnimation();

  





  float PanDistance();

  


  const gfx::Point GetVelocityVector();

  




  SingleTouchData& GetFirstSingleTouch(const MultiTouchInput& aEvent);

  



  void StartPanning(const MultiTouchInput& aStartPoint);

  



  void UpdateWithTouchAtDevicePoint(const MultiTouchInput& aEvent);

  


  void TrackTouch(const MultiTouchInput& aEvent);

  






  const nsIntRect CalculatePendingDisplayPort();

  







  bool EnlargeDisplayPortAlongAxis(float aViewport, float aVelocity,
                                   float* aDisplayPortOffset, float* aDisplayPortLength);

  







  void RequestContentRepaint();

  





  bool DoFling(const TimeDuration& aDelta);

  



  const FrameMetrics& GetFrameMetrics();

private:
  enum PanZoomState {
    NOTHING,        
    FLING,          
    TOUCHING,       
    PANNING,        
    PINCHING,       
    ANIMATING_ZOOM  
  };

  enum ContentPainterStatus {
    
    
    
    CONTENT_IDLE,
    
    
    
    CONTENT_PAINTING,
    
    
    
    
    
    
    
   CONTENT_PAINTING_AND_PAINT_PENDING
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

  AxisX mX;
  AxisY mY;

  
  
  
  
  Monitor mMonitor;

  
  
  TimeStamp mLastSampleTime;
  
  int32_t mLastEventTime;

  
  
  TimeStamp mAnimationStartTime;

  
  
  nsIntPoint mLastZoomFocus;

  
  
  PanZoomState mState;

  int mDPI;

  
  
  
  
  ContentPainterStatus mContentPainterStatus;

  
  
  bool mMayHaveTouchListeners;

  
  
  bool mDisableNextTouchBatch;

  friend class Axis;
};

}
}

#endif
