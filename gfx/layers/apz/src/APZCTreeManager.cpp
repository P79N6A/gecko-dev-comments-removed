




#include "APZCTreeManager.h"
#include "Compositor.h"                 
#include "CompositorParent.h"           
#include "InputData.h"                  
#include "Layers.h"                     
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
#include "OverscrollHandoffChain.h"     

#define APZCTM_LOG(...)


namespace mozilla {
namespace layers {

typedef mozilla::gfx::Point Point;
typedef mozilla::gfx::Matrix4x4 Matrix4x4;

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
    std::map<ScrollableLayerGuid, AsyncPanZoomController*> apzcMap;
    UpdatePanZoomControllerTree(aCompositor,
                                aRoot,
                                
                                aCompositor ? aCompositor->RootLayerTreeId() : 0,
                                Matrix4x4(), nullptr, nullptr,
                                aIsFirstPaint, aOriginatingLayersId,
                                paintLogger, &apzcsToDestroy, apzcMap,
                                nsIntRegion());
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
                                             Matrix4x4 aTransform,
                                             AsyncPanZoomController* aParent,
                                             AsyncPanZoomController* aNextSibling,
                                             bool aIsFirstPaint,
                                             uint64_t aOriginatingLayersId,
                                             const APZPaintLogHelper& aPaintLogger,
                                             nsTArray< nsRefPtr<AsyncPanZoomController> >* aApzcsToDestroy,
                                             std::map<ScrollableLayerGuid, AsyncPanZoomController*>& aApzcMap,
                                             const nsIntRegion& aObscured)
{
  mTreeLock.AssertCurrentThreadOwns();

  Matrix4x4 transform = aLayer->GetTransform();

  AsyncPanZoomController* apzc = nullptr;
  mApzcTreeLog << aLayer->Name() << '\t';

  const FrameMetrics& metrics = aLayer->GetFrameMetrics();
  if (metrics.IsScrollable()) {
    const CompositorParent::LayerTreeState* state = CompositorParent::GetIndirectShadowTree(aLayersId);
    if (state && state->mController.get()) {
      
      
      

      
      
      
      
      ScrollableLayerGuid guid(aLayersId, metrics);
      auto insertResult = aApzcMap.insert(std::make_pair(guid, static_cast<AsyncPanZoomController*>(nullptr)));
      if (!insertResult.second) {
        apzc = insertResult.first->second;
      }
      APZCTM_LOG("Found APZC %p for layer %p with identifiers %lld %lld\n", apzc, aLayer, guid.mLayersId, guid.mScrollId);

      
      
      
      if (apzc == nullptr) {
        apzc = aLayer->GetAsyncPanZoomController();

        
        
        
        
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
        APZCTM_LOG("Using APZC %p for layer %p with identifiers %lld %lld\n", apzc, aLayer, aLayersId, metrics.GetScrollId());

        apzc->NotifyLayersUpdated(metrics,
                                  aIsFirstPaint && (aLayersId == aOriginatingLayersId));
        apzc->SetScrollHandoffParentId(aLayer->GetScrollHandoffParentId());

        
        
        
        
        ParentLayerRect visible(metrics.mCompositionBounds);
        CSSRect touchSensitiveRegion;
        if (state->mController->GetTouchSensitiveRegion(&touchSensitiveRegion)) {
          
          
          
          visible = visible.Intersect(touchSensitiveRegion
                                      * metrics.mDevPixelsPerCSSPixel
                                      * metrics.GetParentResolution());
        }

        
        
        
        ParentLayerIntRect roundedVisible = RoundedIn(visible);
        nsIntRegion unobscured;
        unobscured.Sub(nsIntRect(roundedVisible.x, roundedVisible.y,
                                 roundedVisible.width, roundedVisible.height),
                       aObscured);

        apzc->SetLayerHitTestData(unobscured, aTransform, transform);
        APZCTM_LOG("Setting rect(%f %f %f %f) as visible region for APZC %p\n", visible.x, visible.y,
                                                                              visible.width, visible.height,
                                                                              apzc);

        mApzcTreeLog << "APZC " << guid
                     << "\tcb=" << visible
                     << "\tsr=" << metrics.mScrollableRect
                     << (aLayer->GetVisibleRegion().IsEmpty() ? "\tscrollinfo" : "")
                     << (apzc->HasScrollgrab() ? "\tscrollgrab" : "")
                     << "\t" << aLayer->GetContentDescription();

        
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

        insertResult.first->second = apzc;
      }
    }
  }

  aLayer->SetAsyncPanZoomController(apzc);

  mApzcTreeLog << '\n';

  
  
  if (apzc) {
    aTransform = Matrix4x4();
  } else {
    
    aTransform = transform * aTransform;
  }

  uint64_t childLayersId = (aLayer->AsRefLayer() ? aLayer->AsRefLayer()->GetReferentId() : aLayersId);

  nsIntRegion obscured;
  if (aLayersId == childLayersId) {
    
    
    
    
    
    
    
    
    
    
    
    obscured = aObscured;
    obscured.Transform(To3DMatrix(transform).Inverse());
  }

  
  
  AsyncPanZoomController* next = apzc ? nullptr : aNextSibling;
  for (Layer* child = aLayer->GetLastChild(); child; child = child->GetPrevSibling()) {
    gfx::TreeAutoIndent indent(mApzcTreeLog);
    next = UpdatePanZoomControllerTree(aCompositor, child, childLayersId, aTransform, aParent, next,
                                       aIsFirstPaint, aOriginatingLayersId,
                                       aPaintLogger, aApzcsToDestroy, aApzcMap, obscured);

    
    
    nsIntRegion childRegion = child->GetVisibleRegion();
    childRegion.Transform(gfx::To3DMatrix(child->GetTransform()));
    if (child->GetClipRect()) {
      childRegion.AndWith(*child->GetClipRect());
    }

    obscured.OrWith(childRegion);
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
ApplyTransform(gfx::PointTyped<T>* aPoint, const Matrix4x4& aMatrix)
{
  Point result = aMatrix * aPoint->ToUnknownPoint();
  *aPoint = ViewAs<T>(result);
}

 template<class T> void
ApplyTransform(gfx::IntPointTyped<T>* aPoint, const Matrix4x4& aMatrix)
{
  Point result = aMatrix * aPoint->ToUnknownPoint();
  *aPoint = TruncatedToInt(ViewAs<T>(result));
}

 void
ApplyTransform(nsIntPoint* aPoint, const Matrix4x4& aMatrix)
{
  Point result = aMatrix * Point(aPoint->x, aPoint->y);
  aPoint->x = NS_lround(result.x);
  aPoint->y = NS_lround(result.y);
}

 template<class T> void
TransformScreenToGecko(T* aPoint, AsyncPanZoomController* aApzc, APZCTreeManager* aApzcTm)
{
  Matrix4x4 transformToApzc, transformToGecko;
  aApzcTm->GetInputTransforms(aApzc, transformToApzc, transformToGecko);
  ApplyTransform(aPoint, transformToApzc * transformToGecko);
}

nsEventStatus
APZCTreeManager::ReceiveInputEvent(InputData& aEvent,
                                   ScrollableLayerGuid* aOutTargetGuid)
{
  nsEventStatus result = nsEventStatus_eIgnore;
  Matrix4x4 transformToApzc;
  Matrix4x4 transformToGecko;
  bool inOverscrolledApzc = false;
  switch (aEvent.mInputType) {
    case MULTITOUCH_INPUT: {
      MultiTouchInput& touchInput = aEvent.AsMultiTouchInput();
      result = ProcessTouchInput(touchInput, aOutTargetGuid);
      break;
    } case PANGESTURE_INPUT: {
      PanGestureInput& panInput = aEvent.AsPanGestureInput();
      nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(panInput.mPanStartPoint,
                                                            &inOverscrolledApzc);
      if (apzc) {
        
        
        
        PanGestureInput inputForApzc(panInput);
        GetInputTransforms(apzc, transformToApzc, transformToGecko);
        ApplyTransform(&(inputForApzc.mPanStartPoint), transformToApzc);
        result = apzc->ReceiveInputEvent(inputForApzc);

        
        apzc->GetGuid(aOutTargetGuid);
        TransformScreenToGecko(&(panInput.mPanStartPoint), apzc, this);
      }
      break;
    } case PINCHGESTURE_INPUT: {
      PinchGestureInput& pinchInput = aEvent.AsPinchGestureInput();
      nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(pinchInput.mFocusPoint,
                                                            &inOverscrolledApzc);
      if (apzc) {
        
        
        
        PinchGestureInput inputForApzc(pinchInput);
        GetInputTransforms(apzc, transformToApzc, transformToGecko);
        ApplyTransform(&(inputForApzc.mFocusPoint), transformToApzc);
        result = apzc->ReceiveInputEvent(inputForApzc);

        
        apzc->GetGuid(aOutTargetGuid);
        TransformScreenToGecko(&(pinchInput.mFocusPoint), apzc, this);
      }
      break;
    } case TAPGESTURE_INPUT: {
      TapGestureInput& tapInput = aEvent.AsTapGestureInput();
      nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(ScreenPoint(tapInput.mPoint),
                                                            &inOverscrolledApzc);
      if (apzc) {
        
        
        
        TapGestureInput inputForApzc(tapInput);
        GetInputTransforms(apzc, transformToApzc, transformToGecko);
        ApplyTransform(&(inputForApzc.mPoint), transformToApzc);
        result = apzc->ReceiveInputEvent(inputForApzc);

        
        apzc->GetGuid(aOutTargetGuid);
        TransformScreenToGecko(&(tapInput.mPoint), apzc, this);
      }
      break;
    }
  }
  if (inOverscrolledApzc) {
    result = nsEventStatus_eConsumeNoDefault;
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
      
      Matrix4x4 transformToGecko;
      GetInputTransforms(mApzcForInputBlock, mCachedTransformToApzcForInputBlock, transformToGecko);
    } else {
      
      mCachedTransformToApzcForInputBlock = Matrix4x4();
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

  nsEventStatus result = nsEventStatus_eIgnore;
  if (mApzcForInputBlock) {
    mApzcForInputBlock->GetGuid(aOutTargetGuid);
    
    
    
    Matrix4x4 transformToApzc = mCachedTransformToApzcForInputBlock;
    MultiTouchInput inputForApzc(aInput);
    for (size_t i = 0; i < inputForApzc.mTouches.Length(); i++) {
      ApplyTransform(&(inputForApzc.mTouches[i].mScreenPoint), transformToApzc);
    }
    result = mApzcForInputBlock->ReceiveInputEvent(inputForApzc);

    
    
    
    Matrix4x4 transformToGecko;
    GetInputTransforms(mApzcForInputBlock, transformToApzc, transformToGecko);
    Matrix4x4 outTransform = transformToApzc * transformToGecko;
    for (size_t i = 0; i < aInput.mTouches.Length(); i++) {
      ApplyTransform(&(aInput.mTouches[i].mScreenPoint), outTransform);
    }
  }
  if (mInOverscrolledApzc) {
    result = nsEventStatus_eConsumeNoDefault;
  }

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
    Matrix4x4 transformToApzc;
    Matrix4x4 transformToGecko;
    GetInputTransforms(apzc, transformToApzc, transformToGecko);
    Matrix4x4 outTransform = transformToApzc * transformToGecko;
    *aOutTransformedPoint = TransformTo<LayoutDevicePixel>(outTransform, aPoint);
  }
}

nsEventStatus
APZCTreeManager::ProcessEvent(WidgetInputEvent& aEvent,
                              ScrollableLayerGuid* aOutTargetGuid)
{
  MOZ_ASSERT(NS_IsMainThread());
  nsEventStatus result = nsEventStatus_eIgnore;

  
  
  bool inOverscrolledApzc = false;
  nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(ScreenPoint(aEvent.refPoint.x, aEvent.refPoint.y),
                                                        &inOverscrolledApzc);
  if (apzc) {
    apzc->GetGuid(aOutTargetGuid);
    Matrix4x4 transformToApzc;
    Matrix4x4 transformToGecko;
    GetInputTransforms(apzc, transformToApzc, transformToGecko);
    Matrix4x4 outTransform = transformToApzc * transformToGecko;
    ApplyTransform(&(aEvent.refPoint), outTransform);
  }
  if (inOverscrolledApzc) {
    result = nsEventStatus_eConsumeNoDefault;
  }
  return result;
}

nsEventStatus
APZCTreeManager::ReceiveInputEvent(WidgetInputEvent& aEvent,
                                   ScrollableLayerGuid* aOutTargetGuid)
{
  
  
  

  MOZ_ASSERT(NS_IsMainThread());

  switch (aEvent.mClass) {
    case eTouchEventClass: {
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
  Matrix4x4 transformToApzc;
  Matrix4x4 transformToGecko;  

  
  aTreeManager->GetInputTransforms(aSource, transformToApzc, transformToGecko);
  Matrix4x4 untransformToApzc = transformToApzc;
  untransformToApzc.Invert();
  ApplyTransform(&aStartPoint, untransformToApzc);
  ApplyTransform(&aEndPoint, untransformToApzc);

  
  aTreeManager->GetInputTransforms(aTarget, transformToApzc, transformToGecko);
  ApplyTransform(&aStartPoint, transformToApzc);
  ApplyTransform(&aEndPoint, transformToApzc);
}

bool
APZCTreeManager::DispatchScroll(AsyncPanZoomController* aPrev,
                                ScreenPoint aStartPoint,
                                ScreenPoint aEndPoint,
                                const OverscrollHandoffChain& aOverscrollHandoffChain,
                                uint32_t aOverscrollHandoffChainIndex)
{
  nsRefPtr<AsyncPanZoomController> next;
  
  
  if (aOverscrollHandoffChainIndex >= aOverscrollHandoffChain.Length()) {
    
    return false;
  }

  next = aOverscrollHandoffChain.GetApzcAtIndex(aOverscrollHandoffChainIndex);

  if (next == nullptr || next->IsDestroyed()) {
    return false;
  }

  if (next->GetGuid().mLayersId != aPrev->GetGuid().mLayersId) {
    NS_WARNING("Handing off scroll across a layer tree boundary; may need to revise approach to bug 1031067");
  }

  
  
  
  
  
  if (next != aPrev) {
    TransformDisplacement(this, aPrev, next, aStartPoint, aEndPoint);
  }

  
  
  return next->AttemptScroll(aStartPoint, aEndPoint, aOverscrollHandoffChain,
      aOverscrollHandoffChainIndex);
}

bool
APZCTreeManager::HandOffFling(AsyncPanZoomController* aPrev, ScreenPoint aVelocity,
                              nsRefPtr<const OverscrollHandoffChain> aOverscrollHandoffChain)
{
  
  uint32_t i;
  for (i = 0; i < aOverscrollHandoffChain->Length(); ++i) {
    if (aOverscrollHandoffChain->GetApzcAtIndex(i) == aPrev) {
      break;
    }
  }

  
  nsRefPtr<AsyncPanZoomController> next;
  if (i + 1 < aOverscrollHandoffChain->Length()) {
    next = aOverscrollHandoffChain->GetApzcAtIndex(i + 1);
  }

  
  if (next == nullptr || next->IsDestroyed()) {
    return false;
  }

  
  
  
  
  
  
  
  ScreenPoint startPoint;  
  ScreenPoint endPoint = startPoint + aVelocity;
  TransformDisplacement(this, aPrev, next, startPoint, endPoint);
  ScreenPoint transformedVelocity = endPoint - startPoint;

  
  return next->TakeOverFling(transformedVelocity, aOverscrollHandoffChain);
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

already_AddRefed<AsyncPanZoomController>
APZCTreeManager::GetTargetAPZC(const ScreenPoint& aPoint, bool* aOutInOverscrolledApzc)
{
  MonitorAutoLock lock(mTreeLock);
  nsRefPtr<AsyncPanZoomController> target;
  
  gfxPoint point(aPoint.x, aPoint.y);
  bool inOverscrolledApzc = false;
  for (AsyncPanZoomController* apzc = mRootApzc; apzc; apzc = apzc->GetPrevSibling()) {
    target = GetAPZCAtPoint(apzc, point, &inOverscrolledApzc);
    
    
    if (target || inOverscrolledApzc) {
      break;
    }
  }
  
  MOZ_ASSERT(!(target && inOverscrolledApzc));
  if (aOutInOverscrolledApzc) {
    *aOutInOverscrolledApzc = inOverscrolledApzc;
  }
  return target.forget();
}

nsRefPtr<const OverscrollHandoffChain>
APZCTreeManager::BuildOverscrollHandoffChain(const nsRefPtr<AsyncPanZoomController>& aInitialTarget)
{
  
  
  
  
  
  
  
  
  

  
  MonitorAutoLock lock(mTreeLock);

  
  
  
  
  OverscrollHandoffChain* result = new OverscrollHandoffChain;
  AsyncPanZoomController* apzc = aInitialTarget;
  while (apzc != nullptr) {
    result->Add(apzc);

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

  
  
  
  result->SortByScrollPriority();

  
  for (uint32_t i = 0; i < result->Length(); ++i) {
    APZCTM_LOG("OverscrollHandoffChain[%d] = %p\n", i, result->GetApzcAtIndex(i).get());
  }

  return result;
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

  
  
  
  

  
  
  
  
  Matrix4x4 ancestorUntransform = aApzc->GetAncestorTransform();
  ancestorUntransform.Invert();

  
  
  
  gfxPointH3D hitTestPointForThisLayer = To3DMatrix(ancestorUntransform).ProjectPoint(aHitTestPoint);
  APZCTM_LOG("Untransformed %f %f to transient coordinates %f %f for hit-testing APZC %p\n",
           aHitTestPoint.x, aHitTestPoint.y,
           hitTestPointForThisLayer.x, hitTestPointForThisLayer.y, aApzc);

  
  
  
  
  Matrix4x4 cssUntransform = aApzc->GetCSSTransform();
  cssUntransform.Invert();
  Matrix4x4 asyncUntransform = aApzc->GetCurrentAsyncTransform();
  asyncUntransform.Invert();
  Matrix4x4 childUntransform = ancestorUntransform * cssUntransform * asyncUntransform;
  gfxPointH3D hitTestPointForChildLayers = To3DMatrix(childUntransform).ProjectPoint(aHitTestPoint);
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
APZCTreeManager::GetInputTransforms(AsyncPanZoomController *aApzc, Matrix4x4& aTransformToApzcOut,
                                    Matrix4x4& aTransformToGeckoOut)
{
  MonitorAutoLock lock(mTreeLock);

  
  
  
  
  

  
  Matrix4x4 ancestorUntransform = aApzc->GetAncestorTransform();
  ancestorUntransform.Invert();
  
  Matrix4x4 asyncUntransform = aApzc->GetCurrentAsyncTransform();
  asyncUntransform.Invert();
  
  Matrix4x4 nontransientAsyncTransform = aApzc->GetNontransientAsyncTransform();
  
  Matrix4x4 transientAsyncUntransform = nontransientAsyncTransform * asyncUntransform;

  
  Matrix4x4 cssUntransform = aApzc->GetCSSTransform();
  cssUntransform.Invert();
  Matrix4x4 nontransientAsyncUntransform = nontransientAsyncTransform;
  nontransientAsyncUntransform.Invert();
  aTransformToApzcOut = ancestorUntransform * cssUntransform * nontransientAsyncUntransform;
  
  aTransformToGeckoOut = transientAsyncUntransform * aApzc->GetTransformToLastDispatchedPaint() * aApzc->GetCSSTransform() * aApzc->GetAncestorTransform();

  for (AsyncPanZoomController* parent = aApzc->GetParent(); parent; parent = parent->GetParent()) {
    
    ancestorUntransform = parent->GetAncestorTransform();
    ancestorUntransform.Invert();
    
    asyncUntransform = parent->GetCurrentAsyncTransform();
    asyncUntransform.Invert();
    
    cssUntransform = parent->GetCSSTransform();
    cssUntransform.Invert();
    Matrix4x4 untransformSinceLastApzc = ancestorUntransform * cssUntransform * asyncUntransform;

    
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
