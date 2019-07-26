




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
#include "mozilla/MouseEvents.h"
#include "mozilla/mozalloc.h"           
#include "mozilla/TouchEvents.h"
#include "nsDebug.h"                    
#include "nsPoint.h"                    
#include "nsTArray.h"                   
#include "nsThreadUtils.h"              

#include <algorithm>                    

#define APZC_LOG(...)


namespace mozilla {
namespace layers {

float APZCTreeManager::sDPI = 72.0;

APZCTreeManager::APZCTreeManager()
    : mTreeLock("APZCTreeLock"),
      mTouchCount(0)
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
  mTreeLock.AssertCurrentThreadOwns();

  ContainerLayer* container = aLayer->AsContainerLayer();
  AsyncPanZoomController* apzc = nullptr;
  if (container) {
    if (container->GetFrameMetrics().IsScrollable()) {
      const CompositorParent::LayerTreeState* state = CompositorParent::GetIndirectShadowTree(aLayersId);
      if (state && state->mController.get()) {
        
        
        

        apzc = container->GetAsyncPanZoomController();

        
        
        
        
        if (apzc && !apzc->Matches(ScrollableLayerGuid(aLayersId, container->GetFrameMetrics()))) {
          apzc = nullptr;
        }

        
        
        
        
        
        
        if (apzc == nullptr) {
          ScrollableLayerGuid target(aLayersId, container->GetFrameMetrics());
          for (size_t i = 0; i < aApzcsToDestroy->Length(); i++) {
            if (aApzcsToDestroy->ElementAt(i)->Matches(target)) {
              apzc = aApzcsToDestroy->ElementAt(i);
              break;
            }
          }
        }

        
        
        
        
        bool newApzc = (apzc == nullptr || apzc->IsDestroyed());
        if (newApzc) {
          apzc = new AsyncPanZoomController(aLayersId, this, state->mController,
                                            AsyncPanZoomController::USE_GESTURE_DETECTOR);
          apzc->SetCompositorParent(aCompositor);
          apzc->SetCrossProcessCompositorParent(state->mCrossProcessParent);
        } else {
          
          
          
          
          
          aApzcsToDestroy->RemoveElement(apzc);
          apzc->SetPrevSibling(nullptr);
          apzc->SetLastChild(nullptr);
        }
        APZC_LOG("Using APZC %p for layer %p with identifiers %lld %lld\n", apzc, aLayer, aLayersId, container->GetFrameMetrics().mScrollId);

        apzc->NotifyLayersUpdated(container->GetFrameMetrics(),
                                  aIsFirstPaint && (aLayersId == aFirstPaintLayersId));

        ScreenRect visible(container->GetFrameMetrics().mCompositionBounds);
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

        if (newApzc) {
          bool allowZoom;
          CSSToScreenScale minZoom, maxZoom;
          if (apzc->IsRootForLayersId()) {
            
            
            
            if (state->mController->GetRootZoomConstraints(&allowZoom, &minZoom, &maxZoom)) {
              apzc->UpdateZoomConstraints(allowZoom, minZoom, maxZoom);
            }
          } else {
            
            
            
            
            apzc->GetParent()->GetZoomConstraints(&allowZoom, &minZoom, &maxZoom);
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
  
  
  AsyncPanZoomController* next = apzc ? nullptr : aNextSibling;
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
APZCTreeManager::ReceiveInputEvent(const InputData& aEvent,
                                   ScrollableLayerGuid* aOutTargetGuid)
{
  nsEventStatus result = nsEventStatus_eIgnore;
  gfx3DMatrix transformToApzc;
  gfx3DMatrix transformToGecko;
  switch (aEvent.mInputType) {
    case MULTITOUCH_INPUT: {
      const MultiTouchInput& multiTouchInput = aEvent.AsMultiTouchInput();
      if (multiTouchInput.mType == MultiTouchInput::MULTITOUCH_START) {
        mTouchCount++;
        mApzcForInputBlock = GetTargetAPZC(ScreenPoint(multiTouchInput.mTouches[0].mScreenPoint));
        if (multiTouchInput.mTouches.Length() == 1) {
          
          
          BuildOverscrollHandoffChain(mApzcForInputBlock);
        }
        for (size_t i = 1; i < multiTouchInput.mTouches.Length(); i++) {
          nsRefPtr<AsyncPanZoomController> apzc2 = GetTargetAPZC(ScreenPoint(multiTouchInput.mTouches[i].mScreenPoint));
          mApzcForInputBlock = CommonAncestor(mApzcForInputBlock.get(), apzc2.get());
          APZC_LOG("Using APZC %p as the common ancestor\n", mApzcForInputBlock.get());
          
          
          mApzcForInputBlock = RootAPZCForLayersId(mApzcForInputBlock);
          APZC_LOG("Using APZC %p as the root APZC for multi-touch\n", mApzcForInputBlock.get());
        }

        
        if (mApzcForInputBlock) {
          GetInputTransforms(mApzcForInputBlock, transformToApzc, transformToGecko);
          mCachedTransformToApzcForInputBlock = transformToApzc;
        }
      } else if (mApzcForInputBlock) {
        APZC_LOG("Re-using APZC %p as continuation of event block\n", mApzcForInputBlock.get());
      }
      if (mApzcForInputBlock) {
        mApzcForInputBlock->GetGuid(aOutTargetGuid);
        
        
        
        transformToApzc = mCachedTransformToApzcForInputBlock;
        MultiTouchInput inputForApzc(multiTouchInput);
        for (size_t i = 0; i < inputForApzc.mTouches.Length(); i++) {
          ApplyTransform(&(inputForApzc.mTouches[i].mScreenPoint), transformToApzc);
        }
        result = mApzcForInputBlock->ReceiveInputEvent(inputForApzc);
      }
      if (multiTouchInput.mType == MultiTouchInput::MULTITOUCH_CANCEL ||
          multiTouchInput.mType == MultiTouchInput::MULTITOUCH_END) {
        if (mTouchCount >= multiTouchInput.mTouches.Length()) {
          mTouchCount -= multiTouchInput.mTouches.Length();
        } else {
          NS_WARNING("Got an unexpected touchend/touchcancel");
          mTouchCount = 0;
        }
        
        
        if (mTouchCount == 0) {
          mApzcForInputBlock = nullptr;
          mOverscrollHandoffChain.clear();
        }
      }
      break;
    } case PINCHGESTURE_INPUT: {
      const PinchGestureInput& pinchInput = aEvent.AsPinchGestureInput();
      nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(pinchInput.mFocusPoint);
      if (apzc) {
        apzc->GetGuid(aOutTargetGuid);
        GetInputTransforms(apzc, transformToApzc, transformToGecko);
        PinchGestureInput inputForApzc(pinchInput);
        ApplyTransform(&(inputForApzc.mFocusPoint), transformToApzc);
        result = apzc->ReceiveInputEvent(inputForApzc);
      }
      break;
    } case TAPGESTURE_INPUT: {
      const TapGestureInput& tapInput = aEvent.AsTapGestureInput();
      nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(ScreenPoint(tapInput.mPoint));
      if (apzc) {
        apzc->GetGuid(aOutTargetGuid);
        GetInputTransforms(apzc, transformToApzc, transformToGecko);
        TapGestureInput inputForApzc(tapInput);
        ApplyTransform(&(inputForApzc.mPoint), transformToApzc);
        result = apzc->ReceiveInputEvent(inputForApzc);
      }
      break;
    }
  }
  return result;
}

already_AddRefed<AsyncPanZoomController>
APZCTreeManager::GetTouchInputBlockAPZC(const WidgetTouchEvent& aEvent,
                                        ScreenPoint aPoint)
{
  nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(aPoint);
  if (aEvent.touches.Length() == 1) {
    
    
    BuildOverscrollHandoffChain(apzc);
  }
  gfx3DMatrix transformToApzc, transformToGecko;
  
  mCachedTransformToApzcForInputBlock = transformToApzc;
  if (!apzc) {
    return apzc.forget();
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
    
    GetInputTransforms(apzc, mCachedTransformToApzcForInputBlock, transformToGecko);
  }
  return apzc.forget();
}

nsEventStatus
APZCTreeManager::ProcessTouchEvent(const WidgetTouchEvent& aEvent,
                                   ScrollableLayerGuid* aOutTargetGuid,
                                   WidgetTouchEvent* aOutEvent)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsEventStatus ret = nsEventStatus_eIgnore;
  if (!aEvent.touches.Length()) {
    return ret;
  }
  if (aEvent.message == NS_TOUCH_START) {
    mTouchCount++;
    ScreenPoint point = ScreenPoint(aEvent.touches[0]->mRefPoint.x, aEvent.touches[0]->mRefPoint.y);
    mApzcForInputBlock = GetTouchInputBlockAPZC(aEvent, point);
  }

  if (mApzcForInputBlock) {
    mApzcForInputBlock->GetGuid(aOutTargetGuid);
    
    
    
    gfx3DMatrix transformToApzc = mCachedTransformToApzcForInputBlock;
    MultiTouchInput inputForApzc(aEvent);
    for (size_t i = 0; i < inputForApzc.mTouches.Length(); i++) {
      ApplyTransform(&(inputForApzc.mTouches[i].mScreenPoint), transformToApzc);
    }
    ret = mApzcForInputBlock->ReceiveInputEvent(inputForApzc);

    
    
    
    gfx3DMatrix transformToGecko;
    GetInputTransforms(mApzcForInputBlock, transformToApzc, transformToGecko);
    gfx3DMatrix outTransform = transformToApzc * transformToGecko;
    for (size_t i = 0; i < aOutEvent->touches.Length(); i++) {
      ApplyTransform(&(aOutEvent->touches[i]->mRefPoint), outTransform);
    }
  }
  
  
  if (aEvent.message == NS_TOUCH_CANCEL ||
      aEvent.message == NS_TOUCH_END) {
    if (mTouchCount >= aEvent.touches.Length()) {
      mTouchCount -= aEvent.touches.Length();
    } else {
      NS_WARNING("Got an unexpected touchend/touchcancel");
      mTouchCount = 0;
    }
    if (mTouchCount == 0) {
      mApzcForInputBlock = nullptr;
      mOverscrollHandoffChain.clear();
    }
  }
  return ret;
}

void
APZCTreeManager::TransformCoordinateToGecko(const ScreenIntPoint& aPoint,
                                            LayoutDeviceIntPoint* aOutTransformedPoint)
{
  MOZ_ASSERT(aOutTransformedPoint);
  nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(aPoint);
  if (apzc && aOutTransformedPoint) {
    gfx3DMatrix transformToApzc;
    gfx3DMatrix transformToGecko;
    GetInputTransforms(apzc, transformToApzc, transformToGecko);
    gfx3DMatrix outTransform = transformToApzc * transformToGecko;
    aOutTransformedPoint->x = aPoint.x;
    aOutTransformedPoint->y = aPoint.y;
    ApplyTransform(aOutTransformedPoint, outTransform);
  }
}


nsEventStatus
APZCTreeManager::ProcessMouseEvent(const WidgetMouseEvent& aEvent,
                                   ScrollableLayerGuid* aOutTargetGuid,
                                   WidgetMouseEvent* aOutEvent)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(ScreenPoint(aEvent.refPoint.x, aEvent.refPoint.y));
  if (!apzc) {
    return nsEventStatus_eIgnore;
  }
  apzc->GetGuid(aOutTargetGuid);
  gfx3DMatrix transformToApzc;
  gfx3DMatrix transformToGecko;
  GetInputTransforms(apzc, transformToApzc, transformToGecko);
  MultiTouchInput inputForApzc(aEvent);
  ApplyTransform(&(inputForApzc.mTouches[0].mScreenPoint), transformToApzc);
  gfx3DMatrix outTransform = transformToApzc * transformToGecko;
  ApplyTransform(&aOutEvent->refPoint, outTransform);
  return apzc->ReceiveInputEvent(inputForApzc);
}

nsEventStatus
APZCTreeManager::ProcessEvent(const WidgetInputEvent& aEvent,
                              ScrollableLayerGuid* aOutTargetGuid,
                              WidgetInputEvent* aOutEvent)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(ScreenPoint(aEvent.refPoint.x, aEvent.refPoint.y));
  if (!apzc) {
    return nsEventStatus_eIgnore;
  }
  apzc->GetGuid(aOutTargetGuid);
  gfx3DMatrix transformToApzc;
  gfx3DMatrix transformToGecko;
  GetInputTransforms(apzc, transformToApzc, transformToGecko);
  gfx3DMatrix outTransform = transformToApzc * transformToGecko;
  ApplyTransform(&(aOutEvent->refPoint), outTransform);
  return nsEventStatus_eIgnore;
}

nsEventStatus
APZCTreeManager::ReceiveInputEvent(const WidgetInputEvent& aEvent,
                                   ScrollableLayerGuid* aOutTargetGuid,
                                   WidgetInputEvent* aOutEvent)
{
  MOZ_ASSERT(NS_IsMainThread());

  switch (aEvent.eventStructType) {
    case NS_TOUCH_EVENT: {
      const WidgetTouchEvent& touchEvent = *aEvent.AsTouchEvent();
      return ProcessTouchEvent(touchEvent, aOutTargetGuid, aOutEvent->AsTouchEvent());
    }
    case NS_MOUSE_EVENT: {
      
      const WidgetMouseEvent& mouseEvent = *aEvent.AsMouseEvent();
      WidgetMouseEvent* outMouseEvent = aOutEvent->AsMouseEvent();
      return ProcessMouseEvent(mouseEvent, aOutTargetGuid, outMouseEvent);
    }
    default: {
      return ProcessEvent(aEvent, aOutTargetGuid, aOutEvent);
    }
  }
}

nsEventStatus
APZCTreeManager::ReceiveInputEvent(WidgetInputEvent& aEvent,
                                   ScrollableLayerGuid* aOutTargetGuid)
{
  MOZ_ASSERT(NS_IsMainThread());

  switch (aEvent.eventStructType) {
    case NS_TOUCH_EVENT: {
      WidgetTouchEvent& touchEvent = *aEvent.AsTouchEvent();
      return ProcessTouchEvent(touchEvent, aOutTargetGuid, &touchEvent);
    }
    default: {
      return ProcessEvent(aEvent, aOutTargetGuid, &aEvent);
    }
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
  
  
  if (apzc && apzc->IsRootForLayersId()) {
    MonitorAutoLock lock(mTreeLock);
    UpdateZoomConstraintsRecursively(apzc.get(), aAllowZoom, aMinScale, aMaxScale);
  }
}

void
APZCTreeManager::UpdateZoomConstraintsRecursively(AsyncPanZoomController* aApzc,
                                                  bool aAllowZoom,
                                                  const CSSToScreenScale& aMinScale,
                                                  const CSSToScreenScale& aMaxScale)
{
  mTreeLock.AssertCurrentThreadOwns();

  aApzc->UpdateZoomConstraints(aAllowZoom, aMinScale, aMaxScale);
  for (AsyncPanZoomController* child = aApzc->GetLastChild(); child; child = child->GetPrevSibling()) {
    
    if (!child->IsRootForLayersId()) {
      UpdateZoomConstraintsRecursively(child, aAllowZoom, aMinScale, aMaxScale);
    }
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
APZCTreeManager::DispatchScroll(AsyncPanZoomController* aPrev, ScreenPoint aStartPoint, ScreenPoint aEndPoint,
                                uint32_t aOverscrollHandoffChainIndex)
{
  
  
  if (aOverscrollHandoffChainIndex >= mOverscrollHandoffChain.length()) {
    
    return;
  }

  nsRefPtr<AsyncPanZoomController> next = mOverscrollHandoffChain[aOverscrollHandoffChainIndex];
  if (next == nullptr)
    return;

  
  
  
  
  
  if (next != aPrev) {
    gfx3DMatrix transformToApzc;
    gfx3DMatrix transformToGecko;  

    
    GetInputTransforms(aPrev, transformToApzc, transformToGecko);
    ApplyTransform(&aStartPoint, transformToApzc.Inverse());
    ApplyTransform(&aEndPoint, transformToApzc.Inverse());

    
    GetInputTransforms(next, transformToApzc, transformToGecko);
    ApplyTransform(&aStartPoint, transformToApzc);
    ApplyTransform(&aEndPoint, transformToApzc);
  }

  
  
  next->AttemptScroll(aStartPoint, aEndPoint, aOverscrollHandoffChainIndex);
}

bool
APZCTreeManager::HitTestAPZC(const ScreenIntPoint& aPoint)
{
  MonitorAutoLock lock(mTreeLock);
  nsRefPtr<AsyncPanZoomController> target;
  
  gfxPoint point(aPoint.x, aPoint.y);
  for (AsyncPanZoomController* apzc = mRootApzc; apzc; apzc = apzc->GetPrevSibling()) {
    target = GetAPZCAtPoint(apzc, point);
    if (target) {
      return true;
    }
  }
  return false;
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

struct CompareByScrollPriority
{
  bool operator()(const nsRefPtr<AsyncPanZoomController>& a, const nsRefPtr<AsyncPanZoomController>& b) {
    return a->HasScrollgrab() && !b->HasScrollgrab();
  }
};

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

void
APZCTreeManager::BuildOverscrollHandoffChain(const nsRefPtr<AsyncPanZoomController>& aInitialTarget)
{
  
  
  
  
  
  
  
  
  

  mOverscrollHandoffChain.clear();

  
  for (AsyncPanZoomController* apzc = aInitialTarget; apzc; apzc = apzc->GetParent()) {
    if (!mOverscrollHandoffChain.append(apzc)) {
      NS_WARNING("Vector::append failed");
      mOverscrollHandoffChain.clear();
      return;
    }
  }

  
  
  
  
  
  
  
  
  std::stable_sort(mOverscrollHandoffChain.begin(), mOverscrollHandoffChain.end(),
                   CompareByScrollPriority());
}

AsyncPanZoomController*
APZCTreeManager::FindTargetAPZC(AsyncPanZoomController* aApzc, const ScrollableLayerGuid& aGuid)
{
  mTreeLock.AssertCurrentThreadOwns();

  
  
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
  mTreeLock.AssertCurrentThreadOwns();

  
  
  
  

  
  
  
  
  gfx3DMatrix ancestorUntransform = aApzc->GetAncestorTransform().Inverse();

  
  
  
  
  gfx3DMatrix hitTestTransform = ancestorUntransform
                               * aApzc->GetCSSTransform().Inverse()
                               * aApzc->GetNontransientAsyncTransform().Inverse();
  gfxPoint hitTestPointForThisLayer = hitTestTransform.ProjectPoint(aHitTestPoint);
  APZC_LOG("Untransformed %f %f to transient coordinates %f %f for hit-testing APZC %p\n",
           aHitTestPoint.x, aHitTestPoint.y,
           hitTestPointForThisLayer.x, hitTestPointForThisLayer.y, aApzc);

  
  
  
  
  gfx3DMatrix childUntransform = ancestorUntransform
                               * aApzc->GetCSSTransform().Inverse()
                               * gfx3DMatrix(aApzc->GetCurrentAsyncTransform()).Inverse();
  gfxPoint hitTestPointForChildLayers = childUntransform.ProjectPoint(aHitTestPoint);
  APZC_LOG("Untransformed %f %f to layer coordinates %f %f for APZC %p\n",
           aHitTestPoint.x, aHitTestPoint.y,
           hitTestPointForChildLayers.x, hitTestPointForChildLayers.y, aApzc);

  
  
  for (AsyncPanZoomController* child = aApzc->GetLastChild(); child; child = child->GetPrevSibling()) {
    AsyncPanZoomController* match = GetAPZCAtPoint(child, hitTestPointForChildLayers);
    if (match) {
      return match;
    }
  }
  if (aApzc->VisibleRegionContains(ScreenPoint(hitTestPointForThisLayer.x, hitTestPointForThisLayer.y))) {
    APZC_LOG("Successfully matched untransformed point %f %f to visible region for APZC %p\n",
             hitTestPointForThisLayer.x, hitTestPointForThisLayer.y, aApzc);
    return aApzc;
  }
  return nullptr;
}















































































void
APZCTreeManager::GetInputTransforms(AsyncPanZoomController *aApzc, gfx3DMatrix& aTransformToApzcOut,
                                    gfx3DMatrix& aTransformToGeckoOut)
{
  MonitorAutoLock lock(mTreeLock);

  
  
  
  
  

  
  gfx3DMatrix ancestorUntransform = aApzc->GetAncestorTransform().Inverse();
  
  gfx3DMatrix asyncUntransform = gfx3DMatrix(aApzc->GetCurrentAsyncTransform()).Inverse();
  
  gfx3DMatrix nontransientAsyncTransform = aApzc->GetNontransientAsyncTransform();
  
  gfx3DMatrix transientAsyncUntransform = nontransientAsyncTransform * asyncUntransform;

  
  aTransformToApzcOut = ancestorUntransform * aApzc->GetCSSTransform().Inverse() * nontransientAsyncTransform.Inverse();
  
  aTransformToGeckoOut = transientAsyncUntransform * aApzc->GetCSSTransform() * aApzc->GetAncestorTransform();

  for (AsyncPanZoomController* parent = aApzc->GetParent(); parent; parent = parent->GetParent()) {
    
    ancestorUntransform = parent->GetAncestorTransform().Inverse();
    
    asyncUntransform = gfx3DMatrix(parent->GetCurrentAsyncTransform()).Inverse();
    
    gfx3DMatrix untransformSinceLastApzc = ancestorUntransform * parent->GetCSSTransform().Inverse() * asyncUntransform;

    
    aTransformToApzcOut = untransformSinceLastApzc * aTransformToApzcOut;
    
    aTransformToGeckoOut = aTransformToGeckoOut * parent->GetCSSTransform() * parent->GetAncestorTransform();

    
    
    
  }
}

already_AddRefed<AsyncPanZoomController>
APZCTreeManager::CommonAncestor(AsyncPanZoomController* aApzc1, AsyncPanZoomController* aApzc2)
{
  MonitorAutoLock lock(mTreeLock);
  nsRefPtr<AsyncPanZoomController> ancestor;

  
  

  
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
      ancestor = aApzc1;
      break;
    }
    if (depth1 <= 0) {
      break;
    }
    aApzc1 = aApzc1->GetParent();
    aApzc2 = aApzc2->GetParent();
  }
  return ancestor.forget();
}

already_AddRefed<AsyncPanZoomController>
APZCTreeManager::RootAPZCForLayersId(AsyncPanZoomController* aApzc)
{
  MonitorAutoLock lock(mTreeLock);
  nsRefPtr<AsyncPanZoomController> apzc = aApzc;
  while (apzc && !apzc->IsRootForLayersId()) {
    apzc = apzc->GetParent();
  }
  return apzc.forget();
}

}
}
