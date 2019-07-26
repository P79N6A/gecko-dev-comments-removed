




#include "APZCTreeManager.h"
#include "AsyncCompositionManager.h"    
#include "Compositor.h"                 
#include "CompositorParent.h"           
#include "InputData.h"                  
#include "Layers.h"                     
#include "gfx3DMatrix.h"                
#include "mozilla/dom/Touch.h"          
#include "mozilla/gfx/Point.h"          
#include "mozilla/layers/AsyncPanZoomController.h"
#include "mozilla/mozalloc.h"           
#include "nsGUIEvent.h"                 
#include "nsPoint.h"                    
#include "nsTArray.h"                   
#include "nsThreadUtils.h"              

#define APZC_LOG(...)


namespace mozilla {
namespace layers {

float APZCTreeManager::sDPI = 72.0;

APZCTreeManager::APZCTreeManager()
    : mTreeLock("APZCTreeLock")
{
  MOZ_ASSERT(NS_IsMainThread());
  AsyncPanZoomController::InitializeGlobalState();
}

APZCTreeManager::~APZCTreeManager()
{
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
                                gfx3DMatrix(), nullptr, nullptr,
                                aIsFirstPaint, aFirstPaintLayersId,
                                &apzcsToDestroy);
  }

  for (size_t i = 0; i < apzcsToDestroy.Length(); i++) {
    APZC_LOG("Destroying APZC at %p\n", apzcsToDestroy[i].get());
    apzcsToDestroy[i]->Destroy();
  }
}

AsyncPanZoomController*
APZCTreeManager::UpdatePanZoomControllerTree(CompositorParent* aCompositor,
                                             Layer* aLayer, uint64_t aLayersId,
                                             gfx3DMatrix aTransform,
                                             AsyncPanZoomController* aParent,
                                             AsyncPanZoomController* aNextSibling,
                                             bool aIsFirstPaint, uint64_t aFirstPaintLayersId,
                                             nsTArray< nsRefPtr<AsyncPanZoomController> >* aApzcsToDestroy)
{
  ContainerLayer* container = aLayer->AsContainerLayer();
  AsyncPanZoomController* apzc = nullptr;
  if (container) {
    if (container->GetFrameMetrics().IsScrollable()) {
      const CompositorParent::LayerTreeState* state = CompositorParent::GetIndirectShadowTree(aLayersId);
      if (state && state->mController.get()) {
        
        
        

        apzc = container->GetAsyncPanZoomController();

        bool newApzc = (apzc == nullptr);
        if (newApzc) {
          apzc = new AsyncPanZoomController(aLayersId, this, state->mController,
                                            AsyncPanZoomController::USE_GESTURE_DETECTOR);
          apzc->SetCompositorParent(aCompositor);
        } else {
          
          
          
          
          
          aApzcsToDestroy->RemoveElement(apzc);
          apzc->SetPrevSibling(nullptr);
          apzc->SetLastChild(nullptr);
        }
        APZC_LOG("Using APZC %p for layer %p with identifiers %lld %lld\n", apzc, aLayer, aLayersId, container->GetFrameMetrics().mScrollId);

        apzc->NotifyLayersUpdated(container->GetFrameMetrics(),
                                        aIsFirstPaint && (aLayersId == aFirstPaintLayersId));

        LayerRect visible = ScreenRect(container->GetFrameMetrics().mCompositionBounds) * ScreenToLayerScale(1.0);
        apzc->SetLayerHitTestData(visible, aTransform, aLayer->GetTransform());
        APZC_LOG("Setting rect(%f %f %f %f) as visible region for APZC %p\n", visible.x, visible.y,
                                                                              visible.width, visible.height,
                                                                              apzc);

        
        if (aNextSibling) {
          aNextSibling->SetPrevSibling(apzc);
        } else if (aParent) {
          aParent->SetLastChild(apzc);
        } else {
          mRootApzc = apzc;
        }

        
        aParent = apzc;

        if (newApzc && apzc->IsRootForLayersId()) {
          
          
          
          bool allowZoom;
          CSSToScreenScale minZoom, maxZoom;
          if (state->mController->GetZoomConstraints(&allowZoom, &minZoom, &maxZoom)) {
            apzc->UpdateZoomConstraints(allowZoom, minZoom, maxZoom);
          }
        }
      }
    }

    container->SetAsyncPanZoomController(apzc);
  }

  
  
  if (apzc) {
    aTransform = gfx3DMatrix();
  } else {
    
    aTransform = aLayer->GetTransform() * aTransform;
  }

  uint64_t childLayersId = (aLayer->AsRefLayer() ? aLayer->AsRefLayer()->GetReferentId() : aLayersId);
  AsyncPanZoomController* next = nullptr;
  for (Layer* child = aLayer->GetLastChild(); child; child = child->GetPrevSibling()) {
    next = UpdatePanZoomControllerTree(aCompositor, child, childLayersId, aTransform, aParent, next,
                                       aIsFirstPaint, aFirstPaintLayersId, aApzcsToDestroy);
  }

  
  
  
  
  if (apzc) {
    return apzc;
  }
  if (next) {
    return next;
  }
  return aNextSibling;
}

 template<class T> void
ApplyTransform(gfx::PointTyped<T>* aPoint, const gfx3DMatrix& aMatrix)
{
  gfxPoint result = aMatrix.Transform(gfxPoint(aPoint->x, aPoint->y));
  aPoint->x = result.x;
  aPoint->y = result.y;
}

 template<class T> void
ApplyTransform(gfx::IntPointTyped<T>* aPoint, const gfx3DMatrix& aMatrix)
{
  gfxPoint result = aMatrix.Transform(gfxPoint(aPoint->x, aPoint->y));
  aPoint->x = NS_lround(result.x);
  aPoint->y = NS_lround(result.y);
}

 void
ApplyTransform(nsIntPoint* aPoint, const gfx3DMatrix& aMatrix)
{
  gfxPoint result = aMatrix.Transform(gfxPoint(aPoint->x, aPoint->y));
  aPoint->x = NS_lround(result.x);
  aPoint->y = NS_lround(result.y);
}

nsEventStatus
APZCTreeManager::ReceiveInputEvent(const InputData& aEvent)
{
  nsEventStatus result = nsEventStatus_eIgnore;
  gfx3DMatrix transformToApzc;
  gfx3DMatrix transformToScreen;
  switch (aEvent.mInputType) {
    case MULTITOUCH_INPUT: {
      const MultiTouchInput& multiTouchInput = aEvent.AsMultiTouchInput();
      if (multiTouchInput.mType == MultiTouchInput::MULTITOUCH_START) {
        mApzcForInputBlock = GetTargetAPZC(ScreenPoint(multiTouchInput.mTouches[0].mScreenPoint));
        for (size_t i = 1; i < multiTouchInput.mTouches.Length(); i++) {
          nsRefPtr<AsyncPanZoomController> apzc2 = GetTargetAPZC(ScreenPoint(multiTouchInput.mTouches[i].mScreenPoint));
          mApzcForInputBlock = CommonAncestor(mApzcForInputBlock.get(), apzc2.get());
          APZC_LOG("Using APZC %p as the common ancestor\n", mApzcForInputBlock.get());
          
          
          mApzcForInputBlock = RootAPZCForLayersId(mApzcForInputBlock);
          APZC_LOG("Using APZC %p as the root APZC for multi-touch\n", mApzcForInputBlock.get());
        }

        
        if (mApzcForInputBlock) {
          GetInputTransforms(mApzcForInputBlock, transformToApzc, transformToScreen);
          mCachedTransformToApzcForInputBlock = transformToApzc;
        }
      } else if (mApzcForInputBlock) {
        APZC_LOG("Re-using APZC %p as continuation of event block\n", mApzcForInputBlock.get());
      }
      if (mApzcForInputBlock) {
        
        
        
        transformToApzc = mCachedTransformToApzcForInputBlock;
        MultiTouchInput inputForApzc(multiTouchInput);
        for (size_t i = 0; i < inputForApzc.mTouches.Length(); i++) {
          ApplyTransform(&(inputForApzc.mTouches[i].mScreenPoint), transformToApzc);
        }
        result = mApzcForInputBlock->ReceiveInputEvent(inputForApzc);
        
        
        if (multiTouchInput.mType == MultiTouchInput::MULTITOUCH_CANCEL ||
            (multiTouchInput.mType == MultiTouchInput::MULTITOUCH_END && multiTouchInput.mTouches.Length() == 1)) {
          mApzcForInputBlock = nullptr;
        }
      }
      break;
    } case PINCHGESTURE_INPUT: {
      const PinchGestureInput& pinchInput = aEvent.AsPinchGestureInput();
      nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(pinchInput.mFocusPoint);
      if (apzc) {
        GetInputTransforms(apzc, transformToApzc, transformToScreen);
        PinchGestureInput inputForApzc(pinchInput);
        ApplyTransform(&(inputForApzc.mFocusPoint), transformToApzc);
        result = apzc->ReceiveInputEvent(inputForApzc);
      }
      break;
    } case TAPGESTURE_INPUT: {
      const TapGestureInput& tapInput = aEvent.AsTapGestureInput();
      nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(ScreenPoint(tapInput.mPoint));
      if (apzc) {
        GetInputTransforms(apzc, transformToApzc, transformToScreen);
        TapGestureInput inputForApzc(tapInput);
        ApplyTransform(&(inputForApzc.mPoint), transformToApzc);
        result = apzc->ReceiveInputEvent(inputForApzc);
      }
      break;
    }
  }
  return result;
}

AsyncPanZoomController*
APZCTreeManager::GetTouchInputBlockAPZC(const nsTouchEvent& aEvent, ScreenPoint aPoint)
{
  nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(aPoint);
  gfx3DMatrix transformToApzc, transformToScreen;
  
  mCachedTransformToApzcForInputBlock = transformToApzc;
  if (!apzc) {
    return nullptr;
  }
  for (size_t i = 1; i < aEvent.touches.Length(); i++) {
    nsIntPoint point = aEvent.touches[i]->mRefPoint;
    nsRefPtr<AsyncPanZoomController> apzc2 =
      GetTargetAPZC(ScreenPoint(point.x, point.y));
    apzc = CommonAncestor(apzc.get(), apzc2.get());
    APZC_LOG("Using APZC %p as the common ancestor\n", apzc.get());
    
    
    apzc = RootAPZCForLayersId(apzc);
    APZC_LOG("Using APZC %p as the root APZC for multi-touch\n", apzc.get());
  }
  if (apzc) {
    
    GetInputTransforms(apzc, mCachedTransformToApzcForInputBlock, transformToScreen);
  }
  return apzc.get();
}

nsEventStatus
APZCTreeManager::ProcessTouchEvent(const nsTouchEvent& aEvent,
                                   nsTouchEvent* aOutEvent)
{
  
  
  
  gfx3DMatrix transformToApzc = mCachedTransformToApzcForInputBlock;
  MultiTouchInput inputForApzc(aEvent);
  for (size_t i = 0; i < inputForApzc.mTouches.Length(); i++) {
    ApplyTransform(&(inputForApzc.mTouches[i].mScreenPoint), transformToApzc);
  }
  nsEventStatus ret = mApzcForInputBlock->ReceiveInputEvent(inputForApzc);

  
  
  
  gfx3DMatrix transformToScreen;
  GetInputTransforms(mApzcForInputBlock, transformToApzc, transformToScreen);
  gfx3DMatrix outTransform = transformToApzc * transformToScreen;
  nsTouchEvent* outEvent = static_cast<nsTouchEvent*>(aOutEvent);
  for (size_t i = 0; i < outEvent->touches.Length(); i++) {
    ApplyTransform(&(outEvent->touches[i]->mRefPoint), outTransform);
  }

  
  
  if (aEvent.message == NS_TOUCH_CANCEL ||
      (aEvent.message == NS_TOUCH_END && aEvent.touches.Length() == 1)) {
    mApzcForInputBlock = nullptr;
  }
  return ret;
}

nsEventStatus
APZCTreeManager::ProcessMouseEvent(const nsMouseEvent& aEvent,
                                   nsMouseEvent* aOutEvent)
{
  nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(ScreenPoint(aEvent.refPoint.x, aEvent.refPoint.y));
  if (!apzc) {
    return nsEventStatus_eIgnore;
  }
  gfx3DMatrix transformToApzc;
  gfx3DMatrix transformToScreen;
  GetInputTransforms(apzc, transformToApzc, transformToScreen);
  MultiTouchInput inputForApzc(aEvent);
  ApplyTransform(&(inputForApzc.mTouches[0].mScreenPoint), transformToApzc);
  gfx3DMatrix outTransform = transformToApzc * transformToScreen;
  ApplyTransform(&(static_cast<nsMouseEvent*>(aOutEvent)->refPoint), outTransform);
  return apzc->ReceiveInputEvent(inputForApzc);
}

nsEventStatus
APZCTreeManager::ProcessEvent(const nsInputEvent& aEvent,
                              nsInputEvent* aOutEvent)
{
  
  nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(ScreenPoint(aEvent.refPoint.x, aEvent.refPoint.y));
  if (!apzc) {
    return nsEventStatus_eIgnore;
  }
  gfx3DMatrix transformToApzc;
  gfx3DMatrix transformToScreen;
  GetInputTransforms(apzc, transformToApzc, transformToScreen);
  ApplyTransform(&(aOutEvent->refPoint), transformToApzc);
  gfx3DMatrix outTransform = transformToApzc * transformToScreen;
  ApplyTransform(&(aOutEvent->refPoint), outTransform);
  return nsEventStatus_eIgnore;
}

nsEventStatus
APZCTreeManager::ReceiveInputEvent(const nsInputEvent& aEvent,
                                   nsInputEvent* aOutEvent)
{
  MOZ_ASSERT(NS_IsMainThread());

  switch (aEvent.eventStructType) {
    case NS_TOUCH_EVENT: {
      const nsTouchEvent& touchEvent = static_cast<const nsTouchEvent&>(aEvent);
      if (!touchEvent.touches.Length()) {
        return nsEventStatus_eIgnore;
      }
      if (touchEvent.message == NS_TOUCH_START) {
        ScreenPoint point = ScreenPoint(touchEvent.touches[0]->mRefPoint.x, touchEvent.touches[0]->mRefPoint.y);
        mApzcForInputBlock = GetTouchInputBlockAPZC(touchEvent, point);
      }
      if (!mApzcForInputBlock) {
        return nsEventStatus_eIgnore;
      }
      nsTouchEvent* outEvent = static_cast<nsTouchEvent*>(aOutEvent);
      return ProcessTouchEvent(touchEvent, outEvent);
    }
    case NS_MOUSE_EVENT: {
      
      const nsMouseEvent& mouseEvent = static_cast<const nsMouseEvent&>(aEvent);
      nsMouseEvent* outEvent = static_cast<nsMouseEvent*>(aOutEvent);
      return ProcessMouseEvent(mouseEvent, outEvent);
    }
    default: {
      return ProcessEvent(aEvent, aOutEvent);
    }
  }
}

nsEventStatus
APZCTreeManager::ReceiveInputEvent(nsInputEvent& aEvent)
{
  MOZ_ASSERT(NS_IsMainThread());

  switch (aEvent.eventStructType) {
    case NS_TOUCH_EVENT: {
      nsTouchEvent& touchEvent = static_cast<nsTouchEvent&>(aEvent);
      if (!touchEvent.touches.Length()) {
        return nsEventStatus_eIgnore;
      }
      if (touchEvent.message == NS_TOUCH_START) {
        ScreenPoint point = ScreenPoint(touchEvent.touches[0]->mRefPoint.x, touchEvent.touches[0]->mRefPoint.y);
        mApzcForInputBlock = GetTouchInputBlockAPZC(touchEvent, point);
      }
      if (!mApzcForInputBlock) {
        return nsEventStatus_eIgnore;
      }
      return ProcessTouchEvent(touchEvent, &touchEvent);
    }
    default: {
      return ProcessEvent(aEvent, &aEvent);
    }
  }
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
                                       const CSSToScreenScale& aMinScale,
                                       const CSSToScreenScale& aMaxScale)
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
  for (size_t i = 0; i < apzcsToDestroy.Length(); i++) {
    apzcsToDestroy[i]->Destroy();
  }
  mRootApzc = nullptr;
}

void
APZCTreeManager::HandleOverscroll(AsyncPanZoomController* aChild, ScreenPoint aStartPoint, ScreenPoint aEndPoint)
{
  AsyncPanZoomController* parent = aChild->GetParent();
  if (parent == nullptr)
    return;

  gfx3DMatrix transformToApzc;
  gfx3DMatrix transformToScreen;  

  
  GetInputTransforms(aChild, transformToApzc, transformToScreen);
  ApplyTransform(&aStartPoint, transformToApzc.Inverse());
  ApplyTransform(&aEndPoint, transformToApzc.Inverse());

  
  GetInputTransforms(parent, transformToApzc, transformToScreen);
  ApplyTransform(&aStartPoint, transformToApzc);
  ApplyTransform(&aEndPoint, transformToApzc);

  parent->AttemptScroll(aStartPoint, aEndPoint);
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
APZCTreeManager::GetAPZCAtPoint(AsyncPanZoomController* aApzc, const gfxPoint& aHitTestPoint)
{
  
  
  
  

  
  
  gfx3DMatrix ancestorUntransform = aApzc->GetAncestorTransform().Inverse();
  
  
  gfx3DMatrix asyncUntransform = gfx3DMatrix(aApzc->GetCurrentAsyncTransform()).Inverse();
  
  
  gfx3DMatrix untransformSinceLastApzc = ancestorUntransform * asyncUntransform * aApzc->GetCSSTransform().Inverse();
  
  
  gfxPoint untransformed = untransformSinceLastApzc.ProjectPoint(aHitTestPoint);
  APZC_LOG("Untransformed %f %f to %f %f for APZC %p\n", aHitTestPoint.x, aHitTestPoint.y, untransformed.x, untransformed.y, aApzc);

  
  
  for (AsyncPanZoomController* child = aApzc->GetLastChild(); child; child = child->GetPrevSibling()) {
    AsyncPanZoomController* match = GetAPZCAtPoint(child, untransformed);
    if (match) {
      return match;
    }
  }
  if (aApzc->VisibleRegionContains(LayerPoint(untransformed.x, untransformed.y))) {
    APZC_LOG("Successfully matched untransformed point %f %f to visible region for APZC %p\n", untransformed.x, untransformed.y, aApzc);
    return aApzc;
  }
  return nullptr;
}






























































void
APZCTreeManager::GetInputTransforms(AsyncPanZoomController *aApzc, gfx3DMatrix& aTransformToApzcOut,
                                    gfx3DMatrix& aTransformToScreenOut)
{
  
  
  
  
  

  
  gfx3DMatrix ancestorUntransform = aApzc->GetAncestorTransform().Inverse();
  
  gfx3DMatrix asyncUntransform = gfx3DMatrix(aApzc->GetCurrentAsyncTransform()).Inverse();

  
  aTransformToApzcOut = ancestorUntransform;
  
  aTransformToScreenOut = asyncUntransform * aApzc->GetAncestorTransform();

  for (AsyncPanZoomController* parent = aApzc->GetParent(); parent; parent = parent->GetParent()) {
    
    ancestorUntransform = parent->GetAncestorTransform().Inverse();
    
    asyncUntransform = gfx3DMatrix(parent->GetCurrentAsyncTransform()).Inverse();
    
    gfx3DMatrix untransformSinceLastApzc = ancestorUntransform * asyncUntransform * parent->GetCSSTransform().Inverse();

    
    aTransformToApzcOut = untransformSinceLastApzc * aTransformToApzcOut;
    
    aTransformToScreenOut = aTransformToScreenOut * parent->GetCSSTransform() * parent->GetAncestorTransform();

    
    
    
  }
}

AsyncPanZoomController*
APZCTreeManager::CommonAncestor(AsyncPanZoomController* aApzc1, AsyncPanZoomController* aApzc2)
{
  
  

  
  int depth1 = 0, depth2 = 0;
  for (AsyncPanZoomController* parent = aApzc1; parent; parent = parent->GetParent()) {
    depth1++;
  }
  for (AsyncPanZoomController* parent = aApzc2; parent; parent = parent->GetParent()) {
    depth2++;
  }

  
  
  int minDepth = depth1 < depth2 ? depth1 : depth2;
  while (depth1 > minDepth) {
    depth1--;
    aApzc1 = aApzc1->GetParent();
  }
  while (depth2 > minDepth) {
    depth2--;
    aApzc2 = aApzc2->GetParent();
  }

  
  
  while (true) {
    if (aApzc1 == aApzc2) {
      return aApzc1;
    }
    if (depth1 <= 0) {
      break;
    }
    aApzc1 = aApzc1->GetParent();
    aApzc2 = aApzc2->GetParent();
  }
  return nullptr;
}

AsyncPanZoomController*
APZCTreeManager::RootAPZCForLayersId(AsyncPanZoomController* aApzc)
{
  while (aApzc && !aApzc->IsRootForLayersId()) {
    aApzc = aApzc->GetParent();
  }
  return aApzc;
}

}
}
