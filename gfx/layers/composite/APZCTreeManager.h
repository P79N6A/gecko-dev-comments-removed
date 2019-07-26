




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























class APZCTreeManager {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(APZCTreeManager)

public:
  APZCTreeManager();
  virtual ~APZCTreeManager();

  
















  void UpdatePanZoomControllerTree(CompositorParent* aCompositor, Layer* aRoot,
                                   bool aIsFirstPaint, uint64_t aFirstPaintLayersId);

  




  nsEventStatus ReceiveInputEvent(const InputData& aEvent);

  














  nsEventStatus ReceiveInputEvent(const WidgetInputEvent& aEvent,
                                  WidgetInputEvent* aOutEvent);

  





  nsEventStatus ReceiveInputEvent(WidgetInputEvent& aEvent);

  






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
                             const CSSToScreenScale& aMinScale,
                             const CSSToScreenScale& aMaxScale);

  




  void UpdateScrollOffset(const ScrollableLayerGuid& aGuid,
                          const CSSPoint& aScrollOffset);

  




  void CancelAnimation(const ScrollableLayerGuid &aGuid);

  






  void ClearTree();

  


  bool HitTestAPZC(const ScreenPoint& aPoint);

  



  static void SetDPI(float aDpiValue) { sDPI = aDpiValue; }

  


  static float GetDPI() { return sDPI; }

  









  void HandleOverscroll(AsyncPanZoomController* aAPZC, ScreenPoint aStartPoint, ScreenPoint aEndPoint);

protected:
  



  virtual void AssertOnCompositorThread();

public:
  





  already_AddRefed<AsyncPanZoomController> GetTargetAPZC(const ScrollableLayerGuid& aGuid);
  already_AddRefed<AsyncPanZoomController> GetTargetAPZC(const ScreenPoint& aPoint);
  void GetInputTransforms(AsyncPanZoomController *aApzc, gfx3DMatrix& aTransformToApzcOut,
                          gfx3DMatrix& aTransformToScreenOut);
private:
  
  AsyncPanZoomController* FindTargetAPZC(AsyncPanZoomController* aApzc, const ScrollableLayerGuid& aGuid);
  AsyncPanZoomController* GetAPZCAtPoint(AsyncPanZoomController* aApzc, const gfxPoint& aHitTestPoint);
  AsyncPanZoomController* CommonAncestor(AsyncPanZoomController* aApzc1, AsyncPanZoomController* aApzc2);
  AsyncPanZoomController* RootAPZCForLayersId(AsyncPanZoomController* aApzc);
  AsyncPanZoomController* GetTouchInputBlockAPZC(const WidgetTouchEvent& aEvent, ScreenPoint aPoint);
  nsEventStatus ProcessTouchEvent(const WidgetTouchEvent& touchEvent, WidgetTouchEvent* aOutEvent);
  nsEventStatus ProcessMouseEvent(const nsMouseEvent& mouseEvent, nsMouseEvent* aOutEvent);
  nsEventStatus ProcessEvent(const WidgetInputEvent& inputEvent, WidgetInputEvent* aOutEvent);

  








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
  






  gfx3DMatrix mCachedTransformToApzcForInputBlock;

  static float sDPI;
};

}
}

#endif 
