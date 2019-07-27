



#include "ClientTiledThebesLayer.h"
#include "FrameMetrics.h"               
#include "Units.h"                      
#include "UnitTransforms.h"             
#include "ClientLayerManager.h"         
#include "gfx3DMatrix.h"                
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
}

void
ClientTiledThebesLayer::FillSpecificAttributes(SpecificLayerAttributes& aAttrs)
{
  aAttrs = ThebesLayerAttributes(GetValidRegion());
}

static LayerRect
ApplyParentLayerToLayerTransform(const gfx3DMatrix& aTransform, const ParentLayerRect& aParentLayerRect)
{
  return TransformTo<LayerPixel>(aTransform, aParentLayerRect);
}

static gfx3DMatrix
GetTransformToAncestorsParentLayer(Layer* aStart, Layer* aAncestor)
{
  gfx::Matrix4x4 transform;
  Layer* ancestorParent = aAncestor->GetParent();
  for (Layer* iter = aStart; iter != ancestorParent; iter = iter->GetParent()) {
    if (iter->AsContainerLayer()) {
      
      
      const FrameMetrics& metrics = iter->AsContainerLayer()->GetFrameMetrics();
      transform = transform * gfx::Matrix4x4().Scale(metrics.mResolution.scale, metrics.mResolution.scale, 1.f);
    }
    transform = transform * iter->GetTransform();
  }
  gfx3DMatrix ret;
  gfx::To3DMatrix(transform, ret);
  return ret;
}

void
ClientTiledThebesLayer::GetAncestorLayers(ContainerLayer** aOutScrollAncestor,
                                          ContainerLayer** aOutDisplayPortAncestor)
{
  ContainerLayer* scrollAncestor = nullptr;
  ContainerLayer* displayPortAncestor = nullptr;
  for (ContainerLayer* ancestor = GetParent(); ancestor; ancestor = ancestor->GetParent()) {
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

  
  
  ContainerLayer* scrollAncestor = nullptr;
  ContainerLayer* displayPortAncestor = nullptr;
  GetAncestorLayers(&scrollAncestor, &displayPortAncestor);

  if (!displayPortAncestor || !scrollAncestor) {
    
#if defined(MOZ_WIDGET_ANDROID) || defined(MOZ_B2G)
    
    
    NS_WARNING("Tiled Thebes layer with no scrollable container ancestor");
#endif
    return;
  }

  TILING_LOG(("TILING %p: Found scrollAncestor %p and displayPortAncestor %p\n", this,
    scrollAncestor, displayPortAncestor));

  const FrameMetrics& scrollMetrics = scrollAncestor->GetFrameMetrics();
  const FrameMetrics& displayportMetrics = displayPortAncestor->GetFrameMetrics();

  
  
  gfx3DMatrix transformDisplayPortToLayer =
    GetTransformToAncestorsParentLayer(this, displayPortAncestor).Inverse();

  
  
  
  
  

  
  
  ParentLayerRect criticalDisplayPort =
    (displayportMetrics.mCriticalDisplayPort * displayportMetrics.GetZoomToParent())
    + displayportMetrics.mCompositionBounds.TopLeft();
  mPaintData.mCriticalDisplayPort = RoundedOut(
    ApplyParentLayerToLayerTransform(transformDisplayPortToLayer, criticalDisplayPort));
  TILING_LOG_OBJ(("TILING %p: Critical displayport %s\n", this, tmpstr.get()), mPaintData.mCriticalDisplayPort);

  
  
  mPaintData.mResolution = displayportMetrics.GetZoomToParent();
  TILING_LOG(("TILING %p: Resolution %f\n", this, mPaintData.mResolution.scale));

  
  mPaintData.mTransformToCompBounds =
    GetTransformToAncestorsParentLayer(this, scrollAncestor);
  mPaintData.mCompositionBounds = ApplyParentLayerToLayerTransform(
    mPaintData.mTransformToCompBounds.Inverse(), scrollMetrics.mCompositionBounds);
  TILING_LOG_OBJ(("TILING %p: Composition bounds %s\n", this, tmpstr.get()), mPaintData.mCompositionBounds);

  
  mPaintData.mScrollOffset = displayportMetrics.GetScrollOffset() * displayportMetrics.GetZoomToParent();
  TILING_LOG_OBJ(("TILING %p: Scroll offset %s\n", this, tmpstr.get()), mPaintData.mScrollOffset);
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
    oldValidRegion.And(oldValidRegion, mVisibleRegion);
    if (!mPaintData.mCriticalDisplayPort.IsEmpty()) {
      oldValidRegion.And(oldValidRegion, LayerIntRect::ToUntyped(mPaintData.mCriticalDisplayPort));
    }

    TILING_LOG_OBJ(("TILING %p: Progressive update with old valid region %s\n", this, tmpstr.get()), oldValidRegion);

    return mContentClient->mTiledBuffer.ProgressiveUpdate(mValidRegion, aInvalidRegion,
                      oldValidRegion, &mPaintData, aCallback, aCallbackData);
  }

  

  mValidRegion = mVisibleRegion;
  if (!mPaintData.mCriticalDisplayPort.IsEmpty()) {
    mValidRegion.And(mValidRegion, LayerIntRect::ToUntyped(mPaintData.mCriticalDisplayPort));
  }

  TILING_LOG_OBJ(("TILING %p: Non-progressive paint invalid region %s\n", this, tmpstr.get()), aInvalidRegion);
  TILING_LOG_OBJ(("TILING %p: Non-progressive paint new valid region %s\n", this, tmpstr.get()), mValidRegion);

  mContentClient->mTiledBuffer.SetFrameResolution(mPaintData.mResolution);
  mContentClient->mTiledBuffer.PaintThebes(mValidRegion, aInvalidRegion, aCallback, aCallbackData);
  return true;
}

bool
ClientTiledThebesLayer::RenderLowPrecision(nsIntRegion& aInvalidRegion,
                                           LayerManager::DrawThebesLayerCallback aCallback,
                                           void* aCallbackData)
{
  
  
  if (!nsIntRegion(LayerIntRect::ToUntyped(mPaintData.mCriticalDisplayPort)).Contains(mVisibleRegion)) {
    nsIntRegion oldValidRegion = mContentClient->mLowPrecisionTiledBuffer.GetValidRegion();
    oldValidRegion.And(oldValidRegion, mVisibleRegion);

    bool updatedBuffer = false;

    
    if (mContentClient->mLowPrecisionTiledBuffer.GetFrameResolution() != mPaintData.mResolution ||
        mContentClient->mLowPrecisionTiledBuffer.HasFormatChanged()) {
      if (!mLowPrecisionValidRegion.IsEmpty()) {
        updatedBuffer = true;
      }
      oldValidRegion.SetEmpty();
      mLowPrecisionValidRegion.SetEmpty();
      mContentClient->mLowPrecisionTiledBuffer.SetFrameResolution(mPaintData.mResolution);
      aInvalidRegion = mVisibleRegion;
    }

    
    if (mPaintData.mLowPrecisionPaintCount == 1) {
      mLowPrecisionValidRegion.And(mLowPrecisionValidRegion, mVisibleRegion);
    }
    mPaintData.mLowPrecisionPaintCount++;

    
    
    aInvalidRegion.Sub(aInvalidRegion, mValidRegion);

    TILING_LOG_OBJ(("TILING %p: Progressive paint: low-precision invalid region is %s\n", this, tmpstr.get()), aInvalidRegion);
    TILING_LOG_OBJ(("TILING %p: Progressive paint: low-precision old valid region is %s\n", this, tmpstr.get()), oldValidRegion);

    if (!aInvalidRegion.IsEmpty()) {
      updatedBuffer = mContentClient->mLowPrecisionTiledBuffer.ProgressiveUpdate(
                            mLowPrecisionValidRegion, aInvalidRegion, oldValidRegion,
                            &mPaintData, aCallback, aCallbackData);
    }

    TILING_LOG_OBJ(("TILING %p: Progressive paint: low-precision new valid region is %s\n", this, tmpstr.get()), mLowPrecisionValidRegion);
    return updatedBuffer;
  }
  if (!mLowPrecisionValidRegion.IsEmpty()) {
    TILING_LOG(("TILING %p: Clearing low-precision buffer\n", this));
    
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
  TILING_LOG(("TILING %p: Paint finished\n", this));
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

  TILING_LOG_OBJ(("TILING %p: Initial visible region %s\n", this, tmpstr.get()), mVisibleRegion);
  TILING_LOG_OBJ(("TILING %p: Initial valid region %s\n", this, tmpstr.get()), mValidRegion);
  TILING_LOG_OBJ(("TILING %p: Initial low-precision valid region %s\n", this, tmpstr.get()), mLowPrecisionValidRegion);

  nsIntRegion invalidRegion;
  invalidRegion.Sub(mVisibleRegion, mValidRegion);
  if (invalidRegion.IsEmpty()) {
    EndPaint();
    return;
  }

  if (!ClientManager()->IsRepeatTransaction()) {
    
    if (GetMaskLayer()) {
      ToClientLayer(GetMaskLayer())->RenderLayer();
    }

    
    if (UseFastPath()) {
      TILING_LOG(("TILING %p: Taking fast-path\n", this));
      mValidRegion = mVisibleRegion;
      mContentClient->mTiledBuffer.PaintThebes(mValidRegion, invalidRegion, callback, data);
      ClientManager()->Hold(this);
      mContentClient->UseTiledLayerBuffer(TiledContentClient::TILED_BUFFER);
      return;
    }

    
    
    BeginPaint();
    if (mPaintData.mPaintFinished) {
      return;
    }

    
    
    
    mValidRegion.And(mValidRegion, mVisibleRegion);
    if (!mPaintData.mCriticalDisplayPort.IsEmpty()) {
      mValidRegion.And(mValidRegion, LayerIntRect::ToUntyped(mPaintData.mCriticalDisplayPort));
      invalidRegion.And(invalidRegion, LayerIntRect::ToUntyped(mPaintData.mCriticalDisplayPort));
    }

    TILING_LOG_OBJ(("TILING %p: First-transaction valid region %s\n", this, tmpstr.get()), mValidRegion);
    TILING_LOG_OBJ(("TILING %p: First-transaction invalid region %s\n", this, tmpstr.get()), invalidRegion);
  } else {
    if (!mPaintData.mCriticalDisplayPort.IsEmpty()) {
      invalidRegion.And(invalidRegion, LayerIntRect::ToUntyped(mPaintData.mCriticalDisplayPort));
    }
    TILING_LOG_OBJ(("TILING %p: Repeat-transaction invalid region %s\n", this, tmpstr.get()), invalidRegion);
  }

  nsIntRegion lowPrecisionInvalidRegion;
  if (gfxPrefs::UseLowPrecisionBuffer()) {
    
    
    lowPrecisionInvalidRegion.Sub(mVisibleRegion, mLowPrecisionValidRegion);
    lowPrecisionInvalidRegion.Sub(lowPrecisionInvalidRegion, mValidRegion);
  }
  TILING_LOG_OBJ(("TILING %p: Low-precision invalid region %s\n", this, tmpstr.get()), lowPrecisionInvalidRegion);

  bool updatedHighPrecision = RenderHighPrecision(invalidRegion, callback, data);
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
    
    
    
    
    TILING_LOG(("TILING %p: Scheduling repeat transaction for low-precision painting\n", this));
    ClientManager()->SetRepeatTransaction();
    mPaintData.mLowPrecisionPaintCount = 1;
    mPaintData.mPaintFinished = false;
    return;
  }

  bool updatedLowPrecision = RenderLowPrecision(lowPrecisionInvalidRegion, callback, data);
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
