




#include "APZCTreeManager.h"
#include "AsyncPanZoomController.h"
#include "Compositor.h"                 
#include "InputBlockState.h"            
#include "InputData.h"                  
#include "Layers.h"                     
#include "mozilla/dom/Touch.h"          
#include "mozilla/gfx/Point.h"          
#include "mozilla/layers/AsyncCompositionManager.h" 
#include "mozilla/layers/CompositorParent.h" 
#include "mozilla/layers/LayerMetricsWrapper.h"
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
#include "OverscrollHandoffState.h"     
#include "LayersLogging.h"              

#define APZCTM_LOG(...)


namespace mozilla {
namespace layers {

typedef mozilla::gfx::Point Point;
typedef mozilla::gfx::Point4D Point4D;
typedef mozilla::gfx::Matrix4x4 Matrix4x4;

float APZCTreeManager::sDPI = 160.0;

struct APZCTreeManager::TreeBuildingState {
  TreeBuildingState(CompositorParent* aCompositor,
                    bool aIsFirstPaint, uint64_t aOriginatingLayersId,
                    APZTestData* aTestData, uint32_t aPaintSequence)
    : mCompositor(aCompositor)
    , mIsFirstPaint(aIsFirstPaint)
    , mOriginatingLayersId(aOriginatingLayersId)
    , mPaintLogger(aTestData, aPaintSequence)
  {
  }

  
  CompositorParent* const mCompositor;
  const bool mIsFirstPaint;
  const uint64_t mOriginatingLayersId;
  const APZPaintLogHelper mPaintLogger;

  
  nsTArray< nsRefPtr<AsyncPanZoomController> > mApzcsToDestroy;
  std::map<ScrollableLayerGuid, AsyncPanZoomController*> mApzcMap;
};

 const ScreenMargin
APZCTreeManager::CalculatePendingDisplayPort(
  const FrameMetrics& aFrameMetrics,
  const ParentLayerPoint& aVelocity,
  double aEstimatedPaintDuration)
{
  return AsyncPanZoomController::CalculatePendingDisplayPort(
    aFrameMetrics, aVelocity, aEstimatedPaintDuration);
}

APZCTreeManager::APZCTreeManager()
    : mInputQueue(new InputQueue()),
      mTreeLock("APZCTreeLock"),
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
APZCTreeManager::SetAllowedTouchBehavior(uint64_t aInputBlockId,
                                         const nsTArray<TouchBehaviorFlags> &aValues)
{
  mInputQueue->SetAllowedTouchBehavior(aInputBlockId, aValues);
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

  
  
  APZTestData* testData = nullptr;
  if (gfxPrefs::APZTestLoggingEnabled()) {
    if (CompositorParent::LayerTreeState* state = CompositorParent::GetIndirectShadowTree(aOriginatingLayersId)) {
      testData = &state->mApzTestData;
      testData->StartNewPaint(aPaintSequenceNumber);
    }
  }

  TreeBuildingState state(aCompositor, aIsFirstPaint, aOriginatingLayersId,
                          testData, aPaintSequenceNumber);

  
  
  
  
  
  
  
  
  
  
  
  
  Collect(mRootApzc, &state.mApzcsToDestroy);
  mRootApzc = nullptr;

  if (aRoot) {
    mApzcTreeLog << "[start]\n";
    LayerMetricsWrapper root(aRoot);
    UpdatePanZoomControllerTree(state, root,
                                
                                aCompositor ? aCompositor->RootLayerTreeId() : 0,
                                Matrix4x4(), nullptr, nullptr, nsIntRegion());
    mApzcTreeLog << "[end]\n";
  }

  for (size_t i = 0; i < state.mApzcsToDestroy.Length(); i++) {
    APZCTM_LOG("Destroying APZC at %p\n", state.mApzcsToDestroy[i].get());
    state.mApzcsToDestroy[i]->Destroy();
  }
}

static nsIntRegion
ComputeTouchSensitiveRegion(GeckoContentController* aController,
                            const FrameMetrics& aMetrics,
                            const nsIntRegion& aObscured)
{
  
  
  
  
  ParentLayerRect visible(aMetrics.mCompositionBounds);
  CSSRect touchSensitiveRegion;
  if (aController->GetTouchSensitiveRegion(&touchSensitiveRegion)) {
    
    
    
    
    
    
    LayoutDeviceToParentLayerScale parentCumulativeResolution =
          aMetrics.mCumulativeResolution
        / ParentLayerToLayerScale(aMetrics.mPresShellResolution);
    visible = visible.Intersect(touchSensitiveRegion
                                * aMetrics.mDevPixelsPerCSSPixel
                                * parentCumulativeResolution);
  }

  
  
  
  ParentLayerIntRect roundedVisible = RoundedIn(visible);
  nsIntRegion unobscured;
  unobscured.Sub(nsIntRect(roundedVisible.x, roundedVisible.y,
                           roundedVisible.width, roundedVisible.height),
                 aObscured);
  return unobscured;
}

void
APZCTreeManager::PrintAPZCInfo(const LayerMetricsWrapper& aLayer,
                               const AsyncPanZoomController* apzc)
{
  const FrameMetrics& metrics = aLayer.Metrics();
  mApzcTreeLog << "APZC " << apzc->GetGuid() << "\tcb=" << metrics.mCompositionBounds
               << "\tsr=" << metrics.mScrollableRect
               << (aLayer.IsScrollInfoLayer() ? "\tscrollinfo" : "")
               << (apzc->HasScrollgrab() ? "\tscrollgrab" : "") << "\t"
               << metrics.GetContentDescription().get();
}

AsyncPanZoomController*
APZCTreeManager::PrepareAPZCForLayer(const LayerMetricsWrapper& aLayer,
                                     const FrameMetrics& aMetrics,
                                     uint64_t aLayersId,
                                     const gfx::Matrix4x4& aAncestorTransform,
                                     const nsIntRegion& aObscured,
                                     AsyncPanZoomController* aParent,
                                     AsyncPanZoomController* aNextSibling,
                                     TreeBuildingState& aState)
{
  if (!aMetrics.IsScrollable()) {
    return nullptr;
  }

  const CompositorParent::LayerTreeState* state = CompositorParent::GetIndirectShadowTree(aLayersId);
  if (!(state && state->mController.get())) {
    return nullptr;
  }

  AsyncPanZoomController* apzc = nullptr;
  
  
  

  
  
  
  
  ScrollableLayerGuid guid(aLayersId, aMetrics);
  auto insertResult = aState.mApzcMap.insert(std::make_pair(guid, static_cast<AsyncPanZoomController*>(nullptr)));
  if (!insertResult.second) {
    apzc = insertResult.first->second;
    PrintAPZCInfo(aLayer, apzc);
  }
  APZCTM_LOG("Found APZC %p for layer %p with identifiers %" PRId64 " %" PRId64 "\n", apzc, aLayer.GetLayer(), guid.mLayersId, guid.mScrollId);

  
  
  
  if (apzc == nullptr) {
    apzc = aLayer.GetApzc();

    
    
    
    
    if (apzc && !apzc->Matches(guid)) {
      apzc = nullptr;
    }

    
    
    
    
    
    
    if (apzc == nullptr) {
      for (size_t i = 0; i < aState.mApzcsToDestroy.Length(); i++) {
        if (aState.mApzcsToDestroy.ElementAt(i)->Matches(guid)) {
          apzc = aState.mApzcsToDestroy.ElementAt(i);
          break;
        }
      }
    }

    
    
    
    
    bool newApzc = (apzc == nullptr || apzc->IsDestroyed());
    if (newApzc) {
      apzc = new AsyncPanZoomController(aLayersId, this, mInputQueue, state->mController,
                                        AsyncPanZoomController::USE_GESTURE_DETECTOR);
      apzc->SetCompositorParent(aState.mCompositor);
      if (state->mCrossProcessParent != nullptr) {
        apzc->ShareFrameMetricsAcrossProcesses();
      }
    } else {
      
      
      
      
      
      aState.mApzcsToDestroy.RemoveElement(apzc);
      apzc->SetPrevSibling(nullptr);
      apzc->SetLastChild(nullptr);
    }
    APZCTM_LOG("Using APZC %p for layer %p with identifiers %" PRId64 " %" PRId64 "\n", apzc, aLayer.GetLayer(), aLayersId, aMetrics.GetScrollId());

    apzc->NotifyLayersUpdated(aMetrics,
        aState.mIsFirstPaint && (aLayersId == aState.mOriginatingLayersId));

    nsIntRegion unobscured = ComputeTouchSensitiveRegion(state->mController, aMetrics, aObscured);
    apzc->SetLayerHitTestData(unobscured, aAncestorTransform);
    APZCTM_LOG("Setting region %s as visible region for APZC %p\n",
        Stringify(unobscured).c_str(), apzc);

    PrintAPZCInfo(aLayer, apzc);

    
    if (aNextSibling) {
      aNextSibling->SetPrevSibling(apzc);
    } else if (aParent) {
      aParent->SetLastChild(apzc);
    } else {
      MOZ_ASSERT(!mRootApzc);
      mRootApzc = apzc;
      apzc->MakeRoot();
    }

    
    
    
    
    
    
    if (aLayersId == aState.mOriginatingLayersId && apzc->GetParent()) {
      aState.mPaintLogger.LogTestData(aMetrics.GetScrollId(), "parentScrollId",
          apzc->GetParent()->GetGuid().mScrollId);
    }

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
  } else {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    nsIntRegion unobscured = ComputeTouchSensitiveRegion(state->mController, aMetrics, aObscured);
    apzc->AddHitTestRegion(unobscured);
    APZCTM_LOG("Adding region %s to visible region of APZC %p\n", Stringify(unobscured).c_str(), apzc);
  }

  return apzc;
}

AsyncPanZoomController*
APZCTreeManager::UpdatePanZoomControllerTree(TreeBuildingState& aState,
                                             const LayerMetricsWrapper& aLayer,
                                             uint64_t aLayersId,
                                             const gfx::Matrix4x4& aAncestorTransform,
                                             AsyncPanZoomController* aParent,
                                             AsyncPanZoomController* aNextSibling,
                                             const nsIntRegion& aObscured)
{
  mTreeLock.AssertCurrentThreadOwns();

  mApzcTreeLog << aLayer.Name() << '\t';

  AsyncPanZoomController* apzc = PrepareAPZCForLayer(aLayer,
        aLayer.Metrics(), aLayersId, aAncestorTransform,
        aObscured, aParent, aNextSibling, aState);
  aLayer.SetApzc(apzc);

  mApzcTreeLog << '\n';

  
  
  
  
  
  
  
  Matrix4x4 transform = aLayer.GetTransform();
  Matrix4x4 ancestorTransform = transform;
  if (!apzc) {
    ancestorTransform = ancestorTransform * aAncestorTransform;
  }

  uint64_t childLayersId = (aLayer.AsRefLayer() ? aLayer.AsRefLayer()->GetReferentId() : aLayersId);

  nsIntRegion obscured;
  if (aLayersId == childLayersId) {
    
    
    
    
    
    
    
    
    
    
    
    obscured = aObscured;
    obscured.Transform(To3DMatrix(transform).Inverse());
  }

  
  
  AsyncPanZoomController* next = aNextSibling;
  if (apzc) {
    
    
    aParent = apzc;
    next = apzc->GetFirstChild();
  }

  for (LayerMetricsWrapper child = aLayer.GetLastChild(); child; child = child.GetPrevSibling()) {
    gfx::TreeAutoIndent indent(mApzcTreeLog);
    next = UpdatePanZoomControllerTree(aState, child, childLayersId,
                                       ancestorTransform, aParent, next,
                                       obscured);

    
    
    nsIntRegion childRegion = child.GetVisibleRegion();
    childRegion.Transform(gfx::To3DMatrix(child.GetTransform()));
    if (child.GetClipRect()) {
      childRegion.AndWith(*child.GetClipRect());
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

nsEventStatus
APZCTreeManager::ReceiveInputEvent(InputData& aEvent,
                                   ScrollableLayerGuid* aOutTargetGuid,
                                   uint64_t* aOutInputBlockId)
{
  
  
  if (aOutInputBlockId) {
    *aOutInputBlockId = InputBlockState::NO_BLOCK_ID;
  }
  nsEventStatus result = nsEventStatus_eIgnore;
  Matrix4x4 transformToApzc;
  bool inOverscrolledApzc = false;
  switch (aEvent.mInputType) {
    case MULTITOUCH_INPUT: {
      MultiTouchInput& touchInput = aEvent.AsMultiTouchInput();
      result = ProcessTouchInput(touchInput, aOutTargetGuid, aOutInputBlockId);
      break;
    } case PANGESTURE_INPUT: {
      PanGestureInput& panInput = aEvent.AsPanGestureInput();
      nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(panInput.mPanStartPoint,
                                                            &inOverscrolledApzc);
      if (apzc) {
        transformToApzc = GetScreenToApzcTransform(apzc);
        panInput.mLocalPanStartPoint = TransformTo<ParentLayerPixel>(
            transformToApzc, panInput.mPanStartPoint);
        panInput.mLocalPanDisplacement = TransformVector<ParentLayerPixel>(
            transformToApzc, panInput.mPanDisplacement, panInput.mPanStartPoint);
        result = mInputQueue->ReceiveInputEvent(apzc, panInput, aOutInputBlockId);

        
        apzc->GetGuid(aOutTargetGuid);
        Matrix4x4 transformToGecko = transformToApzc * GetApzcToGeckoTransform(apzc);
        panInput.mPanStartPoint = TransformTo<ScreenPixel>(
            transformToGecko, panInput.mPanStartPoint);
        panInput.mPanDisplacement = TransformVector<ScreenPixel>(
            transformToGecko, panInput.mPanDisplacement, panInput.mPanStartPoint);
      }
      break;
    } case PINCHGESTURE_INPUT: {  
      PinchGestureInput& pinchInput = aEvent.AsPinchGestureInput();
      nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(pinchInput.mFocusPoint,
                                                            &inOverscrolledApzc);
      if (apzc) {
        transformToApzc = GetScreenToApzcTransform(apzc);
        pinchInput.mLocalFocusPoint = TransformTo<ParentLayerPixel>(
            transformToApzc, pinchInput.mFocusPoint);
        result = mInputQueue->ReceiveInputEvent(apzc, pinchInput, aOutInputBlockId);

        
        apzc->GetGuid(aOutTargetGuid);
        Matrix4x4 outTransform = transformToApzc * GetApzcToGeckoTransform(apzc);
        pinchInput.mFocusPoint = TransformTo<ScreenPixel>(
            outTransform, pinchInput.mFocusPoint);
      }
      break;
    } case TAPGESTURE_INPUT: {  
      TapGestureInput& tapInput = aEvent.AsTapGestureInput();
      nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(tapInput.mPoint,
                                                            &inOverscrolledApzc);
      if (apzc) {
        transformToApzc = GetScreenToApzcTransform(apzc);
        tapInput.mLocalPoint = TransformTo<ParentLayerPixel>(
            transformToApzc, tapInput.mPoint);
        result = mInputQueue->ReceiveInputEvent(apzc, tapInput, aOutInputBlockId);

        
        apzc->GetGuid(aOutTargetGuid);
        Matrix4x4 outTransform = transformToApzc * GetApzcToGeckoTransform(apzc);
        tapInput.mPoint = TransformTo<ScreenPixel>(outTransform, tapInput.mPoint);
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

  { 
    
    
    
    
    
    
    
    MonitorAutoLock lock(mTreeLock);
    for (AsyncPanZoomController* apzc = mRootApzc; apzc; apzc = apzc->GetPrevSibling()) {
      FlushRepaintsRecursively(apzc);
    }
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
                                   ScrollableLayerGuid* aOutTargetGuid,
                                   uint64_t* aOutInputBlockId)
{
  if (aInput.mType == MultiTouchInput::MULTITOUCH_START) {
    
    
    
    
    if (mApzcForInputBlock && BuildOverscrollHandoffChain(mApzcForInputBlock)->HasOverscrolledApzc()) {
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
        mInputQueue->ReceiveInputEvent(mApzcForInputBlock, cancel, nullptr);
      }
      mApzcForInputBlock = apzc;
    }

    if (mApzcForInputBlock) {
      
      mCachedTransformToApzcForInputBlock = GetScreenToApzcTransform(mApzcForInputBlock);
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
    for (size_t i = 0; i < aInput.mTouches.Length(); i++) {
      SingleTouchData& touchData = aInput.mTouches[i];
      touchData.mLocalScreenPoint = TransformTo<ParentLayerPixel>(
          transformToApzc, ScreenPoint(touchData.mScreenPoint));
    }
    result = mInputQueue->ReceiveInputEvent(mApzcForInputBlock, aInput, aOutInputBlockId);

    
    
    
    transformToApzc = GetScreenToApzcTransform(mApzcForInputBlock);
    Matrix4x4 transformToGecko = GetApzcToGeckoTransform(mApzcForInputBlock);
    Matrix4x4 outTransform = transformToApzc * transformToGecko;
    for (size_t i = 0; i < aInput.mTouches.Length(); i++) {
      SingleTouchData& touchData = aInput.mTouches[i];
      touchData.mScreenPoint = TransformTo<ScreenPixel>(
          outTransform, touchData.mScreenPoint);
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
    Matrix4x4 transformToApzc = GetScreenToApzcTransform(apzc);
    Matrix4x4 transformToGecko = GetApzcToGeckoTransform(apzc);
    Matrix4x4 outTransform = transformToApzc * transformToGecko;
    *aOutTransformedPoint = TransformTo<LayoutDevicePixel>(outTransform, aPoint);
  }
}

nsEventStatus
APZCTreeManager::ProcessEvent(WidgetInputEvent& aEvent,
                              ScrollableLayerGuid* aOutTargetGuid,
                              uint64_t* aOutInputBlockId)
{
  MOZ_ASSERT(NS_IsMainThread());
  nsEventStatus result = nsEventStatus_eIgnore;

  
  
  bool inOverscrolledApzc = false;
  nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(ScreenPoint(aEvent.refPoint.x, aEvent.refPoint.y),
                                                        &inOverscrolledApzc);
  if (apzc) {
    apzc->GetGuid(aOutTargetGuid);
    Matrix4x4 transformToApzc = GetScreenToApzcTransform(apzc);
    Matrix4x4 transformToGecko = GetApzcToGeckoTransform(apzc);
    Matrix4x4 outTransform = transformToApzc * transformToGecko;
    aEvent.refPoint = TransformTo<LayoutDevicePixel>(outTransform, aEvent.refPoint);
  }
  if (inOverscrolledApzc) {
    result = nsEventStatus_eConsumeNoDefault;
  }
  return result;
}

nsEventStatus
APZCTreeManager::ReceiveInputEvent(WidgetInputEvent& aEvent,
                                   ScrollableLayerGuid* aOutTargetGuid,
                                   uint64_t* aOutInputBlockId)
{
  
  
  

  MOZ_ASSERT(NS_IsMainThread());

  
  
  if (aOutInputBlockId) {
    *aOutInputBlockId = InputBlockState::NO_BLOCK_ID;
  }

  switch (aEvent.mClass) {
    case eTouchEventClass: {
      WidgetTouchEvent& touchEvent = *aEvent.AsTouchEvent();
      MultiTouchInput touchInput(touchEvent);
      nsEventStatus result = ProcessTouchInput(touchInput, aOutTargetGuid, aOutInputBlockId);
      
      
      
      
      touchEvent.touches.Clear();
      touchEvent.touches.SetCapacity(touchInput.mTouches.Length());
      for (size_t i = 0; i < touchInput.mTouches.Length(); i++) {
        *touchEvent.touches.AppendElement() = touchInput.mTouches[i].ToNewDOMTouch();
      }
      return result;
    }
    default: {
      return ProcessEvent(aEvent, aOutTargetGuid, aOutInputBlockId);
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
APZCTreeManager::ContentReceivedTouch(uint64_t aInputBlockId,
                                      bool aPreventDefault)
{
  mInputQueue->ContentReceivedTouch(aInputBlockId, aPreventDefault);
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
APZCTreeManager::FlushRepaintsRecursively(AsyncPanZoomController* aApzc)
{
  mTreeLock.AssertCurrentThreadOwns();

  aApzc->FlushRepaintForNewInputBlock();
  for (AsyncPanZoomController* child = aApzc->GetLastChild(); child; child = child->GetPrevSibling()) {
    FlushRepaintsRecursively(child);
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
                      ParentLayerPoint& aStartPoint,
                      ParentLayerPoint& aEndPoint) {
  
  Matrix4x4 untransformToApzc = aTreeManager->GetScreenToApzcTransform(aSource).Inverse();
  ScreenPoint screenStart = TransformTo<ScreenPixel>(untransformToApzc, aStartPoint);
  ScreenPoint screenEnd = TransformTo<ScreenPixel>(untransformToApzc, aEndPoint);

  
  Matrix4x4 transformToApzc = aTreeManager->GetScreenToApzcTransform(aTarget);
  aStartPoint = TransformTo<ParentLayerPixel>(transformToApzc, screenStart);
  aEndPoint = TransformTo<ParentLayerPixel>(transformToApzc, screenEnd);
}

bool
APZCTreeManager::DispatchScroll(AsyncPanZoomController* aPrev,
                                ParentLayerPoint aStartPoint,
                                ParentLayerPoint aEndPoint,
                                OverscrollHandoffState& aOverscrollHandoffState)
{
  const OverscrollHandoffChain& overscrollHandoffChain = aOverscrollHandoffState.mChain;
  uint32_t overscrollHandoffChainIndex = aOverscrollHandoffState.mChainIndex;
  nsRefPtr<AsyncPanZoomController> next;
  
  
  if (overscrollHandoffChainIndex >= overscrollHandoffChain.Length()) {
    
    return false;
  }

  next = overscrollHandoffChain.GetApzcAtIndex(overscrollHandoffChainIndex);

  if (next == nullptr || next->IsDestroyed()) {
    return false;
  }

  
  
  
  
  
  if (next != aPrev) {
    TransformDisplacement(this, aPrev, next, aStartPoint, aEndPoint);
  }

  
  
  return next->AttemptScroll(aStartPoint, aEndPoint, aOverscrollHandoffState);
}

bool
APZCTreeManager::DispatchFling(AsyncPanZoomController* aPrev,
                               ParentLayerPoint aVelocity,
                               nsRefPtr<const OverscrollHandoffChain> aOverscrollHandoffChain,
                               bool aHandoff)
{
  nsRefPtr<AsyncPanZoomController> current;
  uint32_t aOverscrollHandoffChainLength = aOverscrollHandoffChain->Length();
  uint32_t startIndex;

  
  
  
  
  
  
  
  ParentLayerPoint startPoint;  
  ParentLayerPoint endPoint;
  ParentLayerPoint transformedVelocity = aVelocity;

  if (aHandoff) {
    startIndex = aOverscrollHandoffChain->IndexOf(aPrev) + 1;

    
    
    if (startIndex >= aOverscrollHandoffChainLength) {
      return false;
    }
  } else {
    startIndex = 0;
  }

  for (; startIndex < aOverscrollHandoffChainLength; startIndex++) {
    current = aOverscrollHandoffChain->GetApzcAtIndex(startIndex);

    
    if (current == nullptr || current->IsDestroyed()) {
      return false;
    }

    endPoint = startPoint + transformedVelocity;

    
    if (startIndex > 0) {
      TransformDisplacement(this,
                            aOverscrollHandoffChain->GetApzcAtIndex(startIndex - 1),
                            current,
                            startPoint,
                            endPoint);
    }

    transformedVelocity = endPoint - startPoint;

    bool handoff = (startIndex < 1) ? aHandoff : true;
    if (current->AttemptFling(transformedVelocity,
                              aOverscrollHandoffChain,
                              handoff)) {
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

already_AddRefed<AsyncPanZoomController>
APZCTreeManager::GetTargetAPZC(const ScreenPoint& aPoint, bool* aOutInOverscrolledApzc)
{
  MonitorAutoLock lock(mTreeLock);
  nsRefPtr<AsyncPanZoomController> target;
  
  bool inOverscrolledApzc = false;
  for (AsyncPanZoomController* apzc = mRootApzc; apzc; apzc = apzc->GetPrevSibling()) {
    target = GetAPZCAtPoint(apzc, aPoint.ToUnknownPoint(), &inOverscrolledApzc);
    
    
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
                                const Point& aHitTestPoint,
                                bool* aOutInOverscrolledApzc)
{
  mTreeLock.AssertCurrentThreadOwns();

  
  
  
  

  
  
  
  
  Matrix4x4 ancestorUntransform = aApzc->GetAncestorTransform().Inverse();

  
  
  
  Point4D hitTestPointForThisLayer = ancestorUntransform.ProjectPoint(aHitTestPoint);
  APZCTM_LOG("Untransformed %f %f to transient coordinates %f %f for hit-testing APZC %p\n",
           aHitTestPoint.x, aHitTestPoint.y,
           hitTestPointForThisLayer.x, hitTestPointForThisLayer.y, aApzc);

  
  
  
  
  Matrix4x4 childUntransform = ancestorUntransform * Matrix4x4(aApzc->GetCurrentAsyncTransform()).Inverse();
  Point4D hitTestPointForChildLayers = childUntransform.ProjectPoint(aHitTestPoint);
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
      aApzc->VisibleRegionContains(ParentLayerPoint::FromUnknownPoint(hitTestPointForThisLayer.As2DPoint()))) {
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































































































Matrix4x4
APZCTreeManager::GetScreenToApzcTransform(const AsyncPanZoomController *aApzc) const
{
  Matrix4x4 result;
  MonitorAutoLock lock(mTreeLock);

  
  
  
  
  

  
  Matrix4x4 ancestorUntransform = aApzc->GetAncestorTransform().Inverse();

  
  result = ancestorUntransform;

  for (AsyncPanZoomController* parent = aApzc->GetParent(); parent; parent = parent->GetParent()) {
    
    ancestorUntransform = parent->GetAncestorTransform().Inverse();
    
    Matrix4x4 asyncUntransform = Matrix4x4(parent->GetCurrentAsyncTransform()).Inverse();
    
    Matrix4x4 untransformSinceLastApzc = ancestorUntransform * asyncUntransform;

    
    result = untransformSinceLastApzc * result;

    
    
    
  }

  return result;
}





Matrix4x4
APZCTreeManager::GetApzcToGeckoTransform(const AsyncPanZoomController *aApzc) const
{
  Matrix4x4 result;
  MonitorAutoLock lock(mTreeLock);

  
  
  
  
  

  
  Matrix4x4 asyncUntransform = Matrix4x4(aApzc->GetCurrentAsyncTransform()).Inverse();

  
  result = asyncUntransform * aApzc->GetTransformToLastDispatchedPaint() * aApzc->GetAncestorTransform();

  for (AsyncPanZoomController* parent = aApzc->GetParent(); parent; parent = parent->GetParent()) {
    
    result = result * parent->GetTransformToLastDispatchedPaint() * parent->GetAncestorTransform();

    
    
    
  }

  return result;
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
