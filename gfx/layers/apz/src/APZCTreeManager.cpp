




#include "APZCTreeManager.h"
#include "Compositor.h"                 
#include "CompositorParent.h"           
#include "InputData.h"                  
#include "Layers.h"                     
#include "gfx3DMatrix.h"                
#include "mozilla/dom/Touch.h"          
#include "mozilla/gfx/Point.h"          
#include "mozilla/layers/AsyncCompositionManager.h" 
#include "mozilla/layers/AsyncPanZoomController.h"
#include "mozilla/MouseEvents.h"
#include "mozilla/mozalloc.h"           
#include "mozilla/TouchEvents.h"
#include "mozilla/Preferences.h"        
#include "nsDebug.h"                    
#include "nsPoint.h"                    
#include "nsThreadUtils.h"              
#include "mozilla/gfx/Logging.h"        
#include "UnitTransforms.h"             
#include "gfxPrefs.h"                   

#include <algorithm>                    

#define APZCTM_LOG(...)


namespace mozilla {
namespace layers {

float APZCTreeManager::sDPI = 160.0;

APZCTreeManager::APZCTreeManager()
    : mTreeLock("APZCTreeLock"),
      mInOverscrolledApzc(false),
      mRetainedTouchIdentifier(-1),
      mTouchCount(0),
      mApzcTreeLog("apzctree")
{
  MOZ_ASSERT(NS_IsMainThread());
  AsyncPanZoomController::InitializeGlobalState();
  mApzcTreeLog.ConditionOnPrefFunction(gfxPrefs::APZPrintTree);
}

APZCTreeManager::~APZCTreeManager()
{
}

void
APZCTreeManager::GetAllowedTouchBehavior(WidgetInputEvent* aEvent,
                                         nsTArray<TouchBehaviorFlags>& aOutValues)
{
  WidgetTouchEvent *touchEvent = aEvent->AsTouchEvent();

  aOutValues.Clear();

  for (size_t i = 0; i < touchEvent->touches.Length(); i++) {
    
    
    mozilla::ScreenIntPoint spt;
    spt.x = touchEvent->touches[i]->mRefPoint.x;
    spt.y = touchEvent->touches[i]->mRefPoint.y;

    nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(spt, nullptr);
    aOutValues.AppendElement(apzc
      ? apzc->GetAllowedTouchBehavior(spt)
      : AllowedTouchBehavior::UNKNOWN);
  }
}

void
APZCTreeManager::SetAllowedTouchBehavior(const ScrollableLayerGuid& aGuid,
                                         const nsTArray<TouchBehaviorFlags> &aValues)
{
  nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(aGuid);
  if (apzc) {
    apzc->SetAllowedTouchBehavior(aValues);
  }
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
APZCTreeManager::UpdatePanZoomControllerTree(CompositorParent* aCompositor,
                                             Layer* aRoot,
                                             bool aIsFirstPaint,
                                             uint64_t aOriginatingLayersId,
                                             uint32_t aPaintSequenceNumber)
{
  if (AsyncPanZoomController::GetThreadAssertionsEnabled()) {
    Compositor::AssertOnCompositorThread();
  }

  MonitorAutoLock lock(mTreeLock);

  
  
  
  
  
  
  
  
  
  
  
  
  nsTArray< nsRefPtr<AsyncPanZoomController> > apzcsToDestroy;
  Collect(mRootApzc, &apzcsToDestroy);
  mRootApzc = nullptr;

  
  
  APZTestData* testData = nullptr;
  if (gfxPrefs::APZTestLoggingEnabled()) {
    if (CompositorParent::LayerTreeState* state = CompositorParent::GetIndirectShadowTree(aOriginatingLayersId)) {
      testData = &state->mApzTestData;
      testData->StartNewPaint(aPaintSequenceNumber);
    }
  }
  APZPaintLogHelper paintLogger(testData, aPaintSequenceNumber);

  if (aRoot) {
    mApzcTreeLog << "[start]\n";
    UpdatePanZoomControllerTree(aCompositor,
                                aRoot,
                                
                                aCompositor ? aCompositor->RootLayerTreeId() : 0,
                                gfx3DMatrix(), nullptr, nullptr,
                                aIsFirstPaint, aOriginatingLayersId,
                                paintLogger, &apzcsToDestroy);
    mApzcTreeLog << "[end]\n";
  }

  for (size_t i = 0; i < apzcsToDestroy.Length(); i++) {
    APZCTM_LOG("Destroying APZC at %p\n", apzcsToDestroy[i].get());
    apzcsToDestroy[i]->Destroy();
  }
}

AsyncPanZoomController*
APZCTreeManager::UpdatePanZoomControllerTree(CompositorParent* aCompositor,
                                             Layer* aLayer, uint64_t aLayersId,
                                             gfx3DMatrix aTransform,
                                             AsyncPanZoomController* aParent,
                                             AsyncPanZoomController* aNextSibling,
                                             bool aIsFirstPaint,
                                             uint64_t aOriginatingLayersId,
                                             const APZPaintLogHelper& aPaintLogger,
                                             nsTArray< nsRefPtr<AsyncPanZoomController> >* aApzcsToDestroy)
{
  mTreeLock.AssertCurrentThreadOwns();

  ContainerLayer* container = aLayer->AsContainerLayer();
  AsyncPanZoomController* apzc = nullptr;
  mApzcTreeLog << aLayer->Name() << '\t';
  if (container) {
    const FrameMetrics& metrics = container->GetFrameMetrics();
    if (metrics.IsScrollable()) {
      const CompositorParent::LayerTreeState* state = CompositorParent::GetIndirectShadowTree(aLayersId);
      if (state && state->mController.get()) {
        
        
        

        apzc = container->GetAsyncPanZoomController();

        
        
        
        
        ScrollableLayerGuid guid(aLayersId, metrics);
        if (apzc && !apzc->Matches(guid)) {
          apzc = nullptr;
        }

        
        
        
        
        
        
        if (apzc == nullptr) {
          for (size_t i = 0; i < aApzcsToDestroy->Length(); i++) {
            if (aApzcsToDestroy->ElementAt(i)->Matches(guid)) {
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
          if (state->mCrossProcessParent != nullptr) {
            apzc->ShareFrameMetricsAcrossProcesses();
          }
        } else {
          
          
          
          
          
          aApzcsToDestroy->RemoveElement(apzc);
          apzc->SetPrevSibling(nullptr);
          apzc->SetLastChild(nullptr);
        }
        APZCTM_LOG("Using APZC %p for layer %p with identifiers %lld %lld\n", apzc, aLayer, aLayersId, container->GetFrameMetrics().GetScrollId());

        apzc->NotifyLayersUpdated(metrics,
                                  aIsFirstPaint && (aLayersId == aOriginatingLayersId));
        apzc->SetScrollHandoffParentId(container->GetScrollHandoffParentId());

        
        
        
        
        ParentLayerRect visible(metrics.mCompositionBounds);
        CSSRect touchSensitiveRegion;
        if (state->mController->GetTouchSensitiveRegion(&touchSensitiveRegion)) {
          
          
          
          visible = visible.Intersect(touchSensitiveRegion
                                      * metrics.mDevPixelsPerCSSPixel
                                      * metrics.GetParentResolution());
        }
        gfx3DMatrix transform;
        gfx::To3DMatrix(aLayer->GetTransform(), transform);

        apzc->SetLayerHitTestData(visible, aTransform, transform);
        APZCTM_LOG("Setting rect(%f %f %f %f) as visible region for APZC %p\n", visible.x, visible.y,
                                                                              visible.width, visible.height,
                                                                              apzc);

        mApzcTreeLog << "APZC " << guid
                     << "\tcb=" << visible
                     << "\tsr=" << container->GetFrameMetrics().mScrollableRect
                     << (aLayer->GetVisibleRegion().IsEmpty() ? "\tscrollinfo" : "")
                     << "\t" << container->GetFrameMetrics().GetContentDescription();

        
        if (aNextSibling) {
          aNextSibling->SetPrevSibling(apzc);
        } else if (aParent) {
          aParent->SetLastChild(apzc);
        } else {
          mRootApzc = apzc;
          apzc->MakeRoot();
        }

        
        
        
        
        
        
        if (aLayersId == aOriginatingLayersId && apzc->GetParent()) {
          aPaintLogger.LogTestData(metrics.GetScrollId(), "parentScrollId",
              apzc->GetParent()->GetGuid().mScrollId);
        }

        
        aParent = apzc;

        if (newApzc) {
          if (apzc->IsRootForLayersId()) {
            
            
            
            ZoomConstraints constraints;
            if (state->mController->GetRootZoomConstraints(&constraints)) {
              apzc->UpdateZoomConstraints(constraints);
            }
          } else {
            
            
            
            
            apzc->UpdateZoomConstraints(apzc->GetParent()->GetZoomConstraints());
          }
        }
      }
    }

    container->SetAsyncPanZoomController(apzc);
  }
  mApzcTreeLog << '\n';

  
  
  if (apzc) {
    aTransform = gfx3DMatrix();
  } else {
    
    gfx3DMatrix matrix;
    gfx::To3DMatrix(aLayer->GetTransform(), matrix);
    aTransform = matrix * aTransform;
  }

  uint64_t childLayersId = (aLayer->AsRefLayer() ? aLayer->AsRefLayer()->GetReferentId() : aLayersId);
  
  
  AsyncPanZoomController* next = apzc ? nullptr : aNextSibling;
  for (Layer* child = aLayer->GetLastChild(); child; child = child->GetPrevSibling()) {
    gfx::TreeAutoIndent indent(mApzcTreeLog);
    next = UpdatePanZoomControllerTree(aCompositor, child, childLayersId, aTransform, aParent, next,
                                       aIsFirstPaint, aOriginatingLayersId,
                                       aPaintLogger, aApzcsToDestroy);
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

 template<class T> void
TransformScreenToGecko(T* aPoint, AsyncPanZoomController* aApzc, APZCTreeManager* aApzcTm)
{
  gfx3DMatrix transformToApzc, transformToGecko;
  aApzcTm->GetInputTransforms(aApzc, transformToApzc, transformToGecko);
  ApplyTransform(aPoint, transformToApzc * transformToGecko);
}

nsEventStatus
APZCTreeManager::ReceiveInputEvent(InputData& aEvent,
                                   ScrollableLayerGuid* aOutTargetGuid)
{
  nsEventStatus result = nsEventStatus_eIgnore;
  gfx3DMatrix transformToApzc;
  gfx3DMatrix transformToGecko;
  switch (aEvent.mInputType) {
    case MULTITOUCH_INPUT: {
      MultiTouchInput& touchInput = aEvent.AsMultiTouchInput();
      result = ProcessTouchInput(touchInput, aOutTargetGuid);
      break;
    } case PANGESTURE_INPUT: {
      PanGestureInput& panInput = aEvent.AsPanGestureInput();
      bool inOverscrolledApzc = false;
      nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(panInput.mPanStartPoint,
                                                            &inOverscrolledApzc);
      if (apzc) {
        if (panInput.mType == PanGestureInput::PANGESTURE_START ||
            panInput.mType == PanGestureInput::PANGESTURE_MOMENTUMSTART) {
          BuildOverscrollHandoffChain(apzc);
        }

        
        
        
        PanGestureInput inputForApzc(panInput);
        GetInputTransforms(apzc, transformToApzc, transformToGecko);
        ApplyTransform(&(inputForApzc.mPanStartPoint), transformToApzc);
        result = apzc->ReceiveInputEvent(inputForApzc);

        
        apzc->GetGuid(aOutTargetGuid);
        TransformScreenToGecko(&(panInput.mPanStartPoint), apzc, this);

        if (panInput.mType == PanGestureInput::PANGESTURE_END ||
            panInput.mType == PanGestureInput::PANGESTURE_MOMENTUMEND) {
          ClearOverscrollHandoffChain();
        }
      }
      break;
    } case PINCHGESTURE_INPUT: {
      PinchGestureInput& pinchInput = aEvent.AsPinchGestureInput();
      bool inOverscrolledApzc = false;
      nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(pinchInput.mFocusPoint,
                                                            &inOverscrolledApzc);
      if (apzc) {
        
        
        
        PinchGestureInput inputForApzc(pinchInput);
        GetInputTransforms(apzc, transformToApzc, transformToGecko);
        ApplyTransform(&(inputForApzc.mFocusPoint), transformToApzc);
        apzc->ReceiveInputEvent(inputForApzc);

        
        apzc->GetGuid(aOutTargetGuid);
        TransformScreenToGecko(&(pinchInput.mFocusPoint), apzc, this);
      }
      result = inOverscrolledApzc ? nsEventStatus_eConsumeNoDefault
             : apzc ? nsEventStatus_eConsumeDoDefault
             : nsEventStatus_eIgnore;
      break;
    } case TAPGESTURE_INPUT: {
      TapGestureInput& tapInput = aEvent.AsTapGestureInput();
      bool inOverscrolledApzc = false;
      nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(ScreenPoint(tapInput.mPoint),
                                                            &inOverscrolledApzc);
      if (apzc) {
        
        
        
        TapGestureInput inputForApzc(tapInput);
        GetInputTransforms(apzc, transformToApzc, transformToGecko);
        ApplyTransform(&(inputForApzc.mPoint), transformToApzc);
        apzc->ReceiveInputEvent(inputForApzc);

        
        apzc->GetGuid(aOutTargetGuid);
        TransformScreenToGecko(&(tapInput.mPoint), apzc, this);
      }
      result = inOverscrolledApzc ? nsEventStatus_eConsumeNoDefault
             : apzc ? nsEventStatus_eConsumeDoDefault
             : nsEventStatus_eIgnore;
      break;
    }
  }
  return result;
}

already_AddRefed<AsyncPanZoomController>
APZCTreeManager::GetTouchInputBlockAPZC(const MultiTouchInput& aEvent,
                                        bool* aOutInOverscrolledApzc)
{
  nsRefPtr<AsyncPanZoomController> apzc;
  if (aEvent.mTouches.Length() == 0) {
    return apzc.forget();
  }

  apzc = GetTargetAPZC(aEvent.mTouches[0].mScreenPoint, aOutInOverscrolledApzc);
  for (size_t i = 1; i < aEvent.mTouches.Length(); i++) {
    nsRefPtr<AsyncPanZoomController> apzc2 = GetTargetAPZC(aEvent.mTouches[i].mScreenPoint, aOutInOverscrolledApzc);
    apzc = CommonAncestor(apzc.get(), apzc2.get());
    APZCTM_LOG("Using APZC %p as the common ancestor\n", apzc.get());
    
    
    apzc = RootAPZCForLayersId(apzc);
    APZCTM_LOG("Using APZC %p as the root APZC for multi-touch\n", apzc.get());
  }

  
  BuildOverscrollHandoffChain(apzc);

  return apzc.forget();
}

nsEventStatus
APZCTreeManager::ProcessTouchInput(MultiTouchInput& aInput,
                                   ScrollableLayerGuid* aOutTargetGuid)
{
  if (aInput.mType == MultiTouchInput::MULTITOUCH_START) {
    
    
    
    
    if (mApzcForInputBlock && mApzcForInputBlock->IsOverscrolled()) {
      if (mRetainedTouchIdentifier == -1) {
        mRetainedTouchIdentifier = mApzcForInputBlock->GetLastTouchIdentifier();
      }
      return nsEventStatus_eConsumeNoDefault;
    }

    
    
    mTouchCount = aInput.mTouches.Length();
    mInOverscrolledApzc = false;
    nsRefPtr<AsyncPanZoomController> apzc = GetTouchInputBlockAPZC(aInput, &mInOverscrolledApzc);
    if (apzc != mApzcForInputBlock) {
      
      
      
      if (mApzcForInputBlock) {
        MultiTouchInput cancel(MultiTouchInput::MULTITOUCH_CANCEL, 0, TimeStamp::Now(), 0);
        mApzcForInputBlock->ReceiveInputEvent(cancel);
      }
      mApzcForInputBlock = apzc;
    }

    if (mApzcForInputBlock) {
      
      gfx3DMatrix transformToGecko;
      GetInputTransforms(mApzcForInputBlock, mCachedTransformToApzcForInputBlock, transformToGecko);
    } else {
      
      mCachedTransformToApzcForInputBlock = gfx3DMatrix();
    }
  } else if (mApzcForInputBlock) {
    APZCTM_LOG("Re-using APZC %p as continuation of event block\n", mApzcForInputBlock.get());
  }

  
  
  if (aInput.mType == MultiTouchInput::MULTITOUCH_CANCEL) {
    mRetainedTouchIdentifier = -1;
  }

  
  
  
  
  if (mRetainedTouchIdentifier != -1) {
    for (size_t j = 0; j < aInput.mTouches.Length(); ++j) {
      if (aInput.mTouches[j].mIdentifier != mRetainedTouchIdentifier) {
        aInput.mTouches.RemoveElementAt(j);
        --j;
      }
    }
    if (aInput.mTouches.IsEmpty()) {
      return nsEventStatus_eConsumeNoDefault;
    }
  }

  if (mApzcForInputBlock) {
    mApzcForInputBlock->GetGuid(aOutTargetGuid);
    
    
    
    gfx3DMatrix transformToApzc = mCachedTransformToApzcForInputBlock;
    MultiTouchInput inputForApzc(aInput);
    for (size_t i = 0; i < inputForApzc.mTouches.Length(); i++) {
      ApplyTransform(&(inputForApzc.mTouches[i].mScreenPoint), transformToApzc);
    }
    mApzcForInputBlock->ReceiveInputEvent(inputForApzc);

    
    
    
    gfx3DMatrix transformToGecko;
    GetInputTransforms(mApzcForInputBlock, transformToApzc, transformToGecko);
    gfx3DMatrix outTransform = transformToApzc * transformToGecko;
    for (size_t i = 0; i < aInput.mTouches.Length(); i++) {
      ApplyTransform(&(aInput.mTouches[i].mScreenPoint), outTransform);
    }
  }

  nsEventStatus result = mInOverscrolledApzc ? nsEventStatus_eConsumeNoDefault
                       : mApzcForInputBlock ? nsEventStatus_eConsumeDoDefault
                       : nsEventStatus_eIgnore;

  if (aInput.mType == MultiTouchInput::MULTITOUCH_END) {
    if (mTouchCount >= aInput.mTouches.Length()) {
      
      mTouchCount -= aInput.mTouches.Length();
    } else {
      NS_WARNING("Got an unexpected touchend/touchcancel");
      mTouchCount = 0;
    }
  } else if (aInput.mType == MultiTouchInput::MULTITOUCH_CANCEL) {
    mTouchCount = 0;
  }

  
  
  if (mTouchCount == 0) {
    mApzcForInputBlock = nullptr;
    mInOverscrolledApzc = false;
    mRetainedTouchIdentifier = -1;
    ClearOverscrollHandoffChain();
  }

  return result;
}

void
APZCTreeManager::TransformCoordinateToGecko(const ScreenIntPoint& aPoint,
                                            LayoutDeviceIntPoint* aOutTransformedPoint)
{
  MOZ_ASSERT(aOutTransformedPoint);
  nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(aPoint, nullptr);
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
APZCTreeManager::ProcessEvent(WidgetInputEvent& aEvent,
                              ScrollableLayerGuid* aOutTargetGuid)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  bool inOverscrolledApzc = false;
  nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(ScreenPoint(aEvent.refPoint.x, aEvent.refPoint.y),
                                                        &inOverscrolledApzc);
  if (apzc) {
    apzc->GetGuid(aOutTargetGuid);
    gfx3DMatrix transformToApzc;
    gfx3DMatrix transformToGecko;
    GetInputTransforms(apzc, transformToApzc, transformToGecko);
    gfx3DMatrix outTransform = transformToApzc * transformToGecko;
    ApplyTransform(&(aEvent.refPoint), outTransform);
  }
  return inOverscrolledApzc ? nsEventStatus_eConsumeNoDefault
       : apzc ? nsEventStatus_eConsumeDoDefault
       : nsEventStatus_eIgnore;
}

nsEventStatus
APZCTreeManager::ReceiveInputEvent(WidgetInputEvent& aEvent,
                                   ScrollableLayerGuid* aOutTargetGuid)
{
  
  
  

  MOZ_ASSERT(NS_IsMainThread());

  switch (aEvent.eventStructType) {
    case NS_TOUCH_EVENT: {
      WidgetTouchEvent& touchEvent = *aEvent.AsTouchEvent();
      MultiTouchInput touchInput(touchEvent);
      nsEventStatus result = ProcessTouchInput(touchInput, aOutTargetGuid);
      
      
      
      
      touchEvent.touches.Clear();
      touchEvent.touches.SetCapacity(touchInput.mTouches.Length());
      for (size_t i = 0; i < touchInput.mTouches.Length(); i++) {
        *touchEvent.touches.AppendElement() = touchInput.mTouches[i].ToNewDOMTouch();
      }
      return result;
    }
    default: {
      return ProcessEvent(aEvent, aOutTargetGuid);
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
                                       const ZoomConstraints& aConstraints)
{
  nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(aGuid);
  
  
  if (apzc && apzc->IsRootForLayersId()) {
    MonitorAutoLock lock(mTreeLock);
    UpdateZoomConstraintsRecursively(apzc.get(), aConstraints);
  }
}

void
APZCTreeManager::UpdateZoomConstraintsRecursively(AsyncPanZoomController* aApzc,
                                                  const ZoomConstraints& aConstraints)
{
  mTreeLock.AssertCurrentThreadOwns();

  aApzc->UpdateZoomConstraints(aConstraints);
  for (AsyncPanZoomController* child = aApzc->GetLastChild(); child; child = child->GetPrevSibling()) {
    
    if (!child->IsRootForLayersId()) {
      UpdateZoomConstraintsRecursively(child, aConstraints);
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











static void
TransformDisplacement(APZCTreeManager* aTreeManager,
                      AsyncPanZoomController* aSource,
                      AsyncPanZoomController* aTarget,
                      ScreenPoint& aStartPoint,
                      ScreenPoint& aEndPoint) {
  gfx3DMatrix transformToApzc;
  gfx3DMatrix transformToGecko;  

  
  aTreeManager->GetInputTransforms(aSource, transformToApzc, transformToGecko);
  ApplyTransform(&aStartPoint, transformToApzc.Inverse());
  ApplyTransform(&aEndPoint, transformToApzc.Inverse());

  
  aTreeManager->GetInputTransforms(aTarget, transformToApzc, transformToGecko);
  ApplyTransform(&aStartPoint, transformToApzc);
  ApplyTransform(&aEndPoint, transformToApzc);
}

bool
APZCTreeManager::DispatchScroll(AsyncPanZoomController* aPrev, ScreenPoint aStartPoint, ScreenPoint aEndPoint,
                                uint32_t aOverscrollHandoffChainIndex)
{
  nsRefPtr<AsyncPanZoomController> next;
  {
    
    
    
    
    MonitorAutoLock lock(mTreeLock);

    
    
    if (aOverscrollHandoffChainIndex >= mOverscrollHandoffChain.length()) {
      
      return false;
    }

    next = mOverscrollHandoffChain[aOverscrollHandoffChainIndex];
  }

  if (next == nullptr) {
    return false;
  }

  if (next->GetGuid().mLayersId != aPrev->GetGuid().mLayersId) {
    NS_WARNING("Handing off scroll across a layer tree boundary; may need to revise approach to bug 1031067");
  }

  
  
  
  
  
  if (next != aPrev) {
    TransformDisplacement(this, aPrev, next, aStartPoint, aEndPoint);
  }

  
  
  return next->AttemptScroll(aStartPoint, aEndPoint, aOverscrollHandoffChainIndex);
}

bool
APZCTreeManager::HandOffFling(AsyncPanZoomController* aPrev, ScreenPoint aVelocity)
{
  
  
  
  
  
  
  BuildOverscrollHandoffChain(aPrev);

  nsRefPtr<AsyncPanZoomController> next;  
  {
    
    
    
    
    MonitorAutoLock lock(mTreeLock);

    
    uint32_t i;
    for (i = 0; i < mOverscrollHandoffChain.length(); ++i) {
      if (mOverscrollHandoffChain[i] == aPrev) {
        break;
      }
    }

    
    if (i + 1 < mOverscrollHandoffChain.length()) {
      next = mOverscrollHandoffChain[i + 1];
    }

    
    
    mOverscrollHandoffChain.clear();
  }

  
  if (next == nullptr) {
    return false;
  }

  
  
  
  
  
  
  
  ScreenPoint startPoint;  
  ScreenPoint endPoint = startPoint + aVelocity;
  TransformDisplacement(this, aPrev, next, startPoint, endPoint);
  ScreenPoint transformedVelocity = endPoint - startPoint;

  
  return next->TakeOverFling(transformedVelocity);
}

bool
APZCTreeManager::FlushRepaintsForOverscrollHandoffChain()
{
  MonitorAutoLock lock(mTreeLock);  
  if (mOverscrollHandoffChain.length() == 0) {
    return false;
  }
  for (uint32_t i = 0; i < mOverscrollHandoffChain.length(); i++) {
    nsRefPtr<AsyncPanZoomController> item = mOverscrollHandoffChain[i];
    if (item) {
      item->FlushRepaintForOverscrollHandoff();
    }
  }
  return true;
}

bool
APZCTreeManager::CanBePanned(AsyncPanZoomController* aApzc)
{
  MonitorAutoLock lock(mTreeLock);  

  
  uint32_t i;
  for (i = 0; i < mOverscrollHandoffChain.length(); ++i) {
    if (mOverscrollHandoffChain[i] == aApzc) {
      break;
    }
  }

  
  
  for (uint32_t j = i; j < mOverscrollHandoffChain.length(); ++j) {
    if (mOverscrollHandoffChain[j]->IsPannable()) {
      return true;
    }
  }

  return false;
}

bool
APZCTreeManager::HitTestAPZC(const ScreenIntPoint& aPoint)
{
  nsRefPtr<AsyncPanZoomController> target = GetTargetAPZC(aPoint, nullptr);
  return target != nullptr;
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
APZCTreeManager::GetTargetAPZC(const ScreenPoint& aPoint, bool* aOutInOverscrolledApzc)
{
  MonitorAutoLock lock(mTreeLock);
  nsRefPtr<AsyncPanZoomController> target;
  
  gfxPoint point(aPoint.x, aPoint.y);
  bool inOverscrolledApzc = false;
  for (AsyncPanZoomController* apzc = mRootApzc; apzc; apzc = apzc->GetPrevSibling()) {
    target = GetAPZCAtPoint(apzc, point, &inOverscrolledApzc);
    if (target) {
      break;
    }
  }
  
  MOZ_ASSERT(!(target && inOverscrolledApzc));
  if (aOutInOverscrolledApzc) {
    *aOutInOverscrolledApzc = inOverscrolledApzc;
  }
  return target.forget();
}

void
APZCTreeManager::BuildOverscrollHandoffChain(const nsRefPtr<AsyncPanZoomController>& aInitialTarget)
{
  
  
  
  
  
  
  
  
  

  
  
  MonitorAutoLock lock(mTreeLock);

  mOverscrollHandoffChain.clear();

  
  
  
  
  AsyncPanZoomController* apzc = aInitialTarget;
  while (apzc != nullptr) {
    if (!mOverscrollHandoffChain.append(apzc)) {
      NS_WARNING("Vector::append failed");
      mOverscrollHandoffChain.clear();
      return;
    }
    APZCTM_LOG("mOverscrollHandoffChain[%d] = %p\n", mOverscrollHandoffChain.length(), apzc);

    if (apzc->GetScrollHandoffParentId() == FrameMetrics::NULL_SCROLL_ID) {
      if (!apzc->IsRootForLayersId()) {
        
        NS_WARNING("Found a non-root APZ with no handoff parent");
      }
      apzc = apzc->GetParent();
      continue;
    }

    
    
    
    
    AsyncPanZoomController* scrollParent = nullptr;
    AsyncPanZoomController* parent = apzc;
    while (!parent->IsRootForLayersId()) {
      parent = parent->GetParent();
      
      
      
      if (parent->GetGuid().mScrollId == apzc->GetScrollHandoffParentId()) {
        scrollParent = parent;
        break;
      }
    }
    if (!scrollParent) {
      scrollParent = FindTargetAPZC(parent, apzc->GetScrollHandoffParentId());
    }
    apzc = scrollParent;
  }

  
  
  
  
  
  
  
  
  std::stable_sort(mOverscrollHandoffChain.begin(), mOverscrollHandoffChain.end(),
                   CompareByScrollPriority());
}




AsyncPanZoomController*
APZCTreeManager::FindTargetAPZC(AsyncPanZoomController* aApzc, FrameMetrics::ViewID aScrollId)
{
  mTreeLock.AssertCurrentThreadOwns();

  if (aApzc->GetGuid().mScrollId == aScrollId) {
    return aApzc;
  }
  for (AsyncPanZoomController* child = aApzc->GetLastChild(); child; child = child->GetPrevSibling()) {
    if (child->GetGuid().mLayersId != aApzc->GetGuid().mLayersId) {
      continue;
    }
    AsyncPanZoomController* match = FindTargetAPZC(child, aScrollId);
    if (match) {
      return match;
    }
  }

  return nullptr;
}

void
APZCTreeManager::ClearOverscrollHandoffChain()
{
  MonitorAutoLock lock(mTreeLock);
  mOverscrollHandoffChain.clear();
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
APZCTreeManager::GetAPZCAtPoint(AsyncPanZoomController* aApzc,
                                const gfxPoint& aHitTestPoint,
                                bool* aOutInOverscrolledApzc)
{
  mTreeLock.AssertCurrentThreadOwns();

  
  
  
  

  
  
  
  
  gfx3DMatrix ancestorUntransform = aApzc->GetAncestorTransform().Inverse();

  
  
  
  gfxPointH3D hitTestPointForThisLayer = ancestorUntransform.ProjectPoint(aHitTestPoint);
  APZCTM_LOG("Untransformed %f %f to transient coordinates %f %f for hit-testing APZC %p\n",
           aHitTestPoint.x, aHitTestPoint.y,
           hitTestPointForThisLayer.x, hitTestPointForThisLayer.y, aApzc);

  
  
  
  
  gfx3DMatrix childUntransform = ancestorUntransform
                               * aApzc->GetCSSTransform().Inverse()
                               * gfx3DMatrix(aApzc->GetCurrentAsyncTransform()).Inverse();
  gfxPointH3D hitTestPointForChildLayers = childUntransform.ProjectPoint(aHitTestPoint);
  APZCTM_LOG("Untransformed %f %f to layer coordinates %f %f for APZC %p\n",
           aHitTestPoint.x, aHitTestPoint.y,
           hitTestPointForChildLayers.x, hitTestPointForChildLayers.y, aApzc);

  AsyncPanZoomController* result = nullptr;
  
  
  if (hitTestPointForChildLayers.HasPositiveWCoord()) {
    for (AsyncPanZoomController* child = aApzc->GetLastChild(); child; child = child->GetPrevSibling()) {
      AsyncPanZoomController* match = GetAPZCAtPoint(child, hitTestPointForChildLayers.As2DPoint(), aOutInOverscrolledApzc);
      if (*aOutInOverscrolledApzc) {
        
        return nullptr;
      }
      if (match) {
        result = match;
        break;
      }
    }
  }
  if (!result && hitTestPointForThisLayer.HasPositiveWCoord() &&
      aApzc->VisibleRegionContains(ViewAs<ParentLayerPixel>(hitTestPointForThisLayer.As2DPoint()))) {
    APZCTM_LOG("Successfully matched untransformed point %f %f to visible region for APZC %p\n",
             hitTestPointForThisLayer.x, hitTestPointForThisLayer.y, aApzc);
    result = aApzc;
  }

  
  
  
  if (result && aApzc->IsOverscrolled()) {
    *aOutInOverscrolledApzc = true;
    result = nullptr;
  }

  return result;
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
  
  aTransformToGeckoOut = transientAsyncUntransform * aApzc->GetTransformToLastDispatchedPaint() * aApzc->GetCSSTransform() * aApzc->GetAncestorTransform();

  for (AsyncPanZoomController* parent = aApzc->GetParent(); parent; parent = parent->GetParent()) {
    
    ancestorUntransform = parent->GetAncestorTransform().Inverse();
    
    asyncUntransform = gfx3DMatrix(parent->GetCurrentAsyncTransform()).Inverse();
    
    gfx3DMatrix untransformSinceLastApzc = ancestorUntransform * parent->GetCSSTransform().Inverse() * asyncUntransform;

    
    aTransformToApzcOut = untransformSinceLastApzc * aTransformToApzcOut;
    
    aTransformToGeckoOut = aTransformToGeckoOut * parent->GetTransformToLastDispatchedPaint() * parent->GetCSSTransform() * parent->GetAncestorTransform();

    
    
    
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
