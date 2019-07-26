




#include "APZCTreeManager.h"
#include "AsyncCompositionManager.h"    
#include "LayerManagerComposite.h"      
#include "Compositor.h"

#define APZC_LOG(args...)


namespace mozilla {
namespace layers {

APZCTreeManager::APZCTreeManager()
    : mTreeLock("APZCTreeLock")
{
  MOZ_ASSERT(NS_IsMainThread());
  AsyncPanZoomController::InitializeGlobalState();
}

void
APZCTreeManager::AssertOnCompositorThread()
{
  Compositor::AssertOnCompositorThread();
}


static void
Collect(AsyncPanZoomController* aApzc, nsTArray< nsRefPtr<AsyncPanZoomController> >* aCollection)
{
  if (aApzc) {
    aCollection->AppendElement(aApzc);
    Collect(aApzc->GetLastChild(), aCollection);
    Collect(aApzc->GetPrevSibling(), aCollection);
  }
}

void
APZCTreeManager::UpdatePanZoomControllerTree(CompositorParent* aCompositor, Layer* aRoot,
                                             bool aIsFirstPaint, uint64_t aFirstPaintLayersId)
{
  AssertOnCompositorThread();

  MonitorAutoLock lock(mTreeLock);

  
  
  
  
  
  
  
  
  
  
  
  
  nsTArray< nsRefPtr<AsyncPanZoomController> > apzcsToDestroy;
  Collect(mRootApzc, &apzcsToDestroy);
  mRootApzc = nullptr;

  if (aRoot) {
    UpdatePanZoomControllerTree(aCompositor,
                                aRoot,
                                
                                aCompositor ? aCompositor->RootLayerTreeId() : 0,
                                nullptr, nullptr,
                                aIsFirstPaint, aFirstPaintLayersId,
                                &apzcsToDestroy);
  }

  for (int i = apzcsToDestroy.Length() - 1; i >= 0; i--) {
    APZC_LOG("Destroying APZC at %p\n", apzcsToDestroy[i].get());
    apzcsToDestroy[i]->Destroy();
  }
}

AsyncPanZoomController*
APZCTreeManager::UpdatePanZoomControllerTree(CompositorParent* aCompositor,
                                             Layer* aLayer, uint64_t aLayersId,
                                             AsyncPanZoomController* aParent,
                                             AsyncPanZoomController* aNextSibling,
                                             bool aIsFirstPaint, uint64_t aFirstPaintLayersId,
                                             nsTArray< nsRefPtr<AsyncPanZoomController> >* aApzcsToDestroy)
{
  ContainerLayer* container = aLayer->AsContainerLayer();
  AsyncPanZoomController* controller = nullptr;
  if (container) {
    if (container->GetFrameMetrics().IsScrollable()) {
      const CompositorParent::LayerTreeState* state = CompositorParent::GetIndirectShadowTree(aLayersId);
      if (state && state->mController.get()) {
        
        
        

        controller = container->GetAsyncPanZoomController();
        if (!controller) {
          controller = new AsyncPanZoomController(aLayersId, state->mController,
                                                  AsyncPanZoomController::USE_GESTURE_DETECTOR);
          controller->SetCompositorParent(aCompositor);
        } else {
          
          
          
          
          
          aApzcsToDestroy->RemoveElement(controller);
          controller->SetPrevSibling(nullptr);
          controller->SetLastChild(nullptr);
        }
        APZC_LOG("Using APZC %p for layer %p with identifiers %lld %lld\n", controller, aLayer, aLayersId, container->GetFrameMetrics().mScrollId);

        controller->NotifyLayersUpdated(container->GetFrameMetrics(),
                                        aIsFirstPaint && (aLayersId == aFirstPaintLayersId));

        gfx3DMatrix transform = container->GetEffectiveTransform();
        LayerRect visible = container->GetFrameMetrics().mViewport * container->GetFrameMetrics().LayersPixelsPerCSSPixel();
        gfxRect transformed = transform.TransformBounds(gfxRect(visible.x, visible.y, visible.width, visible.height));
        controller->SetVisibleRegion(transformed);
        APZC_LOG("Setting rect(%f %f %f %f) as visible region for %p\n", transformed.x, transformed.y,
                                                                         transformed.width, transformed.height,
                                                                         controller);

        
        if (aNextSibling) {
          aNextSibling->SetPrevSibling(controller);
        } else if (aParent) {
          aParent->SetLastChild(controller);
        } else {
          mRootApzc = controller;
        }

        
        aParent = controller;
      }
    }

    container->SetAsyncPanZoomController(controller);
  }

  uint64_t childLayersId = (aLayer->AsRefLayer() ? aLayer->AsRefLayer()->GetReferentId() : aLayersId);
  AsyncPanZoomController* next = nullptr;
  for (Layer* child = aLayer->GetLastChild(); child; child = child->GetPrevSibling()) {
    next = UpdatePanZoomControllerTree(aCompositor, child, childLayersId, aParent, next,
                                       aIsFirstPaint, aFirstPaintLayersId, aApzcsToDestroy);
  }

  
  
  
  
  if (controller) {
    return controller;
  }
  if (next) {
    return next;
  }
  return aNextSibling;
}

nsEventStatus
APZCTreeManager::ReceiveInputEvent(const InputData& aEvent)
{
  nsRefPtr<AsyncPanZoomController> apzc;
  switch (aEvent.mInputType) {
    case MULTITOUCH_INPUT: {
      const MultiTouchInput& multiTouchInput = aEvent.AsMultiTouchInput();
      apzc = GetTargetAPZC(ScreenPoint(multiTouchInput.mTouches[0].mScreenPoint));
      break;
    } case PINCHGESTURE_INPUT: {
      const PinchGestureInput& pinchInput = aEvent.AsPinchGestureInput();
      apzc = GetTargetAPZC(pinchInput.mFocusPoint);
      break;
    } case TAPGESTURE_INPUT: {
      const TapGestureInput& tapInput = aEvent.AsTapGestureInput();
      apzc = GetTargetAPZC(ScreenPoint(tapInput.mPoint));
      break;
    } default: {
      
      break;
    }
  }
  if (apzc) {
    return apzc->ReceiveInputEvent(aEvent);
  }
  return nsEventStatus_eIgnore;
}

nsEventStatus
APZCTreeManager::ReceiveInputEvent(const nsInputEvent& aEvent,
                                   nsInputEvent* aOutEvent)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsRefPtr<AsyncPanZoomController> apzc;
  switch (aEvent.eventStructType) {
    case NS_TOUCH_EVENT: {
      const nsTouchEvent& touchEvent = static_cast<const nsTouchEvent&>(aEvent);
      if (touchEvent.touches.Length() > 0) {
        nsIntPoint point = touchEvent.touches[0]->mRefPoint;
        apzc = GetTargetAPZC(ScreenPoint::FromUnknownPoint(gfx::Point(point.x, point.y)));
      }
      break;
    } case NS_MOUSE_EVENT: {
      const nsMouseEvent& mouseEvent = static_cast<const nsMouseEvent&>(aEvent);
      apzc = GetTargetAPZC(ScreenPoint::FromUnknownPoint(gfx::Point(mouseEvent.refPoint.x,
                                                                    mouseEvent.refPoint.y)));
      break;
    } default: {
      
      break;
    }
  }
  if (apzc) {
    return apzc->ReceiveInputEvent(aEvent, aOutEvent);
  }
  return nsEventStatus_eIgnore;
}

void
APZCTreeManager::UpdateCompositionBounds(const ScrollableLayerGuid& aGuid,
                                         const ScreenIntRect& aCompositionBounds)
{
  nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(aGuid);
  if (apzc) {
    apzc->UpdateCompositionBounds(aCompositionBounds);
  }
}

void
APZCTreeManager::CancelDefaultPanZoom(const ScrollableLayerGuid& aGuid)
{
  nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(aGuid);
  if (apzc) {
    apzc->CancelDefaultPanZoom();
  }
}

void
APZCTreeManager::DetectScrollableSubframe(const ScrollableLayerGuid& aGuid)
{
  nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(aGuid);
  if (apzc) {
    apzc->DetectScrollableSubframe();
  }
}

void
APZCTreeManager::ZoomToRect(const ScrollableLayerGuid& aGuid,
                            const CSSRect& aRect)
{
  nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(aGuid);
  if (apzc) {
    apzc->ZoomToRect(aRect);
  }
}

void
APZCTreeManager::ContentReceivedTouch(const ScrollableLayerGuid& aGuid,
                                      bool aPreventDefault)
{
  nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(aGuid);
  if (apzc) {
    apzc->ContentReceivedTouch(aPreventDefault);
  }
}

void
APZCTreeManager::UpdateZoomConstraints(const ScrollableLayerGuid& aGuid,
                                       bool aAllowZoom,
                                       float aMinScale,
                                       float aMaxScale)
{
  nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(aGuid);
  if (apzc) {
    apzc->UpdateZoomConstraints(aAllowZoom, aMinScale, aMaxScale);
  }
}

void
APZCTreeManager::UpdateScrollOffset(const ScrollableLayerGuid& aGuid,
                                    const CSSPoint& aScrollOffset)
{
  nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(aGuid);
  if (apzc) {
    apzc->UpdateScrollOffset(aScrollOffset);
  }
}

void
APZCTreeManager::CancelAnimation(const ScrollableLayerGuid &aGuid)
{
  nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(aGuid);
  if (apzc) {
    apzc->CancelAnimation();
  }
}

void
APZCTreeManager::ClearTree()
{
  MonitorAutoLock lock(mTreeLock);

  
  
  
  nsTArray< nsRefPtr<AsyncPanZoomController> > apzcsToDestroy;
  Collect(mRootApzc, &apzcsToDestroy);
  for (int i = apzcsToDestroy.Length() - 1; i >= 0; i--) {
    apzcsToDestroy[i]->Destroy();
  }
  mRootApzc = nullptr;
}

already_AddRefed<AsyncPanZoomController>
APZCTreeManager::GetTargetAPZC(const ScrollableLayerGuid& aGuid)
{
  MonitorAutoLock lock(mTreeLock);
  nsRefPtr<AsyncPanZoomController> target;
  
  for (AsyncPanZoomController* apzc = mRootApzc; apzc; apzc = apzc->GetPrevSibling()) {
    target = FindTargetAPZC(apzc, aGuid);
    if (target) {
      break;
    }
  }
  return target.forget();
}

already_AddRefed<AsyncPanZoomController>
APZCTreeManager::GetTargetAPZC(const ScreenPoint& aPoint)
{
  MonitorAutoLock lock(mTreeLock);
  nsRefPtr<AsyncPanZoomController> target;
  
  gfxPoint point(aPoint.x, aPoint.y);
  for (AsyncPanZoomController* apzc = mRootApzc; apzc; apzc = apzc->GetPrevSibling()) {
    target = GetAPZCAtPoint(apzc, point);
    if (target) {
      break;
    }
  }
  return target.forget();
}

AsyncPanZoomController*
APZCTreeManager::FindTargetAPZC(AsyncPanZoomController* aApzc, const ScrollableLayerGuid& aGuid) {
  
  
  for (AsyncPanZoomController* child = aApzc->GetLastChild(); child; child = child->GetPrevSibling()) {
    AsyncPanZoomController* match = FindTargetAPZC(child, aGuid);
    if (match) {
      return match;
    }
  }

  if (aApzc->Matches(aGuid)) {
    return aApzc;
  }
  return nullptr;
}

AsyncPanZoomController*
APZCTreeManager::GetAPZCAtPoint(AsyncPanZoomController* aApzc, gfxPoint aHitTestPoint)
{
  
  
  ViewTransform apzcTransform = aApzc->GetCurrentAsyncTransform();
  gfxPoint untransformed = gfx3DMatrix(apzcTransform).Inverse().ProjectPoint(aHitTestPoint);
  for (AsyncPanZoomController* child = aApzc->GetLastChild(); child; child = child->GetPrevSibling()) {
    AsyncPanZoomController* match = GetAPZCAtPoint(child, untransformed);
    if (match) {
      return match;
    }
  }
  if (aApzc->VisibleRegionContains(untransformed)) {
    return aApzc;
  }
  return nullptr;
}

}
}
