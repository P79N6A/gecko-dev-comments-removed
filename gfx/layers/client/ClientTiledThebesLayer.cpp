








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

  if (!displayPortAncestor || !scrollAncestor) {
    
#if defined(MOZ_WIDGET_ANDROID) || defined(MOZ_B2G)
    
    
    NS_WARNING("Tiled Thebes layer with no scrollable container ancestor");
#endif
    return;
  }

  TILING_PRLOG(("TILING 0x%p: Found scrollAncestor 0x%p and displayPortAncestor 0x%p\n", this,
    scrollAncestor, displayPortAncestor));

  const FrameMetrics& scrollMetrics = scrollAncestor->GetFrameMetrics();
  const FrameMetrics& displayportMetrics = displayPortAncestor->GetFrameMetrics();

  
  
  gfx3DMatrix transformToDisplayPort =
    GetTransformToAncestorsParentLayer(this, displayPortAncestor);

  mPaintData.mTransformDisplayPortToLayer = transformToDisplayPort.Inverse();

  
  
  
  
  

  
  
  ParentLayerRect criticalDisplayPort =
    (displayportMetrics.mCriticalDisplayPort * displayportMetrics.GetZoomToParent())
    + displayportMetrics.mCompositionBounds.TopLeft();
  mPaintData.mCriticalDisplayPort = RoundedOut(
    ApplyParentLayerToLayerTransform(mPaintData.mTransformDisplayPortToLayer, criticalDisplayPort));
  TILING_PRLOG_OBJ(("TILING 0x%p: Critical displayport %s\n", this, tmpstr.get()), mPaintData.mCriticalDisplayPort);

  
  
  ParentLayerRect viewport =
    (displayportMetrics.mViewport * displayportMetrics.GetZoomToParent())
    + displayportMetrics.mCompositionBounds.TopLeft();
  mPaintData.mViewport = ApplyParentLayerToLayerTransform(
    mPaintData.mTransformDisplayPortToLayer, viewport);
  TILING_PRLOG_OBJ(("TILING 0x%p: Viewport %s\n", this, tmpstr.get()), mPaintData.mViewport);

  
  
  mPaintData.mResolution = displayportMetrics.GetZoomToParent();
  TILING_PRLOG(("TILING 0x%p: Resolution %f\n", this, mPaintData.mResolution.scale));

  
  gfx3DMatrix transformToCompBounds =
    GetTransformToAncestorsParentLayer(this, scrollAncestor);
  mPaintData.mCompositionBounds = ApplyParentLayerToLayerTransform(
    transformToCompBounds.Inverse(), ParentLayerRect(scrollMetrics.mCompositionBounds));
  TILING_PRLOG_OBJ(("TILING 0x%p: Composition bounds %s\n", this, tmpstr.get()), mPaintData.mCompositionBounds);

  
  mPaintData.mScrollOffset = displayportMetrics.GetScrollOffset() * displayportMetrics.GetZoomToParent();
  TILING_PRLOG_OBJ(("TILING 0x%p: Scroll offset %s\n", this, tmpstr.get()), mPaintData.mScrollOffset);
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

    TILING_PRLOG_OBJ(("TILING 0x%p: Progressive update with old valid region %s\n", this, tmpstr.get()), oldValidRegion);

    return mContentClient->mTiledBuffer.ProgressiveUpdate(mValidRegion, aInvalidRegion,
                      oldValidRegion, &mPaintData, aCallback, aCallbackData);
  }

  

  mValidRegion = mVisibleRegion;
  if (!mPaintData.mCriticalDisplayPort.IsEmpty()) {
    mValidRegion.And(mValidRegion, LayerIntRect::ToUntyped(mPaintData.mCriticalDisplayPort));
  }

  TILING_PRLOG_OBJ(("TILING 0x%p: Non-progressive paint invalid region %s\n", this, tmpstr.get()), aInvalidRegion);
  TILING_PRLOG_OBJ(("TILING 0x%p: Non-progressive paint new valid region %s\n", this, tmpstr.get()), mValidRegion);

  mContentClient->mTiledBuffer.SetFrameResolution(mPaintData.mResolution);
  mContentClient->mTiledBuffer.PaintThebes(mValidRegion, aInvalidRegion, aCallback, aCallbackData);
  return true;
}

bool
ClientTiledThebesLayer::RenderLowPrecision(nsIntRegion& aInvalidRegion,
                                           LayerManager::DrawThebesLayerCallback aCallback,
                                           void* aCallbackData)
{
  
  
  if (!aInvalidRegion.IsEmpty() &&
      !nsIntRegion(LayerIntRect::ToUntyped(mPaintData.mCriticalDisplayPort)).Contains(mVisibleRegion)) {
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

    TILING_PRLOG_OBJ(("TILING 0x%p: Progressive paint: low-precision invalid region is %s\n", this, tmpstr.get()), aInvalidRegion);
    TILING_PRLOG_OBJ(("TILING 0x%p: Progressive paint: low-precision new valid region is %s\n", this, tmpstr.get()), mLowPrecisionValidRegion);

    if (!aInvalidRegion.IsEmpty()) {
      updatedBuffer = mContentClient->mLowPrecisionTiledBuffer.ProgressiveUpdate(
                            mLowPrecisionValidRegion, aInvalidRegion, oldValidRegion,
                            &mPaintData, aCallback, aCallbackData);
    }
    return updatedBuffer;
  }
  if (!mLowPrecisionValidRegion.IsEmpty()) {
    TILING_PRLOG(("TILING 0x%p: Clearing low-precision buffer\n", this));
    
    mLowPrecisionValidRegion.SetEmpty();
    mContentClient->mLowPrecisionTiledBuffer.ResetPaintedAndValidState();
    
    
    
    return true;
  }
  return false;
}

void
ClientTiledThebesLayer::EndPaint(bool aFinish)
{
  if (!aFinish && !mPaintData.mPaintFinished) {
    return;
  }

  mPaintData.mLastScrollOffset = mPaintData.mScrollOffset;
  mPaintData.mPaintFinished = true;
  mPaintData.mFirstPaint = false;
  TILING_PRLOG(("TILING 0x%p: Paint finished\n", this));
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

  TILING_PRLOG_OBJ(("TILING 0x%p: Initial visible region %s\n", this, tmpstr.get()), mVisibleRegion);
  TILING_PRLOG_OBJ(("TILING 0x%p: Initial valid region %s\n", this, tmpstr.get()), mValidRegion);

  nsIntRegion invalidRegion;
  invalidRegion.Sub(mVisibleRegion, mValidRegion);
  if (invalidRegion.IsEmpty()) {
    EndPaint(true);
    return;
  }

  if (!ClientManager()->IsRepeatTransaction()) {
    
    if (GetMaskLayer()) {
      ToClientLayer(GetMaskLayer())->RenderLayer();
    }

    
    if (UseFastPath()) {
      TILING_PRLOG(("TILING 0x%p: Taking fast-path\n"));
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

    TILING_PRLOG_OBJ(("TILING 0x%p: First-transaction valid region %s\n", this, tmpstr.get()), mValidRegion);
    TILING_PRLOG_OBJ(("TILING 0x%p: First-transaction invalid region %s\n", this, tmpstr.get()), invalidRegion);
  } else {
    if (!mPaintData.mCriticalDisplayPort.IsEmpty()) {
      invalidRegion.And(invalidRegion, LayerIntRect::ToUntyped(mPaintData.mCriticalDisplayPort));
    }
    TILING_PRLOG_OBJ(("TILING 0x%p: Repeat-transaction invalid region %s\n", this, tmpstr.get()), invalidRegion);
  }

  nsIntRegion lowPrecisionInvalidRegion;
  if (!mPaintData.mCriticalDisplayPort.IsEmpty()) {
    if (gfxPrefs::UseLowPrecisionBuffer()) {
      
      lowPrecisionInvalidRegion.Sub(mVisibleRegion, mLowPrecisionValidRegion);

      
      
      lowPrecisionInvalidRegion.Sub(lowPrecisionInvalidRegion, mValidRegion);
    }

    if (invalidRegion.IsEmpty() && lowPrecisionInvalidRegion.IsEmpty()) {
      EndPaint(true);
      return;
    }
  }

  TILING_PRLOG_OBJ(("TILING 0x%p: Invalid region %s\n", this, tmpstr.get()), invalidRegion);

  bool updatedHighPrecision = RenderHighPrecision(invalidRegion, callback, data);
  if (updatedHighPrecision) {
    ClientManager()->Hold(this);
    mContentClient->UseTiledLayerBuffer(TiledContentClient::TILED_BUFFER);

    if (!mPaintData.mPaintFinished) {
      
      
      ClientManager()->SetRepeatTransaction();
      return;
    }

    
    
    if (!lowPrecisionInvalidRegion.IsEmpty()) {
      ClientManager()->SetRepeatTransaction();
      mPaintData.mLowPrecisionPaintCount = 1;
      mPaintData.mPaintFinished = false;
    }

    
    
    EndPaint(false);
    return;
  }

  TILING_PRLOG_OBJ(("TILING 0x%p: Low-precision valid region is %s\n", this, tmpstr.get()), mLowPrecisionValidRegion);
  TILING_PRLOG_OBJ(("TILING 0x%p: Low-precision invalid region is %s\n", this, tmpstr.get()), lowPrecisionInvalidRegion);

  bool updatedLowPrecision = RenderLowPrecision(lowPrecisionInvalidRegion, callback, data);
  if (updatedLowPrecision) {
    ClientManager()->Hold(this);
    mContentClient->UseTiledLayerBuffer(TiledContentClient::LOW_PRECISION_TILED_BUFFER);

    if (!mPaintData.mPaintFinished) {
      
      
      ClientManager()->SetRepeatTransaction();
    }
  }

  EndPaint(false);
}

} 
} 
