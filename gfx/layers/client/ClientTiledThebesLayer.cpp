



#include "ClientTiledThebesLayer.h"
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
#include "mozilla/layers/LayersMessages.h"
#include "mozilla/mozalloc.h"           
#include "nsISupportsImpl.h"            
#include "nsRect.h"                     
#include "LayersLogging.h"

namespace mozilla {
namespace layers {


ClientTiledThebesLayer::ClientTiledThebesLayer(ClientLayerManager* const aManager,
                                               ClientLayerManager::ThebesLayerCreationHint aCreationHint)
  : ThebesLayer(aManager,
                static_cast<ClientLayer*>(MOZ_THIS_IN_INITIALIZER_LIST()),
                aCreationHint)
  , mContentClient()
{
  MOZ_COUNT_CTOR(ClientTiledThebesLayer);
  mPaintData.mLastScrollOffset = ParentLayerPoint(0, 0);
  mPaintData.mFirstPaint = true;
}

ClientTiledThebesLayer::~ClientTiledThebesLayer()
{
  MOZ_COUNT_DTOR(ClientTiledThebesLayer);
}

void
ClientTiledThebesLayer::ClearCachedResources()
{
  if (mContentClient) {
    mContentClient->ClearCachedResources();
  }
  mValidRegion.SetEmpty();
  mContentClient = nullptr;
}

void
ClientTiledThebesLayer::FillSpecificAttributes(SpecificLayerAttributes& aAttrs)
{
  aAttrs = ThebesLayerAttributes(GetValidRegion());
}

static LayerRect
ApplyParentLayerToLayerTransform(const gfx::Matrix4x4& aTransform, const ParentLayerRect& aParentLayerRect)
{
  return TransformTo<LayerPixel>(aTransform, aParentLayerRect);
}

static gfx::Matrix4x4
GetTransformToAncestorsParentLayer(Layer* aStart, Layer* aAncestor)
{
  gfx::Matrix4x4 transform;
  Layer* ancestorParent = aAncestor->GetParent();
  for (Layer* iter = aStart; iter != ancestorParent; iter = iter->GetParent()) {
    transform = transform * iter->GetTransform();
    
    
    const FrameMetrics& metrics = iter->GetFrameMetrics();
    transform = transform * gfx::Matrix4x4().Scale(metrics.mResolution.scale, metrics.mResolution.scale, 1.f);
  }
  return transform;
}

void
ClientTiledThebesLayer::GetAncestorLayers(Layer** aOutScrollAncestor,
                                          Layer** aOutDisplayPortAncestor)
{
  Layer* scrollAncestor = nullptr;
  Layer* displayPortAncestor = nullptr;
  for (Layer* ancestor = this; ancestor; ancestor = ancestor->GetParent()) {
    const FrameMetrics& metrics = ancestor->GetFrameMetrics();
    if (!scrollAncestor && metrics.GetScrollId() != FrameMetrics::NULL_SCROLL_ID) {
      scrollAncestor = ancestor;
    }
    if (!metrics.mDisplayPort.IsEmpty()) {
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
}

void
ClientTiledThebesLayer::BeginPaint()
{
  mPaintData.mLowPrecisionPaintCount = 0;
  mPaintData.mPaintFinished = false;
  mPaintData.mCompositionBounds.SetEmpty();
  mPaintData.mCriticalDisplayPort.SetEmpty();

  if (!GetBaseTransform().Is2D()) {
    
    
    
    return;
  }

  
  
  Layer* scrollAncestor = nullptr;
  Layer* displayPortAncestor = nullptr;
  GetAncestorLayers(&scrollAncestor, &displayPortAncestor);

  if (!displayPortAncestor || !scrollAncestor) {
    
#if defined(MOZ_WIDGET_ANDROID) || defined(MOZ_B2G)
    
    
    NS_WARNING("Tiled Thebes layer with no scrollable container ancestor");
#endif
    return;
  }

  TILING_LOG("TILING %p: Found scrollAncestor %p and displayPortAncestor %p\n", this,
    scrollAncestor, displayPortAncestor);

  const FrameMetrics& scrollMetrics = scrollAncestor->GetFrameMetrics();
  const FrameMetrics& displayportMetrics = displayPortAncestor->GetFrameMetrics();

  
  
  gfx::Matrix4x4 transformDisplayPortToLayer =
    GetTransformToAncestorsParentLayer(this, displayPortAncestor);
  transformDisplayPortToLayer.Invert();

  
  
  
  
  

  
  
  ParentLayerRect criticalDisplayPort =
    (displayportMetrics.mCriticalDisplayPort * displayportMetrics.GetZoomToParent())
    + displayportMetrics.mCompositionBounds.TopLeft();
  mPaintData.mCriticalDisplayPort = RoundedOut(
    ApplyParentLayerToLayerTransform(transformDisplayPortToLayer, criticalDisplayPort));
  TILING_LOG("TILING %p: Critical displayport %s\n", this, Stringify(mPaintData.mCriticalDisplayPort).c_str());

  
  
  mPaintData.mResolution = displayportMetrics.GetZoomToParent();
  TILING_LOG("TILING %p: Resolution %f\n", this, mPaintData.mResolution.scale);

  
  mPaintData.mTransformToCompBounds =
    GetTransformToAncestorsParentLayer(this, scrollAncestor);
  gfx::Matrix4x4 transformToBounds = mPaintData.mTransformToCompBounds;
  transformToBounds.Invert();
  mPaintData.mCompositionBounds = ApplyParentLayerToLayerTransform(
    transformToBounds, scrollMetrics.mCompositionBounds);
  TILING_LOG("TILING %p: Composition bounds %s\n", this, Stringify(mPaintData.mCompositionBounds).c_str());

  
  mPaintData.mScrollOffset = displayportMetrics.GetScrollOffset() * displayportMetrics.GetZoomToParent();
  TILING_LOG("TILING %p: Scroll offset %s\n", this, Stringify(mPaintData.mScrollOffset).c_str());
}

bool
ClientTiledThebesLayer::UseFastPath()
{
  const FrameMetrics& parentMetrics = GetParent()->GetFrameMetrics();
  bool multipleTransactionsNeeded = gfxPrefs::UseProgressiveTilePainting()
                                 || gfxPrefs::UseLowPrecisionBuffer()
                                 || !parentMetrics.mCriticalDisplayPort.IsEmpty();
  bool isFixed = GetIsFixedPosition() || GetParent()->GetIsFixedPosition();
  return !multipleTransactionsNeeded || isFixed || parentMetrics.mDisplayPort.IsEmpty();
}

bool
ClientTiledThebesLayer::RenderHighPrecision(nsIntRegion& aInvalidRegion,
                                            const nsIntRegion& aVisibleRegion,
                                            LayerManager::DrawThebesLayerCallback aCallback,
                                            void* aCallbackData)
{
  
  
  if (aInvalidRegion.IsEmpty() || mPaintData.mLowPrecisionPaintCount != 0) {
    return false;
  }

  
  
  if (gfxPrefs::UseProgressiveTilePainting() &&
      !ClientManager()->HasShadowTarget() &&
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
  return true;
}

bool
ClientTiledThebesLayer::RenderLowPrecision(nsIntRegion& aInvalidRegion,
                                           const nsIntRegion& aVisibleRegion,
                                           LayerManager::DrawThebesLayerCallback aCallback,
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
ClientTiledThebesLayer::EndPaint()
{
  mPaintData.mLastScrollOffset = mPaintData.mScrollOffset;
  mPaintData.mPaintFinished = true;
  mPaintData.mFirstPaint = false;
  TILING_LOG("TILING %p: Paint finished\n", this);
}

void
ClientTiledThebesLayer::RenderLayer()
{
  LayerManager::DrawThebesLayerCallback callback =
    ClientManager()->GetThebesLayerCallback();
  void *data = ClientManager()->GetThebesLayerCallbackData();
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
  }

  TILING_LOG("TILING %p: Initial visible region %s\n", this, Stringify(mVisibleRegion).c_str());
  TILING_LOG("TILING %p: Initial valid region %s\n", this, Stringify(mValidRegion).c_str());
  TILING_LOG("TILING %p: Initial low-precision valid region %s\n", this, Stringify(mLowPrecisionValidRegion).c_str());

  nsIntRegion neededRegion = mVisibleRegion;
#ifndef MOZ_GFX_OPTIMIZE_MOBILE
  
  if (MayResample()) {
    
    
    
    
    nsIntRect bounds = neededRegion.GetBounds();
    nsIntRect wholeTiles = bounds;
    wholeTiles.Inflate(nsIntSize(gfxPrefs::LayersTileWidth(), gfxPrefs::LayersTileHeight()));
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

    
    if (UseFastPath()) {
      TILING_LOG("TILING %p: Taking fast-path\n", this);
      mValidRegion = neededRegion;
      mContentClient->mTiledBuffer.PaintThebes(mValidRegion, invalidRegion, callback, data);
      ClientManager()->Hold(this);
      mContentClient->UseTiledLayerBuffer(TiledContentClient::TILED_BUFFER);
      return;
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

} 
} 
