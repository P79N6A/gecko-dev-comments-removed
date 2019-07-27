



#include "ClientTiledPaintedLayer.h"
#include "FrameMetrics.h"               
#include "Units.h"                      
#include "UnitTransforms.h"             
#include "ClientLayerManager.h"         
#include "gfxPlatform.h"                
#include "gfxPrefs.h"                   
#include "gfxRect.h"                    
#include "mozilla/Assertions.h"         
#include "mozilla/gfx/BaseSize.h"       
#include "mozilla/gfx/Rect.h"           
#include "mozilla/layers/CompositorChild.h"
#include "mozilla/layers/LayerMetricsWrapper.h" 
#include "mozilla/layers/LayersMessages.h"
#include "mozilla/mozalloc.h"           
#include "nsISupportsImpl.h"            
#include "nsRect.h"                     
#include "LayersLogging.h"

namespace mozilla {
namespace layers {

ClientTiledPaintedLayer::ClientTiledPaintedLayer(ClientLayerManager* const aManager,
                                               ClientLayerManager::PaintedLayerCreationHint aCreationHint)
  : PaintedLayer(aManager, static_cast<ClientLayer*>(this), aCreationHint)
  , mContentClient()
{
  MOZ_COUNT_CTOR(ClientTiledPaintedLayer);
  mPaintData.mLastScrollOffset = ParentLayerPoint(0, 0);
  mPaintData.mFirstPaint = true;
}

ClientTiledPaintedLayer::~ClientTiledPaintedLayer()
{
  MOZ_COUNT_DTOR(ClientTiledPaintedLayer);
}

void
ClientTiledPaintedLayer::ClearCachedResources()
{
  if (mContentClient) {
    mContentClient->ClearCachedResources();
  }
  mValidRegion.SetEmpty();
  mContentClient = nullptr;
}

void
ClientTiledPaintedLayer::FillSpecificAttributes(SpecificLayerAttributes& aAttrs)
{
  aAttrs = PaintedLayerAttributes(GetValidRegion());
}

static LayerRect
ApplyParentLayerToLayerTransform(const gfx::Matrix4x4& aTransform, const ParentLayerRect& aParentLayerRect)
{
  return TransformTo<LayerPixel>(aTransform, aParentLayerRect);
}

static gfx::Matrix4x4
GetTransformToAncestorsParentLayer(Layer* aStart, const LayerMetricsWrapper& aAncestor)
{
  gfx::Matrix4x4 transform;
  const LayerMetricsWrapper& ancestorParent = aAncestor.GetParent();
  for (LayerMetricsWrapper iter(aStart, LayerMetricsWrapper::StartAt::BOTTOM);
       ancestorParent ? iter != ancestorParent : iter.IsValid();
       iter = iter.GetParent()) {
    transform = transform * iter.GetTransform();

    if (gfxPrefs::LayoutUseContainersForRootFrames()) {
      
      
      
      
      
      
      
      
      
      
      const FrameMetrics& metrics = iter.Metrics();
      transform.PostScale(metrics.GetPresShellResolution(), metrics.GetPresShellResolution(), 1.f);
    }
  }
  return transform;
}

void
ClientTiledPaintedLayer::GetAncestorLayers(LayerMetricsWrapper* aOutScrollAncestor,
                                           LayerMetricsWrapper* aOutDisplayPortAncestor,
                                           bool* aOutHasTransformAnimation)
{
  LayerMetricsWrapper scrollAncestor;
  LayerMetricsWrapper displayPortAncestor;
  bool hasTransformAnimation = false;
  for (LayerMetricsWrapper ancestor(this, LayerMetricsWrapper::StartAt::BOTTOM); ancestor; ancestor = ancestor.GetParent()) {
    hasTransformAnimation |= ancestor.HasTransformAnimation();
    const FrameMetrics& metrics = ancestor.Metrics();
    if (!scrollAncestor && metrics.GetScrollId() != FrameMetrics::NULL_SCROLL_ID) {
      scrollAncestor = ancestor;
    }
    if (!metrics.GetDisplayPort().IsEmpty()) {
      displayPortAncestor = ancestor;
      
      
      break;
    }
  }
  if (aOutScrollAncestor) {
    *aOutScrollAncestor = scrollAncestor;
  }
  if (aOutDisplayPortAncestor) {
    *aOutDisplayPortAncestor = displayPortAncestor;
  }
  if (aOutHasTransformAnimation) {
    *aOutHasTransformAnimation = hasTransformAnimation;
  }
}

void
ClientTiledPaintedLayer::BeginPaint()
{
  mPaintData.mLowPrecisionPaintCount = 0;
  mPaintData.mPaintFinished = false;
  mPaintData.mCompositionBounds.SetEmpty();
  mPaintData.mCriticalDisplayPort.SetEmpty();

  if (!GetBaseTransform().Is2D()) {
    
    
    
    return;
  }

  
  
  LayerMetricsWrapper scrollAncestor;
  LayerMetricsWrapper displayPortAncestor;
  bool hasTransformAnimation;
  GetAncestorLayers(&scrollAncestor, &displayPortAncestor, &hasTransformAnimation);

  if (!displayPortAncestor || !scrollAncestor) {
    
#if defined(MOZ_WIDGET_ANDROID) || defined(MOZ_B2G)
    
    
    NS_WARNING("Tiled PaintedLayer with no scrollable container ancestor");
#endif
    return;
  }

  TILING_LOG("TILING %p: Found scrollAncestor %p, displayPortAncestor %p, transform %d\n", this,
    scrollAncestor.GetLayer(), displayPortAncestor.GetLayer(), hasTransformAnimation);

  const FrameMetrics& scrollMetrics = scrollAncestor.Metrics();
  const FrameMetrics& displayportMetrics = displayPortAncestor.Metrics();

  
  
  gfx::Matrix4x4 transformDisplayPortToLayer =
    GetTransformToAncestorsParentLayer(this, displayPortAncestor);
  transformDisplayPortToLayer.Invert();

  
  
  
  
  
  if (!hasTransformAnimation) {
    ParentLayerRect criticalDisplayPort =
      (displayportMetrics.GetCriticalDisplayPort() * displayportMetrics.GetZoom())
      + displayportMetrics.mCompositionBounds.TopLeft();
    mPaintData.mCriticalDisplayPort = RoundedToInt(
      ApplyParentLayerToLayerTransform(transformDisplayPortToLayer, criticalDisplayPort));
  }
  TILING_LOG("TILING %p: Critical displayport %s\n", this, Stringify(mPaintData.mCriticalDisplayPort).c_str());

  
  
  mPaintData.mResolution = displayportMetrics.GetZoom();
  TILING_LOG("TILING %p: Resolution %s\n", this, Stringify(mPaintData.mResolution).c_str());

  
  mPaintData.mTransformToCompBounds =
    GetTransformToAncestorsParentLayer(this, scrollAncestor);
  gfx::Matrix4x4 transformToBounds = mPaintData.mTransformToCompBounds;
  transformToBounds.Invert();
  mPaintData.mCompositionBounds = ApplyParentLayerToLayerTransform(
    transformToBounds, scrollMetrics.mCompositionBounds);
  TILING_LOG("TILING %p: Composition bounds %s\n", this, Stringify(mPaintData.mCompositionBounds).c_str());

  
  mPaintData.mScrollOffset = displayportMetrics.GetScrollOffset() * displayportMetrics.GetZoom();
  TILING_LOG("TILING %p: Scroll offset %s\n", this, Stringify(mPaintData.mScrollOffset).c_str());
}

bool
ClientTiledPaintedLayer::IsScrollingOnCompositor(const FrameMetrics& aParentMetrics)
{
  CompositorChild* compositor = nullptr;
  if (Manager() && Manager()->AsClientLayerManager()) {
    compositor = Manager()->AsClientLayerManager()->GetCompositorChild();
  }

  if (!compositor) {
    return false;
  }

  FrameMetrics compositorMetrics;
  if (!compositor->LookupCompositorFrameMetrics(aParentMetrics.GetScrollId(),
                                                compositorMetrics)) {
    return false;
  }

  
  
  float COORDINATE_EPSILON = 1.f;

  return !FuzzyEqualsAdditive(compositorMetrics.GetScrollOffset().x,
                              aParentMetrics.GetScrollOffset().x,
                              COORDINATE_EPSILON) ||
         !FuzzyEqualsAdditive(compositorMetrics.GetScrollOffset().y,
                              aParentMetrics.GetScrollOffset().y,
                              COORDINATE_EPSILON);
}

bool
ClientTiledPaintedLayer::UseProgressiveDraw() {
  if (!gfxPlatform::GetPlatform()->UseProgressivePaint()) {
    
    return false;
  }

  if (ClientManager()->HasShadowTarget()) {
    
    
    
    return false;
  }

  if (mPaintData.mCriticalDisplayPort.IsEmpty()) {
    
    
    
    
    
    return false;
  }

  if (GetIsFixedPosition() || GetParent()->GetIsFixedPosition()) {
    
    
    
    return false;
  }

  if (gfxPrefs::AsyncPanZoomEnabled()) {
    LayerMetricsWrapper scrollAncestor;
    GetAncestorLayers(&scrollAncestor, nullptr, nullptr);
    MOZ_ASSERT(scrollAncestor); 
    const FrameMetrics& parentMetrics = scrollAncestor.Metrics();
    if (!IsScrollingOnCompositor(parentMetrics)) {
      return false;
    }
  }

  return true;
}

bool
ClientTiledPaintedLayer::RenderHighPrecision(nsIntRegion& aInvalidRegion,
                                            const nsIntRegion& aVisibleRegion,
                                            LayerManager::DrawPaintedLayerCallback aCallback,
                                            void* aCallbackData)
{
  
  
  if (aInvalidRegion.IsEmpty() || mPaintData.mLowPrecisionPaintCount != 0) {
    return false;
  }

  
  if (UseProgressiveDraw() &&
      mContentClient->mTiledBuffer.GetFrameResolution() == mPaintData.mResolution) {
    
    
    
    nsIntRegion oldValidRegion = mContentClient->mTiledBuffer.GetValidRegion();
    oldValidRegion.And(oldValidRegion, aVisibleRegion);
    if (!mPaintData.mCriticalDisplayPort.IsEmpty()) {
      oldValidRegion.And(oldValidRegion, LayerIntRect::ToUntyped(mPaintData.mCriticalDisplayPort));
    }

    TILING_LOG("TILING %p: Progressive update with old valid region %s\n", this, Stringify(oldValidRegion).c_str());

    return mContentClient->mTiledBuffer.ProgressiveUpdate(mValidRegion, aInvalidRegion,
                      oldValidRegion, &mPaintData, aCallback, aCallbackData);
  }

  

  mValidRegion = aVisibleRegion;
  if (!mPaintData.mCriticalDisplayPort.IsEmpty()) {
    mValidRegion.And(mValidRegion, LayerIntRect::ToUntyped(mPaintData.mCriticalDisplayPort));
  }

  TILING_LOG("TILING %p: Non-progressive paint invalid region %s\n", this, Stringify(aInvalidRegion).c_str());
  TILING_LOG("TILING %p: Non-progressive paint new valid region %s\n", this, Stringify(mValidRegion).c_str());

  mContentClient->mTiledBuffer.SetFrameResolution(mPaintData.mResolution);
  mContentClient->mTiledBuffer.PaintThebes(mValidRegion, aInvalidRegion, aCallback, aCallbackData);
  mPaintData.mPaintFinished = true;
  return true;
}

bool
ClientTiledPaintedLayer::RenderLowPrecision(nsIntRegion& aInvalidRegion,
                                           const nsIntRegion& aVisibleRegion,
                                           LayerManager::DrawPaintedLayerCallback aCallback,
                                           void* aCallbackData)
{
  
  
  if (!nsIntRegion(LayerIntRect::ToUntyped(mPaintData.mCriticalDisplayPort)).Contains(aVisibleRegion)) {
    nsIntRegion oldValidRegion = mContentClient->mLowPrecisionTiledBuffer.GetValidRegion();
    oldValidRegion.And(oldValidRegion, aVisibleRegion);

    bool updatedBuffer = false;

    
    if (mContentClient->mLowPrecisionTiledBuffer.GetFrameResolution() != mPaintData.mResolution ||
        mContentClient->mLowPrecisionTiledBuffer.HasFormatChanged()) {
      if (!mLowPrecisionValidRegion.IsEmpty()) {
        updatedBuffer = true;
      }
      oldValidRegion.SetEmpty();
      mLowPrecisionValidRegion.SetEmpty();
      mContentClient->mLowPrecisionTiledBuffer.ResetPaintedAndValidState();
      mContentClient->mLowPrecisionTiledBuffer.SetFrameResolution(mPaintData.mResolution);
      aInvalidRegion = aVisibleRegion;
    }

    
    if (mPaintData.mLowPrecisionPaintCount == 1) {
      mLowPrecisionValidRegion.And(mLowPrecisionValidRegion, aVisibleRegion);
    }
    mPaintData.mLowPrecisionPaintCount++;

    
    
    aInvalidRegion.Sub(aInvalidRegion, mValidRegion);

    TILING_LOG("TILING %p: Progressive paint: low-precision invalid region is %s\n", this, Stringify(aInvalidRegion).c_str());
    TILING_LOG("TILING %p: Progressive paint: low-precision old valid region is %s\n", this, Stringify(oldValidRegion).c_str());

    if (!aInvalidRegion.IsEmpty()) {
      updatedBuffer = mContentClient->mLowPrecisionTiledBuffer.ProgressiveUpdate(
                            mLowPrecisionValidRegion, aInvalidRegion, oldValidRegion,
                            &mPaintData, aCallback, aCallbackData);
    }

    TILING_LOG("TILING %p: Progressive paint: low-precision new valid region is %s\n", this, Stringify(mLowPrecisionValidRegion).c_str());
    return updatedBuffer;
  }
  if (!mLowPrecisionValidRegion.IsEmpty()) {
    TILING_LOG("TILING %p: Clearing low-precision buffer\n", this);
    
    mLowPrecisionValidRegion.SetEmpty();
    mContentClient->mLowPrecisionTiledBuffer.ResetPaintedAndValidState();
    
    
    
    return true;
  }
  return false;
}

void
ClientTiledPaintedLayer::EndPaint()
{
  mPaintData.mLastScrollOffset = mPaintData.mScrollOffset;
  mPaintData.mPaintFinished = true;
  mPaintData.mFirstPaint = false;
  TILING_LOG("TILING %p: Paint finished\n", this);
}

void
ClientTiledPaintedLayer::RenderLayer()
{
  LayerManager::DrawPaintedLayerCallback callback =
    ClientManager()->GetPaintedLayerCallback();
  void *data = ClientManager()->GetPaintedLayerCallbackData();
  if (!callback) {
    ClientManager()->SetTransactionIncomplete();
    return;
  }

  if (!mContentClient) {
    mContentClient = new TiledContentClient(this, ClientManager());

    mContentClient->Connect();
    ClientManager()->AsShadowForwarder()->Attach(mContentClient, this);
    MOZ_ASSERT(mContentClient->GetForwarder());
  }

  if (mContentClient->mTiledBuffer.HasFormatChanged()) {
    mValidRegion = nsIntRegion();
    mContentClient->mTiledBuffer.ResetPaintedAndValidState();
  }

  TILING_LOG("TILING %p: Initial visible region %s\n", this, Stringify(mVisibleRegion).c_str());
  TILING_LOG("TILING %p: Initial valid region %s\n", this, Stringify(mValidRegion).c_str());
  TILING_LOG("TILING %p: Initial low-precision valid region %s\n", this, Stringify(mLowPrecisionValidRegion).c_str());

  nsIntRegion neededRegion = mVisibleRegion;
#ifndef MOZ_IGNORE_PAINT_WILL_RESAMPLE
  
  if (MayResample()) {
    
    
    
    
    nsIntRect bounds = neededRegion.GetBounds();
    nsIntRect wholeTiles = bounds;
    wholeTiles.InflateToMultiple(nsIntSize(
      gfxPlatform::GetPlatform()->GetTileWidth(),
      gfxPlatform::GetPlatform()->GetTileHeight()));
    nsIntRect padded = bounds;
    padded.Inflate(1);
    padded.IntersectRect(padded, wholeTiles);
    neededRegion = padded;
  }
#endif

  nsIntRegion invalidRegion;
  invalidRegion.Sub(neededRegion, mValidRegion);
  if (invalidRegion.IsEmpty()) {
    EndPaint();
    return;
  }

  if (!ClientManager()->IsRepeatTransaction()) {
    
    if (GetMaskLayer()) {
      ToClientLayer(GetMaskLayer())->RenderLayer();
    }

    
    
    BeginPaint();
    if (mPaintData.mPaintFinished) {
      return;
    }

    
    
    
    mValidRegion.And(mValidRegion, neededRegion);
    if (!mPaintData.mCriticalDisplayPort.IsEmpty()) {
      mValidRegion.And(mValidRegion, LayerIntRect::ToUntyped(mPaintData.mCriticalDisplayPort));
      invalidRegion.And(invalidRegion, LayerIntRect::ToUntyped(mPaintData.mCriticalDisplayPort));
    }

    TILING_LOG("TILING %p: First-transaction valid region %s\n", this, Stringify(mValidRegion).c_str());
    TILING_LOG("TILING %p: First-transaction invalid region %s\n", this, Stringify(invalidRegion).c_str());
  } else {
    if (!mPaintData.mCriticalDisplayPort.IsEmpty()) {
      invalidRegion.And(invalidRegion, LayerIntRect::ToUntyped(mPaintData.mCriticalDisplayPort));
    }
    TILING_LOG("TILING %p: Repeat-transaction invalid region %s\n", this, Stringify(invalidRegion).c_str());
  }

  nsIntRegion lowPrecisionInvalidRegion;
  if (gfxPrefs::UseLowPrecisionBuffer()) {
    
    
    lowPrecisionInvalidRegion.Sub(neededRegion, mLowPrecisionValidRegion);
    lowPrecisionInvalidRegion.Sub(lowPrecisionInvalidRegion, mValidRegion);
  }
  TILING_LOG("TILING %p: Low-precision invalid region %s\n", this, Stringify(lowPrecisionInvalidRegion).c_str());

  bool updatedHighPrecision = RenderHighPrecision(invalidRegion,
                                                  neededRegion,
                                                  callback, data);
  if (updatedHighPrecision) {
    ClientManager()->Hold(this);
    mContentClient->UseTiledLayerBuffer(TiledContentClient::TILED_BUFFER);

    if (!mPaintData.mPaintFinished) {
      
      
      ClientManager()->SetRepeatTransaction();
      return;
    }
  }

  
  if (lowPrecisionInvalidRegion.IsEmpty()) {
    EndPaint();
    return;
  }

  if (updatedHighPrecision) {
    
    
    
    
    TILING_LOG("TILING %p: Scheduling repeat transaction for low-precision painting\n", this);
    ClientManager()->SetRepeatTransaction();
    mPaintData.mLowPrecisionPaintCount = 1;
    mPaintData.mPaintFinished = false;
    return;
  }

  bool updatedLowPrecision = RenderLowPrecision(lowPrecisionInvalidRegion,
                                                neededRegion,
                                                callback, data);
  if (updatedLowPrecision) {
    ClientManager()->Hold(this);
    mContentClient->UseTiledLayerBuffer(TiledContentClient::LOW_PRECISION_TILED_BUFFER);

    if (!mPaintData.mPaintFinished) {
      
      
      ClientManager()->SetRepeatTransaction();
      return;
    }
  }

  
  
  EndPaint();
}

void
ClientTiledPaintedLayer::PrintInfo(std::stringstream& aStream, const char* aPrefix)
{
  PaintedLayer::PrintInfo(aStream, aPrefix);
  if (mContentClient) {
    aStream << "\n";
    nsAutoCString pfx(aPrefix);
    pfx += "  ";
    mContentClient->PrintInfo(aStream, pfx.get());
  }
}

} 
} 
