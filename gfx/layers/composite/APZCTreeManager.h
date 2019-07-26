




#ifndef mozilla_layers_APZCTreeManager_h
#define mozilla_layers_APZCTreeManager_h

#include <stdint.h>                     
#include "FrameMetrics.h"               
#include "Units.h"                      
#include "gfxPoint.h"                   
#include "gfx3DMatrix.h"                
#include "mozilla/Assertions.h"         
#include "mozilla/EventForwards.h"      
#include "mozilla/Monitor.h"            
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsISupportsImpl.h"
#include "nsTraceRefcnt.h"              

class gfx3DMatrix;
template <class E> class nsTArray;

namespace mozilla {
class InputData;

namespace layers {

class Layer;
class AsyncPanZoomController;
class CompositorParent;























class APZCTreeManager {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(APZCTreeManager)

public:
  APZCTreeManager();
  virtual ~APZCTreeManager();

  
















  void UpdatePanZoomControllerTree(CompositorParent* aCompositor, Layer* aRoot,
                                   bool aIsFirstPaint, uint64_t aFirstPaintLayersId);

  








  nsEventStatus ReceiveInputEvent(const InputData& aEvent,
                                  ScrollableLayerGuid* aOutTargetGuid);

  
















  nsEventStatus ReceiveInputEvent(const WidgetInputEvent& aEvent,
                                  ScrollableLayerGuid* aOutTargetGuid,
                                  WidgetInputEvent* aOutEvent);

  







  nsEventStatus ReceiveInputEvent(WidgetInputEvent& aEvent,
                                  ScrollableLayerGuid* aOutTargetGuid);

  





  void TransformCoordinateToGecko(const ScreenIntPoint& aPoint,
                                  LayoutDeviceIntPoint* aOutTransformedPoint);

  







  void UpdateRootCompositionBounds(const uint64_t& aLayersId,
                                   const ScreenIntRect& aCompositionBounds);

  




  void ZoomToRect(const ScrollableLayerGuid& aGuid,
                  const CSSRect& aRect);

  





  void ContentReceivedTouch(const ScrollableLayerGuid& aGuid,
                            bool aPreventDefault);

  




  void UpdateZoomConstraints(const ScrollableLayerGuid& aGuid,
                             bool aAllowZoom,
                             const CSSToScreenScale& aMinScale,
                             const CSSToScreenScale& aMaxScale);

  




  void UpdateScrollOffset(const ScrollableLayerGuid& aGuid,
                          const CSSPoint& aScrollOffset);

  




  void CancelAnimation(const ScrollableLayerGuid &aGuid);

  






  void ClearTree();

  


  bool HitTestAPZC(const ScreenIntPoint& aPoint);

  



  static void SetDPI(float aDpiValue) { sDPI = aDpiValue; }

  


  static float GetDPI() { return sDPI; }

  









  void HandleOverscroll(AsyncPanZoomController* aAPZC, ScreenPoint aStartPoint, ScreenPoint aEndPoint);

protected:
  



  virtual void AssertOnCompositorThread();

public:
  





  already_AddRefed<AsyncPanZoomController> GetTargetAPZC(const ScrollableLayerGuid& aGuid);
  already_AddRefed<AsyncPanZoomController> GetTargetAPZC(const ScreenPoint& aPoint);
  already_AddRefed<AsyncPanZoomController> GetRootAPZCFor(const uint64_t& aLayersId);
  void GetInputTransforms(AsyncPanZoomController *aApzc, gfx3DMatrix& aTransformToApzcOut,
                          gfx3DMatrix& aTransformToGeckoOut);
private:
  
  AsyncPanZoomController* FindTargetAPZC(AsyncPanZoomController* aApzc, const ScrollableLayerGuid& aGuid);
  AsyncPanZoomController* GetAPZCAtPoint(AsyncPanZoomController* aApzc, const gfxPoint& aHitTestPoint);
  AsyncPanZoomController* FindRootAPZC(AsyncPanZoomController* aApzc, const uint64_t& aLayersId);
  already_AddRefed<AsyncPanZoomController> CommonAncestor(AsyncPanZoomController* aApzc1, AsyncPanZoomController* aApzc2);
  already_AddRefed<AsyncPanZoomController> RootAPZCForLayersId(AsyncPanZoomController* aApzc);
  already_AddRefed<AsyncPanZoomController> GetTouchInputBlockAPZC(const WidgetTouchEvent& aEvent, ScreenPoint aPoint);
  nsEventStatus ProcessTouchEvent(const WidgetTouchEvent& touchEvent, ScrollableLayerGuid* aOutTargetGuid, WidgetTouchEvent* aOutEvent);
  nsEventStatus ProcessMouseEvent(const WidgetMouseEvent& mouseEvent, ScrollableLayerGuid* aOutTargetGuid, WidgetMouseEvent* aOutEvent);
  nsEventStatus ProcessEvent(const WidgetInputEvent& inputEvent, ScrollableLayerGuid* aOutTargetGuid, WidgetInputEvent* aOutEvent);

  








  AsyncPanZoomController* UpdatePanZoomControllerTree(CompositorParent* aCompositor,
                                                      Layer* aLayer, uint64_t aLayersId,
                                                      gfx3DMatrix aTransform,
                                                      AsyncPanZoomController* aParent,
                                                      AsyncPanZoomController* aNextSibling,
                                                      bool aIsFirstPaint,
                                                      uint64_t aFirstPaintLayersId,
                                                      nsTArray< nsRefPtr<AsyncPanZoomController> >* aApzcsToDestroy);

private:
  




  mozilla::Monitor mTreeLock;
  nsRefPtr<AsyncPanZoomController> mRootApzc;
  




  nsRefPtr<AsyncPanZoomController> mApzcForInputBlock;
  
  uint32_t mTouchCount;
  






  gfx3DMatrix mCachedTransformToApzcForInputBlock;

  static float sDPI;
};

}
}

#endif 
