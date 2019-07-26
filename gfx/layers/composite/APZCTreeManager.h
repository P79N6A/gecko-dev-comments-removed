




#ifndef mozilla_layers_APZCTreeManager_h
#define mozilla_layers_APZCTreeManager_h

#include "mozilla/layers/AsyncPanZoomController.h"
#include "Layers.h"
#include "CompositorParent.h"

namespace mozilla {
namespace layers {

class AsyncPanZoomController;
class CompositorParent;









struct ScrollableLayerGuid {
  uint64_t mLayersId;
  uint32_t mPresShellId;
  FrameMetrics::ViewID mScrollId;

  ScrollableLayerGuid(uint64_t aLayersId, uint32_t aPresShellId,
                      FrameMetrics::ViewID aScrollId)
    : mLayersId(aLayersId)
    , mPresShellId(aPresShellId)
    , mScrollId(aScrollId)
  {
    MOZ_COUNT_CTOR(ScrollableLayerGuid);
  }

  ScrollableLayerGuid(uint64_t aLayersId, const FrameMetrics& aMetrics)
    : mLayersId(aLayersId)
    , mPresShellId(aMetrics.mPresShellId)
    , mScrollId(aMetrics.mScrollId)
  {
    MOZ_COUNT_CTOR(ScrollableLayerGuid);
  }

  ScrollableLayerGuid(uint64_t aLayersId)
    : mLayersId(aLayersId)
    , mPresShellId(0)
    , mScrollId(FrameMetrics::ROOT_SCROLL_ID)
  {
    MOZ_COUNT_CTOR(ScrollableLayerGuid);
    
    
  }

  ~ScrollableLayerGuid()
  {
    MOZ_COUNT_DTOR(ScrollableLayerGuid);
  }

  bool operator==(const ScrollableLayerGuid& other) const
  {
    return mLayersId == other.mLayersId
        && mPresShellId == other.mPresShellId
        && mScrollId == other.mScrollId;
  }

  bool operator!=(const ScrollableLayerGuid& other) const
  {
    return !(*this == other);
  }
};























class APZCTreeManager MOZ_FINAL {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(APZCTreeManager)

public:
  APZCTreeManager();

  
















  void UpdatePanZoomControllerTree(CompositorParent* aCompositor, Layer* aRoot,
                                   bool aIsFirstPaint, uint64_t aFirstPaintLayersId);

  





  nsEventStatus ReceiveInputEvent(const InputData& aEvent);

  









  nsEventStatus ReceiveInputEvent(const nsInputEvent& aEvent,
                                  nsInputEvent* aOutEvent);

  






  void UpdateCompositionBounds(const ScrollableLayerGuid& aGuid,
                               const ScreenIntRect& aCompositionBounds);

  







  void CancelDefaultPanZoom(const ScrollableLayerGuid& aGuid);

  



  void DetectScrollableSubframe(const ScrollableLayerGuid& aGuid);

  




  void ZoomToRect(const ScrollableLayerGuid& aGuid,
                  const CSSRect& aRect);

  





  void ContentReceivedTouch(const ScrollableLayerGuid& aGuid,
                            bool aPreventDefault);

  




  void UpdateZoomConstraints(const ScrollableLayerGuid& aGuid,
                             bool aAllowZoom,
                             float aMinScale,
                             float aMaxScale);

  




  void UpdateScrollOffset(const ScrollableLayerGuid& aGuid,
                          const CSSPoint& aScrollOffset);

  




  void CancelAnimation(const ScrollableLayerGuid &aGuid);

  






  void ClearTree();

private:
  




  already_AddRefed<AsyncPanZoomController> GetTargetAPZC(const ScrollableLayerGuid& aGuid);
  already_AddRefed<AsyncPanZoomController> GetTargetAPZC(const ScreenPoint& aPoint);
  
  AsyncPanZoomController* FindTargetAPZC(AsyncPanZoomController* aApzc, const ScrollableLayerGuid& aGuid);
  AsyncPanZoomController* GetAPZCAtPoint(AsyncPanZoomController* aApzc, gfxPoint aHitTestPoint);

  








  AsyncPanZoomController* UpdatePanZoomControllerTree(CompositorParent* aCompositor,
                                                      Layer* aLayer, uint64_t aLayersId,
                                                      AsyncPanZoomController* aParent,
                                                      AsyncPanZoomController* aNextSibling,
                                                      bool aIsFirstPaint,
                                                      uint64_t aFirstPaintLayersId,
                                                      nsTArray< nsRefPtr<AsyncPanZoomController> >* aApzcsToDestroy);

private:
  




  mozilla::Monitor mTreeLock;
  nsRefPtr<AsyncPanZoomController> mRootApzc;
};

}
}

#endif 
