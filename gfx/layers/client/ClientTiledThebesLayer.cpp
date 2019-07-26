



#include "mozilla/layers/PLayerTransactionChild.h"
#include "ClientTiledThebesLayer.h"
#include "gfxImageSurface.h"
#include "GeckoProfiler.h"
#include "gfxPlatform.h"


namespace mozilla {
namespace layers {


ClientTiledThebesLayer::ClientTiledThebesLayer(ClientLayerManager* const aManager)
  : ThebesLayer(aManager, static_cast<ClientLayer*>(this))
  , mContentClient()
{
  MOZ_COUNT_CTOR(ClientTiledThebesLayer);
  mPaintData.mLastScrollOffset = gfx::Point(0, 0);
  mPaintData.mFirstPaint = true;
}

ClientTiledThebesLayer::~ClientTiledThebesLayer()
{
  MOZ_COUNT_DTOR(ClientTiledThebesLayer);
}

void
ClientTiledThebesLayer::FillSpecificAttributes(SpecificLayerAttributes& aAttrs)
{
  aAttrs = ThebesLayerAttributes(GetValidRegion());
}

void
ClientTiledThebesLayer::BeginPaint()
{
  if (ClientManager()->IsRepeatTransaction()) {
    return;
  }

  mPaintData.mLowPrecisionPaintCount = 0;
  mPaintData.mPaintFinished = false;

  
  mPaintData.mTransformScreenToLayer = GetEffectiveTransform();
  
  
  
  for (ContainerLayer* parent = GetParent(); parent; parent = parent->GetParent()) {
    if (parent->UseIntermediateSurface()) {
      mPaintData.mTransformScreenToLayer.PreMultiply(parent->GetEffectiveTransform());
    }
  }
  mPaintData.mTransformScreenToLayer.Invert();

  
  mPaintData.mLayerCriticalDisplayPort.SetEmpty();
  const gfx::Rect& criticalDisplayPort = GetParent()->GetFrameMetrics().mCriticalDisplayPort;
  if (!criticalDisplayPort.IsEmpty()) {
    gfxRect transformedCriticalDisplayPort =
      mPaintData.mTransformScreenToLayer.TransformBounds(
        gfxRect(criticalDisplayPort.x, criticalDisplayPort.y,
                criticalDisplayPort.width, criticalDisplayPort.height));
    transformedCriticalDisplayPort.RoundOut();
    mPaintData.mLayerCriticalDisplayPort = nsIntRect(transformedCriticalDisplayPort.x,
                                             transformedCriticalDisplayPort.y,
                                             transformedCriticalDisplayPort.width,
                                             transformedCriticalDisplayPort.height);
  }

  
  mPaintData.mResolution.SizeTo(1, 1);
  for (ContainerLayer* parent = GetParent(); parent; parent = parent->GetParent()) {
    const FrameMetrics& metrics = parent->GetFrameMetrics();
    mPaintData.mResolution.width *= metrics.mResolution.width;
    mPaintData.mResolution.height *= metrics.mResolution.height;
  }

  
  
  mPaintData.mCompositionBounds.SetEmpty();
  mPaintData.mScrollOffset.MoveTo(0, 0);
  Layer* primaryScrollable = ClientManager()->GetPrimaryScrollableLayer();
  if (primaryScrollable) {
    const FrameMetrics& metrics = primaryScrollable->AsContainerLayer()->GetFrameMetrics();
    mPaintData.mScrollOffset = metrics.mScrollOffset;
    gfxRect transformedViewport = mPaintData.mTransformScreenToLayer.TransformBounds(
      gfxRect(metrics.mCompositionBounds.x, metrics.mCompositionBounds.y,
              metrics.mCompositionBounds.width, metrics.mCompositionBounds.height));
    transformedViewport.RoundOut();
    mPaintData.mCompositionBounds =
      nsIntRect(transformedViewport.x, transformedViewport.y,
                transformedViewport.width, transformedViewport.height);
  }
}

void
ClientTiledThebesLayer::EndPaint(bool aFinish)
{
  if (!aFinish && !mPaintData.mPaintFinished) {
    return;
  }

  mPaintData.mLastScrollOffset = mPaintData.mScrollOffset;
  mPaintData.mPaintFinished = true;
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
    ClientManager()->Attach(mContentClient, this);
    MOZ_ASSERT(mContentClient->GetForwarder());
  }

  if (mContentClient->mTiledBuffer.HasFormatChanged()) {
    mValidRegion = nsIntRegion();
  }

  nsIntRegion invalidRegion = mVisibleRegion;
  invalidRegion.Sub(invalidRegion, mValidRegion);
  if (invalidRegion.IsEmpty()) {
    EndPaint(true);
    return;
  }

  
  if (GetMaskLayer() && !ClientManager()->IsRepeatTransaction()) {
    ToClientLayer(GetMaskLayer())->RenderLayer();
  }

  
  
  if (!gfxPlatform::UseProgressiveTilePainting() &&
      !gfxPlatform::UseLowPrecisionBuffer() &&
      GetParent()->GetFrameMetrics().mCriticalDisplayPort.IsEmpty()) {
    mValidRegion = mVisibleRegion;

    NS_ASSERTION(!ClientManager()->IsRepeatTransaction(), "Didn't paint our mask layer");

    mContentClient->mTiledBuffer.PaintThebes(mValidRegion, invalidRegion,
                                             callback, data);

    ClientManager()->Hold(this);
    mContentClient->LockCopyAndWrite(TiledContentClient::TILED_BUFFER);

    return;
  }

  
  BeginPaint();
  if (mPaintData.mPaintFinished) {
    return;
  }

  
  
  if (!ClientManager()->IsRepeatTransaction()) {
    mValidRegion.And(mValidRegion, mVisibleRegion);
    if (!mPaintData.mLayerCriticalDisplayPort.IsEmpty()) {
      
      
      mValidRegion.And(mValidRegion, mPaintData.mLayerCriticalDisplayPort);
    }
  }

  nsIntRegion lowPrecisionInvalidRegion;
  if (!mPaintData.mLayerCriticalDisplayPort.IsEmpty()) {
    if (gfxPlatform::UseLowPrecisionBuffer()) {
      
      lowPrecisionInvalidRegion.Sub(mVisibleRegion, mLowPrecisionValidRegion);

      
      
      lowPrecisionInvalidRegion.Sub(lowPrecisionInvalidRegion, mValidRegion);
    }

    
    invalidRegion.And(invalidRegion, mPaintData.mLayerCriticalDisplayPort);
    if (invalidRegion.IsEmpty() && lowPrecisionInvalidRegion.IsEmpty()) {
      EndPaint(true);
      return;
    }
  }

  if (!invalidRegion.IsEmpty() && mPaintData.mLowPrecisionPaintCount == 0) {
    bool updatedBuffer = false;
    
    if (gfxPlatform::UseProgressiveTilePainting() &&
        !ClientManager()->HasShadowTarget() &&
        mContentClient->mTiledBuffer.GetFrameResolution() == mPaintData.mResolution) {
      
      
      
      nsIntRegion oldValidRegion = mContentClient->mTiledBuffer.GetValidRegion();
      oldValidRegion.And(oldValidRegion, mVisibleRegion);
      if (!mPaintData.mLayerCriticalDisplayPort.IsEmpty()) {
        oldValidRegion.And(oldValidRegion, mPaintData.mLayerCriticalDisplayPort);
      }

      updatedBuffer =
        mContentClient->mTiledBuffer.ProgressiveUpdate(mValidRegion, invalidRegion,
                                                       oldValidRegion, &mPaintData,
                                                       callback, data);
    } else {
      updatedBuffer = true;
      mValidRegion = mVisibleRegion;
      if (!mPaintData.mLayerCriticalDisplayPort.IsEmpty()) {
        mValidRegion.And(mValidRegion, mPaintData.mLayerCriticalDisplayPort);
      }
      mContentClient->mTiledBuffer.SetFrameResolution(mPaintData.mResolution);
      mContentClient->mTiledBuffer.PaintThebes(mValidRegion, invalidRegion,
                                               callback, data);
    }

    if (updatedBuffer) {
      mPaintData.mFirstPaint = false;
      ClientManager()->Hold(this);
      mContentClient->LockCopyAndWrite(TiledContentClient::TILED_BUFFER);

      
      
      if (!lowPrecisionInvalidRegion.IsEmpty() && mPaintData.mPaintFinished) {
        ClientManager()->SetRepeatTransaction();
        mPaintData.mLowPrecisionPaintCount = 1;
        mPaintData.mPaintFinished = false;
      }

      
      
      EndPaint(false);
      return;
    }
  }

  
  
  bool updatedLowPrecision = false;
  if (!lowPrecisionInvalidRegion.IsEmpty() &&
      !nsIntRegion(mPaintData.mLayerCriticalDisplayPort).Contains(mVisibleRegion)) {
    nsIntRegion oldValidRegion =
      mContentClient->mLowPrecisionTiledBuffer.GetValidRegion();
    oldValidRegion.And(oldValidRegion, mVisibleRegion);

    
    if (mContentClient->mLowPrecisionTiledBuffer.GetFrameResolution() != mPaintData.mResolution ||
        mContentClient->mLowPrecisionTiledBuffer.HasFormatChanged()) {
      if (!mLowPrecisionValidRegion.IsEmpty()) {
        updatedLowPrecision = true;
      }
      oldValidRegion.SetEmpty();
      mLowPrecisionValidRegion.SetEmpty();
      mContentClient->mLowPrecisionTiledBuffer.SetFrameResolution(mPaintData.mResolution);
      lowPrecisionInvalidRegion = mVisibleRegion;
    }

    
    if (mPaintData.mLowPrecisionPaintCount == 1) {
      mLowPrecisionValidRegion.And(mLowPrecisionValidRegion, mVisibleRegion);
    }
    mPaintData.mLowPrecisionPaintCount++;

    
    
    lowPrecisionInvalidRegion.Sub(lowPrecisionInvalidRegion, mValidRegion);

    if (!lowPrecisionInvalidRegion.IsEmpty()) {
      updatedLowPrecision = mContentClient->mLowPrecisionTiledBuffer
                              .ProgressiveUpdate(mLowPrecisionValidRegion,
                                                 lowPrecisionInvalidRegion,
                                                 oldValidRegion, &mPaintData,
                                                 callback, data);
    }
  } else if (!mLowPrecisionValidRegion.IsEmpty()) {
    
    updatedLowPrecision = true;
    mLowPrecisionValidRegion.SetEmpty();
    mContentClient->mLowPrecisionTiledBuffer.PaintThebes(mLowPrecisionValidRegion,
                                                         mLowPrecisionValidRegion,
                                                         callback, data);
  }

  
  
  
  if (updatedLowPrecision) {
    ClientManager()->Hold(this);
    mContentClient->LockCopyAndWrite(TiledContentClient::LOW_PRECISION_TILED_BUFFER);
  }

  EndPaint(false);
}

} 
} 
