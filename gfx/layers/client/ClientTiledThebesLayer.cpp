



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

namespace mozilla {
namespace layers {


ClientTiledThebesLayer::ClientTiledThebesLayer(ClientLayerManager* const aManager)
  : ThebesLayer(aManager,
                static_cast<ClientLayer*>(MOZ_THIS_IN_INITIALIZER_LIST()))
  , mContentClient()
{
  MOZ_COUNT_CTOR(ClientTiledThebesLayer);
  mPaintData.mLastScrollOffset = ScreenPoint(0, 0);
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

static LayoutDeviceRect
ApplyParentLayerToLayoutTransform(const gfx3DMatrix& aTransform, const ParentLayerRect& aParentLayerRect)
{
  return TransformTo<LayoutDevicePixel>(aTransform, aParentLayerRect);
}

void
ClientTiledThebesLayer::BeginPaint()
{
  if (ClientManager()->IsRepeatTransaction()) {
    return;
  }

  mPaintData.mLowPrecisionPaintCount = 0;
  mPaintData.mPaintFinished = false;

  
  ContainerLayer* scrollParent = nullptr;
  for (ContainerLayer* parent = GetParent(); parent; parent = parent->GetParent()) {
    const FrameMetrics& metrics = parent->GetFrameMetrics();
    if (metrics.mScrollId != FrameMetrics::NULL_SCROLL_ID) {
      scrollParent = parent;
      break;
    }
  }

  if (!scrollParent) {
    
    
    NS_WARNING("Tiled Thebes layer with no scrollable container parent");
    mPaintData.mCompositionBounds.SetEmpty();
    return;
  }

  const FrameMetrics& metrics = scrollParent->GetFrameMetrics();

  
  
  gfx::Matrix4x4 effectiveTransform = GetEffectiveTransform();
  for (ContainerLayer* parent = GetParent(); parent; parent = parent->GetParent()) {
    if (parent->UseIntermediateSurface()) {
      effectiveTransform = effectiveTransform * parent->GetEffectiveTransform();
    }
  }
  gfx3DMatrix layoutToParentLayer;
  gfx::To3DMatrix(effectiveTransform, layoutToParentLayer);
  layoutToParentLayer.ScalePost(metrics.GetParentResolution().scale,
                                metrics.GetParentResolution().scale,
                                1.f);

  mPaintData.mTransformParentLayerToLayout = layoutToParentLayer.Inverse();

  
  mPaintData.mLayoutCriticalDisplayPort.SetEmpty();
  if (!metrics.mCriticalDisplayPort.IsEmpty()) {
    
    
    const ParentLayerRect& criticalDisplayPort = metrics.mCriticalDisplayPort
                                               * metrics.mDevPixelsPerCSSPixel
                                               * metrics.GetParentResolution();
    LayoutDeviceRect transformedCriticalDisplayPort =
      ApplyParentLayerToLayoutTransform(mPaintData.mTransformParentLayerToLayout, criticalDisplayPort);
    mPaintData.mLayoutCriticalDisplayPort =
      LayoutDeviceIntRect::ToUntyped(RoundedOut(transformedCriticalDisplayPort));
  }

  
  
  mPaintData.mResolution = metrics.mZoom;

  
  
  mPaintData.mCompositionBounds.SetEmpty();
  mPaintData.mScrollOffset.MoveTo(0, 0);
  Layer* primaryScrollable = ClientManager()->GetPrimaryScrollableLayer();
  if (primaryScrollable) {
    const FrameMetrics& metrics = primaryScrollable->AsContainerLayer()->GetFrameMetrics();
    mPaintData.mScrollOffset = metrics.GetScrollOffset() * metrics.mZoom;
    mPaintData.mCompositionBounds =
      ApplyParentLayerToLayoutTransform(mPaintData.mTransformParentLayerToLayout,
                                        ParentLayerRect(metrics.mCompositionBounds));
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
  mPaintData.mFirstPaint = false;
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

  nsIntRegion invalidRegion = mVisibleRegion;
  invalidRegion.Sub(invalidRegion, mValidRegion);
  if (invalidRegion.IsEmpty()) {
    EndPaint(true);
    return;
  }

  
  if (GetMaskLayer() && !ClientManager()->IsRepeatTransaction()) {
    ToClientLayer(GetMaskLayer())->RenderLayer();
  }

  
  
  const FrameMetrics& parentMetrics = GetParent()->GetFrameMetrics();
  if ((!gfxPrefs::UseProgressiveTilePainting() &&
       !gfxPrefs::UseLowPrecisionBuffer() &&
       parentMetrics.mCriticalDisplayPort.IsEmpty()) ||
       parentMetrics.mDisplayPort.IsEmpty()) {
    mValidRegion = mVisibleRegion;

    NS_ASSERTION(!ClientManager()->IsRepeatTransaction(), "Didn't paint our mask layer");

    mContentClient->mTiledBuffer.PaintThebes(mValidRegion, invalidRegion,
                                             callback, data);

    ClientManager()->Hold(this);
    mContentClient->UseTiledLayerBuffer(TiledContentClient::TILED_BUFFER);

    return;
  }

  
  BeginPaint();
  if (mPaintData.mPaintFinished) {
    return;
  }

  
  
  if (!ClientManager()->IsRepeatTransaction()) {
    mValidRegion.And(mValidRegion, mVisibleRegion);
    if (!mPaintData.mLayoutCriticalDisplayPort.IsEmpty()) {
      
      
      mValidRegion.And(mValidRegion, mPaintData.mLayoutCriticalDisplayPort);
    }
  }

  nsIntRegion lowPrecisionInvalidRegion;
  if (!mPaintData.mLayoutCriticalDisplayPort.IsEmpty()) {
    if (gfxPrefs::UseLowPrecisionBuffer()) {
      
      lowPrecisionInvalidRegion.Sub(mVisibleRegion, mLowPrecisionValidRegion);

      
      
      lowPrecisionInvalidRegion.Sub(lowPrecisionInvalidRegion, mValidRegion);
    }

    
    invalidRegion.And(invalidRegion, mPaintData.mLayoutCriticalDisplayPort);
    if (invalidRegion.IsEmpty() && lowPrecisionInvalidRegion.IsEmpty()) {
      EndPaint(true);
      return;
    }
  }

  if (!invalidRegion.IsEmpty() && mPaintData.mLowPrecisionPaintCount == 0) {
    bool updatedBuffer = false;
    
    if (gfxPrefs::UseProgressiveTilePainting() &&
        !ClientManager()->HasShadowTarget() &&
        mContentClient->mTiledBuffer.GetFrameResolution() == mPaintData.mResolution) {
      
      
      
      nsIntRegion oldValidRegion = mContentClient->mTiledBuffer.GetValidRegion();
      oldValidRegion.And(oldValidRegion, mVisibleRegion);
      if (!mPaintData.mLayoutCriticalDisplayPort.IsEmpty()) {
        oldValidRegion.And(oldValidRegion, mPaintData.mLayoutCriticalDisplayPort);
      }

      updatedBuffer =
        mContentClient->mTiledBuffer.ProgressiveUpdate(mValidRegion, invalidRegion,
                                                       oldValidRegion, &mPaintData,
                                                       callback, data);
    } else {
      updatedBuffer = true;
      mValidRegion = mVisibleRegion;
      if (!mPaintData.mLayoutCriticalDisplayPort.IsEmpty()) {
        mValidRegion.And(mValidRegion, mPaintData.mLayoutCriticalDisplayPort);
      }
      mContentClient->mTiledBuffer.SetFrameResolution(mPaintData.mResolution);
      mContentClient->mTiledBuffer.PaintThebes(mValidRegion, invalidRegion,
                                               callback, data);
    }

    if (updatedBuffer) {
      ClientManager()->Hold(this);
      mContentClient->UseTiledLayerBuffer(TiledContentClient::TILED_BUFFER);

      
      
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
      !nsIntRegion(mPaintData.mLayoutCriticalDisplayPort).Contains(mVisibleRegion)) {
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
    mContentClient->UseTiledLayerBuffer(TiledContentClient::LOW_PRECISION_TILED_BUFFER);
  }

  EndPaint(false);
}

} 
} 
