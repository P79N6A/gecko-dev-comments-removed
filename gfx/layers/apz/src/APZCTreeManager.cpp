




#include "APZCTreeManager.h"
#include "AsyncPanZoomController.h"
#include "Compositor.h"                 
#include "HitTestingTreeNode.h"         
#include "InputBlockState.h"            
#include "InputData.h"                  
#include "Layers.h"                     
#include "mozilla/dom/Touch.h"          
#include "mozilla/gfx/Point.h"          
#include "mozilla/layers/APZThreadUtils.h"  
#include "mozilla/layers/AsyncCompositionManager.h" 
#include "mozilla/layers/CompositorParent.h" 
#include "mozilla/layers/LayerMetricsWrapper.h"
#include "mozilla/MouseEvents.h"
#include "mozilla/mozalloc.h"           
#include "mozilla/TouchEvents.h"
#include "mozilla/Preferences.h"        
#include "mozilla/EventStateManager.h"  
#include "nsDebug.h"                    
#include "nsPoint.h"                    
#include "nsThreadUtils.h"              
#include "mozilla/gfx/Logging.h"        
#include "UnitTransforms.h"             
#include "gfxPrefs.h"                   
#include "OverscrollHandoffState.h"     
#include "LayersLogging.h"              

#define ENABLE_APZCTM_LOGGING 0


#if ENABLE_APZCTM_LOGGING
#  define APZCTM_LOG(...) printf_stderr("APZCTM: " __VA_ARGS__)
#else
#  define APZCTM_LOG(...)
#endif

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

  

  
  
  
  nsTArray<nsRefPtr<HitTestingTreeNode>> mNodesToDestroy;

  
  
  
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
      mHitResultForInputBlock(HitNothing),
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

AsyncPanZoomController*
APZCTreeManager::MakeAPZCInstance(uint64_t aLayersId,
                                  GeckoContentController* aController)
{
  return new AsyncPanZoomController(aLayersId, this, mInputQueue,
    aController, AsyncPanZoomController::USE_GESTURE_DETECTOR);
}

void
APZCTreeManager::SetAllowedTouchBehavior(uint64_t aInputBlockId,
                                         const nsTArray<TouchBehaviorFlags> &aValues)
{
  mInputQueue->SetAllowedTouchBehavior(aInputBlockId, aValues);
}


static void
Collect(HitTestingTreeNode* aNode, nsTArray<nsRefPtr<HitTestingTreeNode>>* aCollection)
{
  if (aNode) {
    aCollection->AppendElement(aNode);
    Collect(aNode->GetLastChild(), aCollection);
    Collect(aNode->GetPrevSibling(), aCollection);
  }
}

void
APZCTreeManager::UpdateHitTestingTree(CompositorParent* aCompositor,
                                      Layer* aRoot,
                                      bool aIsFirstPaint,
                                      uint64_t aOriginatingLayersId,
                                      uint32_t aPaintSequenceNumber)
{
  APZThreadUtils::AssertOnCompositorThread();

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

  
  
  
  
  
  
  
  
  
  
  
  
  Collect(mRootNode, &state.mNodesToDestroy);
  mRootNode = nullptr;

  if (aRoot) {
    mApzcTreeLog << "[start]\n";
    LayerMetricsWrapper root(aRoot);
    UpdateHitTestingTree(state, root,
                         
                         aCompositor ? aCompositor->RootLayerTreeId() : 0,
                         Matrix4x4(), nullptr, nullptr);
    mApzcTreeLog << "[end]\n";
  }

  for (size_t i = 0; i < state.mNodesToDestroy.Length(); i++) {
    APZCTM_LOG("Destroying node at %p with APZC %p\n",
        state.mNodesToDestroy[i].get(),
        state.mNodesToDestroy[i]->GetApzc());
    state.mNodesToDestroy[i]->Destroy();
  }

#if ENABLE_APZCTM_LOGGING
  
  printf_stderr("APZCTreeManager (%p)\n", this);
  mRootNode->Dump("  ");
#endif
}



static ParentLayerIntRegion
ComputeClipRegion(GeckoContentController* aController,
                  const LayerMetricsWrapper& aLayer)
{
  ParentLayerIntRegion clipRegion;
  if (aLayer.GetClipRect()) {
    clipRegion = ViewAs<ParentLayerPixel>(*aLayer.GetClipRect());
  } else {
    
    
    
    
    clipRegion = RoundedToInt(aLayer.Metrics().mCompositionBounds);
  }

  
  
  
  CSSRect touchSensitiveRegion;
  if (aController->GetTouchSensitiveRegion(&touchSensitiveRegion)) {
    
    
    
    
    
    
    LayoutDeviceToParentLayerScale2D parentCumulativeResolution =
          aLayer.Metrics().GetCumulativeResolution()
        / ParentLayerToLayerScale(aLayer.Metrics().GetPresShellResolution());
    
    
    
    ParentLayerIntRegion extraClip = RoundedIn(
        touchSensitiveRegion
        * aLayer.Metrics().GetDevPixelsPerCSSPixel()
        * parentCumulativeResolution);
    clipRegion.AndWith(extraClip);
  }

  return clipRegion;
}

void
APZCTreeManager::PrintAPZCInfo(const LayerMetricsWrapper& aLayer,
                               const AsyncPanZoomController* apzc)
{
  const FrameMetrics& metrics = aLayer.Metrics();
  mApzcTreeLog << "APZC " << apzc->GetGuid() << "\tcb=" << metrics.mCompositionBounds
               << "\tsr=" << metrics.GetScrollableRect()
               << (aLayer.IsScrollInfoLayer() ? "\tscrollinfo" : "")
               << (apzc->HasScrollgrab() ? "\tscrollgrab" : "") << "\t"
               << metrics.GetContentDescription().get();
}

void
APZCTreeManager::AttachNodeToTree(HitTestingTreeNode* aNode,
                                  HitTestingTreeNode* aParent,
                                  HitTestingTreeNode* aNextSibling)
{
  if (aNextSibling) {
    aNextSibling->SetPrevSibling(aNode);
  } else if (aParent) {
    aParent->SetLastChild(aNode);
  } else {
    MOZ_ASSERT(!mRootNode);
    mRootNode = aNode;
    aNode->MakeRoot();
  }
}

static EventRegions
GetEventRegions(const LayerMetricsWrapper& aLayer)
{
  if (aLayer.IsScrollInfoLayer()) {
    return EventRegions(nsIntRegion(ParentLayerIntRect::ToUntyped(
      RoundedToInt(aLayer.Metrics().mCompositionBounds))));
  }
  return aLayer.GetEventRegions();
}

already_AddRefed<HitTestingTreeNode>
APZCTreeManager::RecycleOrCreateNode(TreeBuildingState& aState,
                                     AsyncPanZoomController* aApzc)
{
  
  
  
  for (size_t i = 0; i < aState.mNodesToDestroy.Length(); i++) {
    nsRefPtr<HitTestingTreeNode> node = aState.mNodesToDestroy[i];
    if (!node->IsPrimaryHolder()) {
      aState.mNodesToDestroy.RemoveElement(node);
      node->RecycleWith(aApzc);
      return node.forget();
    }
  }
  nsRefPtr<HitTestingTreeNode> node = new HitTestingTreeNode(aApzc, false);
  return node.forget();
}

static EventRegionsOverride
GetEventRegionsOverride(HitTestingTreeNode* aParent,
                       const LayerMetricsWrapper& aLayer)
{
  
  
  
  
  EventRegionsOverride result = aLayer.GetEventRegionsOverride();
  if (aParent) {
    result |= aParent->GetEventRegionsOverride();
  }
  return result;
}

HitTestingTreeNode*
APZCTreeManager::PrepareNodeForLayer(const LayerMetricsWrapper& aLayer,
                                     const FrameMetrics& aMetrics,
                                     uint64_t aLayersId,
                                     const gfx::Matrix4x4& aAncestorTransform,
                                     HitTestingTreeNode* aParent,
                                     HitTestingTreeNode* aNextSibling,
                                     TreeBuildingState& aState)
{
  bool needsApzc = true;
  if (!aMetrics.IsScrollable()) {
    needsApzc = false;
  }

  const CompositorParent::LayerTreeState* state = CompositorParent::GetIndirectShadowTree(aLayersId);
  if (!(state && state->mController.get())) {
    needsApzc = false;
  }

  nsRefPtr<HitTestingTreeNode> node = nullptr;
  if (!needsApzc) {
    node = RecycleOrCreateNode(aState, nullptr);
    AttachNodeToTree(node, aParent, aNextSibling);
    node->SetHitTestData(GetEventRegions(aLayer), aLayer.GetTransform(),
        aLayer.GetClipRect() ? Some(ParentLayerIntRegion(ViewAs<ParentLayerPixel>(*aLayer.GetClipRect()))) : Nothing(),
        GetEventRegionsOverride(aParent, aLayer));
    return node;
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

    
    
    
    
    
    
    
    if (apzc && (!apzc->Matches(guid) || !apzc->HasTreeManager(this))) {
      apzc = nullptr;
    }

    
    
    
    
    
    
    
    
    
    
    
    
    for (size_t i = 0; i < aState.mNodesToDestroy.Length(); i++) {
      nsRefPtr<HitTestingTreeNode> n = aState.mNodesToDestroy[i];
      if (n->IsPrimaryHolder() && n->GetApzc() && n->GetApzc()->Matches(guid)) {
        node = n;
        if (apzc != nullptr) {
          
          
          MOZ_ASSERT(apzc == node->GetApzc());
        }
        apzc = node->GetApzc();
        break;
      }
    }

    
    
    
    
    bool newApzc = (apzc == nullptr || apzc->IsDestroyed());
    if (newApzc) {
      apzc = MakeAPZCInstance(aLayersId, state->mController);
      apzc->SetCompositorParent(aState.mCompositor);
      if (state->mCrossProcessParent != nullptr) {
        apzc->ShareFrameMetricsAcrossProcesses();
      }
      MOZ_ASSERT(node == nullptr);
      node = new HitTestingTreeNode(apzc, true);
    } else {
      
      
      
      
      
      aState.mNodesToDestroy.RemoveElement(node);
      node->SetPrevSibling(nullptr);
      node->SetLastChild(nullptr);
    }

    APZCTM_LOG("Using APZC %p for layer %p with identifiers %" PRId64 " %" PRId64 "\n", apzc, aLayer.GetLayer(), aLayersId, aMetrics.GetScrollId());

    apzc->NotifyLayersUpdated(aMetrics,
        aState.mIsFirstPaint && (aLayersId == aState.mOriginatingLayersId));

    
    
    
    MOZ_ASSERT(node->IsPrimaryHolder() && node->GetApzc() && node->GetApzc()->Matches(guid));

    ParentLayerIntRegion clipRegion = ComputeClipRegion(state->mController, aLayer);
    node->SetHitTestData(GetEventRegions(aLayer), aLayer.GetTransform(), Some(clipRegion),
        GetEventRegionsOverride(aParent, aLayer));
    apzc->SetAncestorTransform(aAncestorTransform);

    PrintAPZCInfo(aLayer, apzc);

    
    AttachNodeToTree(node, aParent, aNextSibling);

    
    
    
    
    
    
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
    
    
    

    node = RecycleOrCreateNode(aState, apzc);
    AttachNodeToTree(node, aParent, aNextSibling);

    
    
    
    
    MOZ_ASSERT(aAncestorTransform == apzc->GetAncestorTransform());

    ParentLayerIntRegion clipRegion = ComputeClipRegion(state->mController, aLayer);
    node->SetHitTestData(GetEventRegions(aLayer), aLayer.GetTransform(), Some(clipRegion),
        GetEventRegionsOverride(aParent, aLayer));
  }

  return node;
}

HitTestingTreeNode*
APZCTreeManager::UpdateHitTestingTree(TreeBuildingState& aState,
                                      const LayerMetricsWrapper& aLayer,
                                      uint64_t aLayersId,
                                      const gfx::Matrix4x4& aAncestorTransform,
                                      HitTestingTreeNode* aParent,
                                      HitTestingTreeNode* aNextSibling)
{
  mTreeLock.AssertCurrentThreadOwns();

  mApzcTreeLog << aLayer.Name() << '\t';

  HitTestingTreeNode* node = PrepareNodeForLayer(aLayer,
        aLayer.Metrics(), aLayersId, aAncestorTransform,
        aParent, aNextSibling, aState);
  MOZ_ASSERT(node);
  AsyncPanZoomController* apzc = node->GetApzc();
  aLayer.SetApzc(apzc);

  mApzcTreeLog << '\n';

  
  
  
  
  
  
  
  Matrix4x4 transform = aLayer.GetTransform();
  Matrix4x4 ancestorTransform = transform;
  if (!apzc) {
    ancestorTransform = ancestorTransform * aAncestorTransform;
  }

  
  
  MOZ_ASSERT(!node->GetFirstChild());
  aParent = node;
  HitTestingTreeNode* next = nullptr;

  uint64_t childLayersId = (aLayer.AsRefLayer() ? aLayer.AsRefLayer()->GetReferentId() : aLayersId);
  for (LayerMetricsWrapper child = aLayer.GetLastChild(); child; child = child.GetPrevSibling()) {
    gfx::TreeAutoIndent indent(mApzcTreeLog);
    next = UpdateHitTestingTree(aState, child, childLayersId,
                                ancestorTransform, aParent, next);
  }

  return node;
}

nsEventStatus
APZCTreeManager::ReceiveInputEvent(InputData& aEvent,
                                   ScrollableLayerGuid* aOutTargetGuid,
                                   uint64_t* aOutInputBlockId)
{
  APZThreadUtils::AssertOnControllerThread();

  
  
  if (aOutInputBlockId) {
    *aOutInputBlockId = InputBlockState::NO_BLOCK_ID;
  }
  nsEventStatus result = nsEventStatus_eIgnore;
  HitTestResult hitResult = HitNothing;
  switch (aEvent.mInputType) {
    case MULTITOUCH_INPUT: {
      MultiTouchInput& touchInput = aEvent.AsMultiTouchInput();
      result = ProcessTouchInput(touchInput, aOutTargetGuid, aOutInputBlockId);
      break;
    } case SCROLLWHEEL_INPUT: {
      ScrollWheelInput& wheelInput = aEvent.AsScrollWheelInput();
      nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(wheelInput.mOrigin,
                                                            &hitResult);
      if (apzc) {
        MOZ_ASSERT(hitResult == HitLayer || hitResult == HitDispatchToContentRegion);

        result = mInputQueue->ReceiveInputEvent(
          apzc,
           hitResult == HitLayer,
          wheelInput, aOutInputBlockId);

        
        apzc->GetGuid(aOutTargetGuid);
        Matrix4x4 transformToGecko = GetScreenToApzcTransform(apzc)
                                   * GetApzcToGeckoTransform(apzc);
        wheelInput.mOrigin =
          TransformTo<ScreenPixel>(transformToGecko, wheelInput.mOrigin);
      }
      break;
    } case PANGESTURE_INPUT: {
      PanGestureInput& panInput = aEvent.AsPanGestureInput();
      nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(panInput.mPanStartPoint,
                                                            &hitResult);
      if (apzc) {
        MOZ_ASSERT(hitResult == HitLayer || hitResult == HitDispatchToContentRegion);

        result = mInputQueue->ReceiveInputEvent(
            apzc,
             hitResult == HitLayer,
            panInput, aOutInputBlockId);

        
        apzc->GetGuid(aOutTargetGuid);
        Matrix4x4 transformToGecko = GetScreenToApzcTransform(apzc)
                                   * GetApzcToGeckoTransform(apzc);
        panInput.mPanStartPoint = TransformTo<ScreenPixel>(
            transformToGecko, panInput.mPanStartPoint);
        panInput.mPanDisplacement = TransformVector<ScreenPixel>(
            transformToGecko, panInput.mPanDisplacement, panInput.mPanStartPoint);
      }
      break;
    } case PINCHGESTURE_INPUT: {  
      PinchGestureInput& pinchInput = aEvent.AsPinchGestureInput();
      nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(pinchInput.mFocusPoint,
                                                            &hitResult);
      if (apzc) {
        MOZ_ASSERT(hitResult == HitLayer || hitResult == HitDispatchToContentRegion);

        result = mInputQueue->ReceiveInputEvent(
            apzc,
             hitResult == HitLayer,
            pinchInput, aOutInputBlockId);

        
        apzc->GetGuid(aOutTargetGuid);
        Matrix4x4 outTransform = GetScreenToApzcTransform(apzc)
                               * GetApzcToGeckoTransform(apzc);
        pinchInput.mFocusPoint = TransformTo<ScreenPixel>(
            outTransform, pinchInput.mFocusPoint);
      }
      break;
    } case TAPGESTURE_INPUT: {  
      TapGestureInput& tapInput = aEvent.AsTapGestureInput();
      nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(tapInput.mPoint,
                                                            &hitResult);
      if (apzc) {
        MOZ_ASSERT(hitResult == HitLayer || hitResult == HitDispatchToContentRegion);

        result = mInputQueue->ReceiveInputEvent(
            apzc,
             hitResult == HitLayer,
            tapInput, aOutInputBlockId);

        
        apzc->GetGuid(aOutTargetGuid);
        Matrix4x4 outTransform = GetScreenToApzcTransform(apzc)
                               * GetApzcToGeckoTransform(apzc);
        tapInput.mPoint = TransformTo<ScreenPixel>(outTransform, tapInput.mPoint);
      }
      break;
    }
  }
  return result;
}

already_AddRefed<AsyncPanZoomController>
APZCTreeManager::GetTouchInputBlockAPZC(const MultiTouchInput& aEvent,
                                        HitTestResult* aOutHitResult)
{
  nsRefPtr<AsyncPanZoomController> apzc;
  if (aEvent.mTouches.Length() == 0) {
    return apzc.forget();
  }

  { 
    
    
    
    
    
    
    
    MonitorAutoLock lock(mTreeLock);
    FlushRepaintsRecursively(mRootNode);
  }

  apzc = GetTargetAPZC(aEvent.mTouches[0].mScreenPoint, aOutHitResult);
  for (size_t i = 1; i < aEvent.mTouches.Length(); i++) {
    nsRefPtr<AsyncPanZoomController> apzc2 = GetTargetAPZC(aEvent.mTouches[i].mScreenPoint, aOutHitResult);
    apzc = GetMultitouchTarget(apzc, apzc2);
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
    mHitResultForInputBlock = HitNothing;
    nsRefPtr<AsyncPanZoomController> apzc = GetTouchInputBlockAPZC(aInput, &mHitResultForInputBlock);
    
    
    
    
    
    if (apzc != mApzcForInputBlock) {
      
      
      
      if (mApzcForInputBlock) {
        MultiTouchInput cancel(MultiTouchInput::MULTITOUCH_CANCEL, 0, TimeStamp::Now(), 0);
        mInputQueue->ReceiveInputEvent(mApzcForInputBlock,
             true, cancel, nullptr);
      }
      mApzcForInputBlock = apzc;
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
    MOZ_ASSERT(mHitResultForInputBlock == HitLayer || mHitResultForInputBlock == HitDispatchToContentRegion);

    mApzcForInputBlock->GetGuid(aOutTargetGuid);
    result = mInputQueue->ReceiveInputEvent(mApzcForInputBlock,
         mHitResultForInputBlock == HitLayer,
        aInput, aOutInputBlockId);

    
    
    
    Matrix4x4 transformToApzc = GetScreenToApzcTransform(mApzcForInputBlock);
    Matrix4x4 transformToGecko = GetApzcToGeckoTransform(mApzcForInputBlock);
    Matrix4x4 outTransform = transformToApzc * transformToGecko;
    for (size_t i = 0; i < aInput.mTouches.Length(); i++) {
      SingleTouchData& touchData = aInput.mTouches[i];
      touchData.mScreenPoint = TransformTo<ScreenPixel>(
          outTransform, touchData.mScreenPoint);
    }
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
    mHitResultForInputBlock = HitNothing;
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

void
APZCTreeManager::UpdateWheelTransaction(WidgetInputEvent& aEvent)
{
  WheelBlockState* txn = mInputQueue->GetCurrentWheelTransaction();
  if (!txn) {
    return;
  }

  
  
  if (txn->MaybeTimeout(TimeStamp::Now())) {
    return;
  }

  switch (aEvent.message) {
   case NS_MOUSE_MOVE:
   case NS_DRAGDROP_OVER: {
     WidgetMouseEvent* mouseEvent = aEvent.AsMouseEvent();
     if (!mouseEvent->IsReal()) {
       return;
     }

     ScreenIntPoint point =
       ViewAs<ScreenPixel>(aEvent.refPoint, PixelCastJustification::LayoutDeviceToScreenForUntransformedEvent);
     txn->OnMouseMove(point);
     return;
   }
   case NS_KEY_PRESS:
   case NS_KEY_UP:
   case NS_KEY_DOWN:
   case NS_MOUSE_BUTTON_UP:
   case NS_MOUSE_BUTTON_DOWN:
   case NS_MOUSE_DOUBLECLICK:
   case NS_MOUSE_CLICK:
   case NS_CONTEXTMENU:
   case NS_DRAGDROP_DROP:
     txn->EndTransaction();
     return;
  }
}

nsEventStatus
APZCTreeManager::ProcessEvent(WidgetInputEvent& aEvent,
                              ScrollableLayerGuid* aOutTargetGuid,
                              uint64_t* aOutInputBlockId)
{
  MOZ_ASSERT(NS_IsMainThread());
  nsEventStatus result = nsEventStatus_eIgnore;

  
  UpdateWheelTransaction(aEvent);

  
  
  HitTestResult hitResult = HitNothing;
  nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(ScreenPoint(aEvent.refPoint.x, aEvent.refPoint.y),
                                                        &hitResult);
  if (apzc) {
    MOZ_ASSERT(hitResult == HitLayer || hitResult == HitDispatchToContentRegion);
    apzc->GetGuid(aOutTargetGuid);
    Matrix4x4 transformToApzc = GetScreenToApzcTransform(apzc);
    Matrix4x4 transformToGecko = GetApzcToGeckoTransform(apzc);
    Matrix4x4 outTransform = transformToApzc * transformToGecko;
    aEvent.refPoint = TransformTo<LayoutDevicePixel>(outTransform, aEvent.refPoint);
  }
  return result;
}

nsEventStatus
APZCTreeManager::ProcessWheelEvent(WidgetWheelEvent& aEvent,
                                   ScrollableLayerGuid* aOutTargetGuid,
                                   uint64_t* aOutInputBlockId)
{
  ScrollWheelInput::ScrollMode scrollMode = ScrollWheelInput::SCROLLMODE_INSTANT;
  if (gfxPrefs::SmoothScrollEnabled() && gfxPrefs::WheelSmoothScrollEnabled()) {
    scrollMode = ScrollWheelInput::SCROLLMODE_SMOOTH;
  }

  ScreenPoint origin(aEvent.refPoint.x, aEvent.refPoint.y);
  ScrollWheelInput input(aEvent.time, aEvent.timeStamp, 0,
                         scrollMode,
                         ScrollWheelInput::SCROLLDELTA_LINE,
                         origin,
                         aEvent.deltaX,
                         aEvent.deltaY);

  nsEventStatus status = ReceiveInputEvent(input, aOutTargetGuid, aOutInputBlockId);
  aEvent.refPoint.x = input.mOrigin.x;
  aEvent.refPoint.y = input.mOrigin.y;
  return status;
}

bool
APZCTreeManager::WillHandleWheelEvent(WidgetWheelEvent* aEvent)
{
  return EventStateManager::WheelEventIsScrollAction(aEvent) &&
         aEvent->deltaMode == nsIDOMWheelEvent::DOM_DELTA_LINE &&
         !EventStateManager::WheelEventNeedsDeltaMultipliers(aEvent);
}

nsEventStatus
APZCTreeManager::ReceiveInputEvent(WidgetInputEvent& aEvent,
                                   ScrollableLayerGuid* aOutTargetGuid,
                                   uint64_t* aOutInputBlockId)
{
  
  
  
  

  MOZ_ASSERT(NS_IsMainThread());
  APZThreadUtils::AssertOnControllerThread();

  
  
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
    case eWheelEventClass: {
      WidgetWheelEvent& wheelEvent = *aEvent.AsWheelEvent();
      if (!WillHandleWheelEvent(&wheelEvent)) {
        
        
        return ProcessEvent(aEvent, aOutTargetGuid, aOutInputBlockId);
      }
      return ProcessWheelEvent(wheelEvent, aOutTargetGuid, aOutInputBlockId);
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
APZCTreeManager::ContentReceivedInputBlock(uint64_t aInputBlockId, bool aPreventDefault)
{
  APZThreadUtils::AssertOnControllerThread();

  mInputQueue->ContentReceivedInputBlock(aInputBlockId, aPreventDefault);
}

void
APZCTreeManager::SetTargetAPZC(uint64_t aInputBlockId,
                               const nsTArray<ScrollableLayerGuid>& aTargets)
{
  APZThreadUtils::AssertOnControllerThread();

  nsRefPtr<AsyncPanZoomController> target = nullptr;
  if (aTargets.Length() > 0) {
    target = GetTargetAPZC(aTargets[0]);
  }
  for (size_t i = 1; i < aTargets.Length(); i++) {
    nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(aTargets[i]);
    target = GetMultitouchTarget(target, apzc);
  }
  mInputQueue->SetConfirmedTargetApzc(aInputBlockId, target);
}

void
APZCTreeManager::SetTargetAPZC(uint64_t aInputBlockId, const ScrollableLayerGuid& aTarget)
{
  APZThreadUtils::AssertOnControllerThread();

  nsRefPtr<AsyncPanZoomController> apzc = GetTargetAPZC(aTarget);
  mInputQueue->SetConfirmedTargetApzc(aInputBlockId, apzc);
}

void
APZCTreeManager::UpdateZoomConstraints(const ScrollableLayerGuid& aGuid,
                                       const ZoomConstraints& aConstraints)
{
  MonitorAutoLock lock(mTreeLock);
  nsRefPtr<HitTestingTreeNode> node = GetTargetNode(aGuid, nullptr);
  MOZ_ASSERT(!node || node->GetApzc()); 

  
  
  if (node && node->GetApzc()->IsRootForLayersId()) {
    UpdateZoomConstraintsRecursively(node.get(), aConstraints);
  }
}

void
APZCTreeManager::UpdateZoomConstraintsRecursively(HitTestingTreeNode* aNode,
                                                  const ZoomConstraints& aConstraints)
{
  mTreeLock.AssertCurrentThreadOwns();

  if (aNode->IsPrimaryHolder()) {
    MOZ_ASSERT(aNode->GetApzc());
    aNode->GetApzc()->UpdateZoomConstraints(aConstraints);
  }
  for (HitTestingTreeNode* child = aNode->GetLastChild(); child; child = child->GetPrevSibling()) {
    
    if (child->GetApzc() && child->GetApzc()->IsRootForLayersId()) {
      continue;
    }
    UpdateZoomConstraintsRecursively(child, aConstraints);
  }
}

void
APZCTreeManager::FlushRepaintsRecursively(HitTestingTreeNode* aNode)
{
  mTreeLock.AssertCurrentThreadOwns();

  for (HitTestingTreeNode* node = aNode; node; node = node->GetPrevSibling()) {
    if (node->IsPrimaryHolder()) {
      MOZ_ASSERT(node->GetApzc());
      node->GetApzc()->FlushRepaintForNewInputBlock();
    }
    FlushRepaintsRecursively(node->GetLastChild());
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
  
  
  
  APZThreadUtils::RunOnControllerThread(NewRunnableMethod(
    mInputQueue.get(), &InputQueue::Clear));

  MonitorAutoLock lock(mTreeLock);

  
  
  
  nsTArray<nsRefPtr<HitTestingTreeNode>> nodesToDestroy;
  Collect(mRootNode, &nodesToDestroy);
  for (size_t i = 0; i < nodesToDestroy.Length(); i++) {
    nodesToDestroy[i]->Destroy();
  }
  mRootNode = nullptr;
}

nsRefPtr<HitTestingTreeNode>
APZCTreeManager::GetRootNode() const
{
  MonitorAutoLock lock(mTreeLock);
  return mRootNode;
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
  nsRefPtr<HitTestingTreeNode> node = GetTargetNode(aGuid, nullptr);
  MOZ_ASSERT(!node || node->GetApzc()); 
  nsRefPtr<AsyncPanZoomController> apzc = node ? node->GetApzc() : nullptr;
  return apzc.forget();
}

already_AddRefed<HitTestingTreeNode>
APZCTreeManager::GetTargetNode(const ScrollableLayerGuid& aGuid,
                               GuidComparator aComparator)
{
  mTreeLock.AssertCurrentThreadOwns();
  nsRefPtr<HitTestingTreeNode> target = FindTargetNode(mRootNode, aGuid, aComparator);
  return target.forget();
}

already_AddRefed<AsyncPanZoomController>
APZCTreeManager::GetTargetAPZC(const ScreenPoint& aPoint, HitTestResult* aOutHitResult)
{
  MonitorAutoLock lock(mTreeLock);
  HitTestResult hitResult = HitNothing;
  ParentLayerPoint point = ViewAs<ParentLayerPixel>(aPoint,
    PixelCastJustification::ScreenIsParentLayerForRoot);
  nsRefPtr<AsyncPanZoomController> target = GetAPZCAtPoint(mRootNode, point, &hitResult);

  if (aOutHitResult) {
    *aOutHitResult = hitResult;
  }
  return target.forget();
}

static bool
GuidComparatorIgnoringPresShell(const ScrollableLayerGuid& aOne, const ScrollableLayerGuid& aTwo)
{
  return aOne.mLayersId == aTwo.mLayersId
      && aOne.mScrollId == aTwo.mScrollId;
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
      ScrollableLayerGuid guid(parent->GetGuid().mLayersId, 0, apzc->GetScrollHandoffParentId());
      nsRefPtr<HitTestingTreeNode> node = GetTargetNode(guid, &GuidComparatorIgnoringPresShell);
      MOZ_ASSERT(!node || node->GetApzc()); 
      scrollParent = node ? node->GetApzc() : nullptr;
    }
    apzc = scrollParent;
  }

  
  
  
  result->SortByScrollPriority();

  
  for (uint32_t i = 0; i < result->Length(); ++i) {
    APZCTM_LOG("OverscrollHandoffChain[%d] = %p\n", i, result->GetApzcAtIndex(i).get());
  }

  return result;
}

HitTestingTreeNode*
APZCTreeManager::FindTargetNode(HitTestingTreeNode* aNode,
                                const ScrollableLayerGuid& aGuid,
                                GuidComparator aComparator)
{
  mTreeLock.AssertCurrentThreadOwns();

  
  
  for (HitTestingTreeNode* node = aNode; node; node = node->GetPrevSibling()) {
    HitTestingTreeNode* match = FindTargetNode(node->GetLastChild(), aGuid, aComparator);
    if (match) {
      return match;
    }

    bool matches = false;
    if (node->GetApzc()) {
      if (aComparator) {
        matches = aComparator(aGuid, node->GetApzc()->GetGuid());
      } else {
        matches = node->GetApzc()->Matches(aGuid);
      }
    }
    if (matches) {
      return node;
    }
  }
  return nullptr;
}

AsyncPanZoomController*
APZCTreeManager::GetAPZCAtPoint(HitTestingTreeNode* aNode,
                                const ParentLayerPoint& aHitTestPoint,
                                HitTestResult* aOutHitResult)
{
  mTreeLock.AssertCurrentThreadOwns();

  
  
  for (HitTestingTreeNode* node = aNode; node; node = node->GetPrevSibling()) {
    if (node->IsOutsideClip(aHitTestPoint)) {
      
      
      
      APZCTM_LOG("Point %f %f outside clip for node %p\n",
        aHitTestPoint.x, aHitTestPoint.y, node);
      continue;
    }

    AsyncPanZoomController* result = nullptr;

    
    
    Maybe<LayerPoint> hitTestPointForChildLayers = node->Untransform(aHitTestPoint);
    if (hitTestPointForChildLayers) {
      ParentLayerPoint childPoint = ViewAs<ParentLayerPixel>(hitTestPointForChildLayers.ref(),
        PixelCastJustification::MovingDownToChildren);
      result = GetAPZCAtPoint(node->GetLastChild(), childPoint, aOutHitResult);
    }

    
    if (*aOutHitResult == HitNothing) {
      APZCTM_LOG("Testing ParentLayer point %s (Layer %s) against node %p\n",
          Stringify(aHitTestPoint).c_str(),
          hitTestPointForChildLayers ? Stringify(hitTestPointForChildLayers.ref()).c_str() : "nil",
          node);
      HitTestResult hitResult = node->HitTest(aHitTestPoint);
      if (hitResult != HitTestResult::HitNothing) {
        result = node->GetNearestContainingApzc();
        APZCTM_LOG("Successfully matched APZC %p via node %p (hit result %d)\n",
             result, node, hitResult);
        MOZ_ASSERT(hitResult == HitLayer || hitResult == HitDispatchToContentRegion);
        
        *aOutHitResult = hitResult;
      }
    }

    if (*aOutHitResult != HitNothing) {
      return result;
    }
  }

  return nullptr;
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
    
    Matrix4x4 asyncUntransform = parent->GetCurrentAsyncTransformWithOverscroll().Inverse();
    
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

  
  
  
  
  

  
  Matrix4x4 asyncUntransform = aApzc->GetCurrentAsyncTransformWithOverscroll().Inverse();

  
  result = asyncUntransform * aApzc->GetTransformToLastDispatchedPaint() * aApzc->GetAncestorTransform();

  for (AsyncPanZoomController* parent = aApzc->GetParent(); parent; parent = parent->GetParent()) {
    
    result = result * parent->GetTransformToLastDispatchedPaint() * parent->GetAncestorTransform();

    
    
    
  }

  return result;
}

already_AddRefed<AsyncPanZoomController>
APZCTreeManager::GetMultitouchTarget(AsyncPanZoomController* aApzc1, AsyncPanZoomController* aApzc2) const
{
  nsRefPtr<AsyncPanZoomController> apzc = CommonAncestor(aApzc1, aApzc2);
  
  
  apzc = RootAPZCForLayersId(apzc);
  return apzc.forget();
}

already_AddRefed<AsyncPanZoomController>
APZCTreeManager::CommonAncestor(AsyncPanZoomController* aApzc1, AsyncPanZoomController* aApzc2) const
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
APZCTreeManager::RootAPZCForLayersId(AsyncPanZoomController* aApzc) const
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
