




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
#include "mozilla/layers/APZUtils.h"    

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
struct OverscrollHandoffState;
class LayerMetricsWrapper;
class InputQueue;
class GeckoContentController;
class HitTestingTreeNode;










































class APZCTreeManager {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(APZCTreeManager)

  typedef mozilla::layers::AllowedTouchBehavior AllowedTouchBehavior;

  
  
  
  
  struct TreeBuildingState;

public:
  APZCTreeManager();

  




















  void UpdateHitTestingTree(CompositorParent* aCompositor,
                            Layer* aRoot,
                            bool aIsFirstPaint,
                            uint64_t aOriginatingLayersId,
                            uint32_t aPaintSequenceNumber);

  

































  nsEventStatus ReceiveInputEvent(InputData& aEvent,
                                  ScrollableLayerGuid* aOutTargetGuid,
                                  uint64_t* aOutInputBlockId);

  















  nsEventStatus ReceiveInputEvent(WidgetInputEvent& aEvent,
                                  ScrollableLayerGuid* aOutTargetGuid,
                                  uint64_t* aOutInputBlockId);

  





  void TransformCoordinateToGecko(const ScreenIntPoint& aPoint,
                                  LayoutDeviceIntPoint* aOutTransformedPoint);

  




  void ZoomToRect(const ScrollableLayerGuid& aGuid,
                  const CSSRect& aRect);

  






  void ContentReceivedInputBlock(uint64_t aInputBlockId, bool aPreventDefault);

  










  void SetTargetAPZC(uint64_t aInputBlockId,
                     const nsTArray<ScrollableLayerGuid>& aTargets);

  



  void SetTargetAPZC(uint64_t aInputBlockId, const ScrollableLayerGuid& aTarget);

  


  void UpdateZoomConstraints(const ScrollableLayerGuid& aGuid,
                             const ZoomConstraints& aConstraints);

  




  void CancelAnimation(const ScrollableLayerGuid &aGuid);

  






  void ClearTree();

  


  bool HitTestAPZC(const ScreenIntPoint& aPoint);

  




  static const ScreenMargin CalculatePendingDisplayPort(
    const FrameMetrics& aFrameMetrics,
    const ParentLayerPoint& aVelocity,
    double aEstimatedPaintDuration);

  



  static void SetDPI(float aDpiValue) { sDPI = aDpiValue; }

  


  static float GetDPI() { return sDPI; }

  








  void SetAllowedTouchBehavior(uint64_t aInputBlockId,
                               const nsTArray<TouchBehaviorFlags>& aValues);

  













































  bool DispatchScroll(AsyncPanZoomController* aApzc,
                      ParentLayerPoint aStartPoint,
                      ParentLayerPoint aEndPoint,
                      OverscrollHandoffState& aOverscrollHandoffState);

  




















  bool DispatchFling(AsyncPanZoomController* aApzc,
                     ParentLayerPoint aVelocity,
                     nsRefPtr<const OverscrollHandoffChain> aOverscrollHandoffChain,
                     bool aHandoff);

  


  nsRefPtr<const OverscrollHandoffChain> BuildOverscrollHandoffChain(const nsRefPtr<AsyncPanZoomController>& aInitialTarget);

protected:
  
  virtual ~APZCTreeManager();

  
  virtual AsyncPanZoomController* MakeAPZCInstance(uint64_t aLayersId,
                                                   GeckoContentController* aController);

public:
  





  nsRefPtr<HitTestingTreeNode> GetRootNode() const;
  already_AddRefed<AsyncPanZoomController> GetTargetAPZC(const ScreenPoint& aPoint,
                                                         HitTestResult* aOutHitResult);
  gfx::Matrix4x4 GetScreenToApzcTransform(const AsyncPanZoomController *aApzc) const;
  gfx::Matrix4x4 GetApzcToGeckoTransform(const AsyncPanZoomController *aApzc) const;
private:
  typedef bool (*GuidComparator)(const ScrollableLayerGuid&, const ScrollableLayerGuid&);

  
  void AttachNodeToTree(HitTestingTreeNode* aNode,
                        HitTestingTreeNode* aParent,
                        HitTestingTreeNode* aNextSibling);
  already_AddRefed<AsyncPanZoomController> GetTargetAPZC(const ScrollableLayerGuid& aGuid);
  already_AddRefed<HitTestingTreeNode> GetTargetNode(const ScrollableLayerGuid& aGuid,
                                                     GuidComparator aComparator);
  HitTestingTreeNode* FindTargetNode(HitTestingTreeNode* aNode,
                                     const ScrollableLayerGuid& aGuid,
                                     GuidComparator aComparator);
  AsyncPanZoomController* GetAPZCAtPoint(HitTestingTreeNode* aNode,
                                         const ParentLayerPoint& aHitTestPoint,
                                         HitTestResult* aOutHitResult);
  already_AddRefed<AsyncPanZoomController> GetMultitouchTarget(AsyncPanZoomController* aApzc1, AsyncPanZoomController* aApzc2) const;
  already_AddRefed<AsyncPanZoomController> CommonAncestor(AsyncPanZoomController* aApzc1, AsyncPanZoomController* aApzc2) const;
  already_AddRefed<AsyncPanZoomController> RootAPZCForLayersId(AsyncPanZoomController* aApzc) const;
  already_AddRefed<AsyncPanZoomController> GetTouchInputBlockAPZC(const MultiTouchInput& aEvent,
                                                                  HitTestResult* aOutHitResult);
  nsEventStatus ProcessTouchInput(MultiTouchInput& aInput,
                                  ScrollableLayerGuid* aOutTargetGuid,
                                  uint64_t* aOutInputBlockId);
  nsEventStatus ProcessWheelEvent(WidgetWheelEvent& aEvent,
                                  ScrollableLayerGuid* aOutTargetGuid,
                                  uint64_t* aOutInputBlockId);
  nsEventStatus ProcessEvent(WidgetInputEvent& inputEvent,
                             ScrollableLayerGuid* aOutTargetGuid,
                             uint64_t* aOutInputBlockId);
  void UpdateWheelTransaction(WidgetInputEvent& aEvent);
  void UpdateZoomConstraintsRecursively(HitTestingTreeNode* aNode,
                                        const ZoomConstraints& aConstraints);
  void FlushRepaintsRecursively(HitTestingTreeNode* aNode);

  already_AddRefed<HitTestingTreeNode> RecycleOrCreateNode(TreeBuildingState& aState,
                                                           AsyncPanZoomController* aApzc);
  HitTestingTreeNode* PrepareNodeForLayer(const LayerMetricsWrapper& aLayer,
                                          const FrameMetrics& aMetrics,
                                          uint64_t aLayersId,
                                          const gfx::Matrix4x4& aAncestorTransform,
                                          HitTestingTreeNode* aParent,
                                          HitTestingTreeNode* aNextSibling,
                                          TreeBuildingState& aState);

  




















  HitTestingTreeNode* UpdateHitTestingTree(TreeBuildingState& aState,
                                           const LayerMetricsWrapper& aLayer,
                                           uint64_t aLayersId,
                                           const gfx::Matrix4x4& aAncestorTransform,
                                           HitTestingTreeNode* aParent,
                                           HitTestingTreeNode* aNextSibling);

  void PrintAPZCInfo(const LayerMetricsWrapper& aLayer,
                     const AsyncPanZoomController* apzc);

protected:
  


  nsRefPtr<InputQueue> mInputQueue;

private:
  






  mutable mozilla::Monitor mTreeLock;
  nsRefPtr<HitTestingTreeNode> mRootNode;
  




  nsRefPtr<AsyncPanZoomController> mApzcForInputBlock;
  


  HitTestResult mHitResultForInputBlock;
  



  int32_t mRetainedTouchIdentifier;
  
  uint32_t mTouchCount;
  

  gfx::TreeLog mApzcTreeLog;

  static float sDPI;
};

}
}

#endif 
