




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
#include "mozilla/Vector.h"             
#include "nsTArrayForwardDeclare.h"     
#include "mozilla/gfx/Logging.h"        

class gfx3DMatrix;

namespace mozilla {
class InputData;

namespace layers {

enum AllowedTouchBehavior {
  NONE =               0,
  VERTICAL_PAN =       1 << 0,
  HORIZONTAL_PAN =     1 << 1,
  ZOOM =               1 << 2,
  UNKNOWN =            1 << 3
};

class Layer;
class AsyncPanZoomController;
class CompositorParent;







































class APZCTreeManager {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(APZCTreeManager)

  typedef mozilla::layers::AllowedTouchBehavior AllowedTouchBehavior;
  typedef uint32_t TouchBehaviorFlags;

public:
  APZCTreeManager();
  virtual ~APZCTreeManager();

  
















  void UpdatePanZoomControllerTree(CompositorParent* aCompositor, Layer* aRoot,
                                   bool aIsFirstPaint, uint64_t aFirstPaintLayersId);

  








  nsEventStatus ReceiveInputEvent(const InputData& aEvent,
                                  ScrollableLayerGuid* aOutTargetGuid);

  















  nsEventStatus ReceiveInputEvent(WidgetInputEvent& aEvent,
                                  ScrollableLayerGuid* aOutTargetGuid);

  





  void TransformCoordinateToGecko(const ScreenIntPoint& aPoint,
                                  LayoutDeviceIntPoint* aOutTransformedPoint);

  




  void ZoomToRect(const ScrollableLayerGuid& aGuid,
                  const CSSRect& aRect);

  





  void ContentReceivedTouch(const ScrollableLayerGuid& aGuid,
                            bool aPreventDefault);

  


  void UpdateZoomConstraints(const ScrollableLayerGuid& aGuid,
                             const ZoomConstraints& aConstraints);

  




  void CancelAnimation(const ScrollableLayerGuid &aGuid);

  






  void ClearTree();

  


  bool HitTestAPZC(const ScreenIntPoint& aPoint);

  



  static void SetDPI(float aDpiValue) { sDPI = aDpiValue; }

  


  static float GetDPI() { return sDPI; }

  




  void GetAllowedTouchBehavior(WidgetInputEvent* aEvent,
                               nsTArray<TouchBehaviorFlags>& aOutValues);

  




  void SetAllowedTouchBehavior(const ScrollableLayerGuid& aGuid,
                               const nsTArray<TouchBehaviorFlags>& aValues);

  






































  void DispatchScroll(AsyncPanZoomController* aAPZC, ScreenPoint aStartPoint, ScreenPoint aEndPoint,
                      uint32_t aOverscrollHandoffChainIndex);

  






  void HandOffFling(AsyncPanZoomController* aApzc, ScreenPoint aVelocity);

  bool FlushRepaintsForOverscrollHandoffChain();

protected:
  



  virtual void AssertOnCompositorThread();

  


  void BuildOverscrollHandoffChain(const nsRefPtr<AsyncPanZoomController>& aInitialTarget);
public:
  





  already_AddRefed<AsyncPanZoomController> GetTargetAPZC(const ScrollableLayerGuid& aGuid);
  already_AddRefed<AsyncPanZoomController> GetTargetAPZC(const ScreenPoint& aPoint);
  void GetInputTransforms(AsyncPanZoomController *aApzc, gfx3DMatrix& aTransformToApzcOut,
                          gfx3DMatrix& aTransformToGeckoOut);
private:
  
  AsyncPanZoomController* FindTargetAPZC(AsyncPanZoomController* aApzc, const ScrollableLayerGuid& aGuid);
  AsyncPanZoomController* GetAPZCAtPoint(AsyncPanZoomController* aApzc, const gfxPoint& aHitTestPoint);
  already_AddRefed<AsyncPanZoomController> CommonAncestor(AsyncPanZoomController* aApzc1, AsyncPanZoomController* aApzc2);
  already_AddRefed<AsyncPanZoomController> RootAPZCForLayersId(AsyncPanZoomController* aApzc);
  already_AddRefed<AsyncPanZoomController> GetTouchInputBlockAPZC(const WidgetTouchEvent& aEvent);
  nsEventStatus ProcessTouchEvent(WidgetTouchEvent& touchEvent, ScrollableLayerGuid* aOutTargetGuid);
  nsEventStatus ProcessMouseEvent(WidgetMouseEvent& mouseEvent, ScrollableLayerGuid* aOutTargetGuid);
  nsEventStatus ProcessEvent(WidgetInputEvent& inputEvent, ScrollableLayerGuid* aOutTargetGuid);
  void UpdateZoomConstraintsRecursively(AsyncPanZoomController* aApzc,
                                        const ZoomConstraints& aConstraints);
  void ClearOverscrollHandoffChain();

  








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
  




  Vector< nsRefPtr<AsyncPanZoomController> > mOverscrollHandoffChain;
  

  gfx::TreeLog mApzcTreeLog;

  static float sDPI;
};

}
}

#endif 
