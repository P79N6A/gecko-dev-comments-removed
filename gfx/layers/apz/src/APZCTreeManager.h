




#ifndef mozilla_layers_APZCTreeManager_h
#define mozilla_layers_APZCTreeManager_h

#include <stdint.h>                     
#include <map>                          
#include "FrameMetrics.h"               
#include "Units.h"                      
#include "gfxPoint.h"                   
#include "mozilla/Assertions.h"         
#include "mozilla/EventForwards.h"      
#include "mozilla/Monitor.h"            
#include "mozilla/gfx/Matrix.h"         
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsISupportsImpl.h"            
#include "mozilla/Vector.h"             
#include "nsTArrayForwardDeclare.h"     
#include "mozilla/gfx/Logging.h"        

class nsIntRegion;

namespace mozilla {
class InputData;
class MultiTouchInput;

namespace layers {

enum AllowedTouchBehavior {
  NONE =               0,
  VERTICAL_PAN =       1 << 0,
  HORIZONTAL_PAN =     1 << 1,
  PINCH_ZOOM =         1 << 2,
  DOUBLE_TAP_ZOOM =    1 << 3,
  UNKNOWN =            1 << 4
};

class Layer;
class AsyncPanZoomController;
class CompositorParent;
class APZPaintLogHelper;
class OverscrollHandoffChain;
class LayerMetricsWrapper;







































class APZCTreeManager {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(APZCTreeManager)

  typedef mozilla::layers::AllowedTouchBehavior AllowedTouchBehavior;
  typedef uint32_t TouchBehaviorFlags;

  
  
  
  
  struct TreeBuildingState;

public:
  APZCTreeManager();

  




















  void UpdatePanZoomControllerTree(CompositorParent* aCompositor,
                                   Layer* aRoot,
                                   bool aIsFirstPaint,
                                   uint64_t aOriginatingLayersId,
                                   uint32_t aPaintSequenceNumber);

  
































  nsEventStatus ReceiveInputEvent(InputData& aEvent,
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

  













































  bool DispatchScroll(AsyncPanZoomController* aApzc,
                      ScreenPoint aStartPoint,
                      ScreenPoint aEndPoint,
                      const OverscrollHandoffChain& aOverscrollHandoffChain,
                      uint32_t aOverscrollHandoffChainIndex);

  




















  bool DispatchFling(AsyncPanZoomController* aApzc,
                     ScreenPoint aVelocity,
                     nsRefPtr<const OverscrollHandoffChain> aOverscrollHandoffChain,
                     bool aHandoff);

  void SnapBackOverscrolledApzc(AsyncPanZoomController* aStart);

  


  nsRefPtr<const OverscrollHandoffChain> BuildOverscrollHandoffChain(const nsRefPtr<AsyncPanZoomController>& aInitialTarget);
protected:
  
  virtual ~APZCTreeManager();

public:
  





  already_AddRefed<AsyncPanZoomController> GetTargetAPZC(const ScrollableLayerGuid& aGuid);
  already_AddRefed<AsyncPanZoomController> GetTargetAPZC(const ScreenPoint& aPoint,
                                                         bool* aOutInOverscrolledApzc);
  void GetInputTransforms(const AsyncPanZoomController *aApzc,
                          gfx::Matrix4x4& aTransformToApzcOut,
                          gfx::Matrix4x4& aTransformToGeckoOut) const;
private:
  
  AsyncPanZoomController* FindTargetAPZC(AsyncPanZoomController* aApzc, FrameMetrics::ViewID aScrollId);
  AsyncPanZoomController* FindTargetAPZC(AsyncPanZoomController* aApzc, const ScrollableLayerGuid& aGuid);
  AsyncPanZoomController* GetAPZCAtPoint(AsyncPanZoomController* aApzc,
                                         const gfx::Point& aHitTestPoint,
                                         bool* aOutInOverscrolledApzc);
  already_AddRefed<AsyncPanZoomController> CommonAncestor(AsyncPanZoomController* aApzc1, AsyncPanZoomController* aApzc2);
  already_AddRefed<AsyncPanZoomController> RootAPZCForLayersId(AsyncPanZoomController* aApzc);
  already_AddRefed<AsyncPanZoomController> GetTouchInputBlockAPZC(const MultiTouchInput& aEvent,
                                                                  bool* aOutInOverscrolledApzc);
  nsEventStatus ProcessTouchInput(MultiTouchInput& aInput,
                                  ScrollableLayerGuid* aOutTargetGuid);
  nsEventStatus ProcessEvent(WidgetInputEvent& inputEvent,
                             ScrollableLayerGuid* aOutTargetGuid);
  void UpdateZoomConstraintsRecursively(AsyncPanZoomController* aApzc,
                                        const ZoomConstraints& aConstraints);

  AsyncPanZoomController* PrepareAPZCForLayer(const LayerMetricsWrapper& aLayer,
                                              const FrameMetrics& aMetrics,
                                              uint64_t aLayersId,
                                              const gfx::Matrix4x4& aAncestorTransform,
                                              const nsIntRegion& aObscured,
                                              AsyncPanZoomController*& aOutParent,
                                              AsyncPanZoomController* aNextSibling,
                                              TreeBuildingState& aState);

  








  AsyncPanZoomController* UpdatePanZoomControllerTree(TreeBuildingState& aState,
                                                      const LayerMetricsWrapper& aLayer,
                                                      uint64_t aLayersId,
                                                      const gfx::Matrix4x4& aAncestorTransform,
                                                      AsyncPanZoomController* aParent,
                                                      AsyncPanZoomController* aNextSibling,
                                                      const nsIntRegion& aObscured);

  void PrintAPZCInfo(const LayerMetricsWrapper& aLayer,
                     const AsyncPanZoomController* apzc);
private:
  






  mutable mozilla::Monitor mTreeLock;
  nsRefPtr<AsyncPanZoomController> mRootApzc;
  




  nsRefPtr<AsyncPanZoomController> mApzcForInputBlock;
  


  bool mInOverscrolledApzc;
  



  int32_t mRetainedTouchIdentifier;
  
  uint32_t mTouchCount;
  






  gfx::Matrix4x4 mCachedTransformToApzcForInputBlock;
  

  gfx::TreeLog mApzcTreeLog;

  static float sDPI;
};

}
}

#endif 
