




#include "ipc/AutoOpenSurface.h"
#include "mozilla/layers/PLayerTransaction.h"
#include "TiledLayerBuffer.h"



#include "mozilla/Util.h"

#include "mozilla/layers/ShadowLayers.h"

#include "ThebesLayerBuffer.h"
#include "ThebesLayerComposite.h"
#include "mozilla/layers/ContentHost.h"
#include "gfxUtils.h"
#include "gfx2DGlue.h"

#include "mozilla/layers/CompositorTypes.h" 
#include "mozilla/layers/Effects.h"

namespace mozilla {
namespace layers {

ThebesLayerComposite::ThebesLayerComposite(LayerManagerComposite *aManager)
  : ThebesLayer(aManager, nullptr)
  , LayerComposite(aManager)
  , mBuffer(nullptr)
{
  MOZ_COUNT_CTOR(ThebesLayerComposite);
  mImplData = static_cast<LayerComposite*>(this);
}

ThebesLayerComposite::~ThebesLayerComposite()
{
  MOZ_COUNT_DTOR(ThebesLayerComposite);
  if (mBuffer) {
    mBuffer->Detach();
  }
}

void
ThebesLayerComposite::SetCompositableHost(CompositableHost* aHost)
{
  mBuffer= static_cast<ContentHost*>(aHost);
}

void
ThebesLayerComposite::Disconnect()
{
  Destroy();
}

void
ThebesLayerComposite::Destroy()
{
  if (!mDestroyed) {
    if (mBuffer) {
      mBuffer->Detach();
    }
    mBuffer = nullptr;
    mDestroyed = true;
  }
}

Layer*
ThebesLayerComposite::GetLayer()
{
  return this;
}

TiledLayerComposer*
ThebesLayerComposite::GetTiledLayerComposer()
{
  return mBuffer->AsTiledLayerComposer();
}

LayerRenderState
ThebesLayerComposite::GetRenderState()
{
  if (!mBuffer || mDestroyed) {
    return LayerRenderState();
  }
  return mBuffer->GetRenderState();
}

void
ThebesLayerComposite::RenderLayer(const nsIntPoint& aOffset,
                                  const nsIntRect& aClipRect)
{
  if (!mBuffer) {
    return;
  }

  gfx::Matrix4x4 transform;
  ToMatrix4x4(GetEffectiveTransform(), transform);
  gfx::Rect clipRect(aClipRect.x, aClipRect.y, aClipRect.width, aClipRect.height);

#ifdef MOZ_DUMP_PAINTING
  if (gfxUtils::sDumpPainting) {
    nsRefPtr<gfxImageSurface> surf = mBuffer->Dump();
    WriteSnapshotToDumpFile(this, surf);
  }
#endif

  EffectChain effectChain;
  LayerManagerComposite::AddMaskEffect(mMaskLayer, effectChain);

  nsIntRegion visibleRegion = GetEffectiveVisibleRegion();

  TiledLayerProperties tiledLayerProps;
  if (mRequiresTiledProperties) {
    
    
    tiledLayerProps.mVisibleRegion = visibleRegion;
    tiledLayerProps.mDisplayPort = GetDisplayPort();
    tiledLayerProps.mEffectiveResolution = GetEffectiveResolution();
    tiledLayerProps.mCompositionBounds = GetCompositionBounds();
    tiledLayerProps.mRetainTiles = !mIsFixedPosition;
    tiledLayerProps.mValidRegion = mValidRegion;
  }

  mBuffer->Composite(effectChain,
                     GetEffectiveOpacity(),
                     transform,
                     gfx::Point(aOffset.x, aOffset.y),
                     gfx::FILTER_LINEAR,
                     clipRect,
                     &visibleRegion,
                     mRequiresTiledProperties ? &tiledLayerProps
                                              : nullptr);


  if (mRequiresTiledProperties) {
    mValidRegion = tiledLayerProps.mValidRegion;
  }

  mCompositeManager->GetCompositor()->MakeCurrent();
}

CompositableHost*
ThebesLayerComposite::GetCompositableHost() {
  return mBuffer.get();
}

void
ThebesLayerComposite::CleanupResources()
{
  mBuffer = nullptr;
}

gfxSize
ThebesLayerComposite::GetEffectiveResolution()
{
  
  
  
  
  
  gfxSize resolution(1, 1);
  for (ContainerLayer* parent = GetParent(); parent; parent = parent->GetParent()) {
    const FrameMetrics& metrics = parent->GetFrameMetrics();
    resolution.width *= metrics.mResolution.width;
    resolution.height *= metrics.mResolution.height;
  }

  return resolution;
}

gfxRect
ThebesLayerComposite::GetDisplayPort()
{
  
  
  
  
  gfx3DMatrix transform = GetTransform();

  
  
  gfxRect displayPort;
  gfxSize parentResolution = GetEffectiveResolution();
  for (ContainerLayer* parent = GetParent(); parent; parent = parent->GetParent()) {
    const FrameMetrics& metrics = parent->GetFrameMetrics();
    if (displayPort.IsEmpty()) {
      if (!metrics.mDisplayPort.IsEmpty()) {
          
          
          
          
          displayPort = gfxRect(metrics.mDisplayPort.x,
                                metrics.mDisplayPort.y,
                                metrics.mDisplayPort.width,
                                metrics.mDisplayPort.height);
          displayPort.ScaleRoundOut(parentResolution.width, parentResolution.height);
      }
      parentResolution.width /= metrics.mResolution.width;
      parentResolution.height /= metrics.mResolution.height;
    }
    if (parent->UseIntermediateSurface()) {
      transform.PreMultiply(parent->GetTransform());
    }
  }

  
  if (displayPort.IsEmpty()) {
    LayerManagerComposite* manager = static_cast<LayerManagerComposite*>(Manager());
    const nsIntSize& widgetSize = manager->GetWidgetSize();
    displayPort.width = widgetSize.width;
    displayPort.height = widgetSize.height;
  }

  
  displayPort = transform.Inverse().TransformBounds(displayPort);

  return displayPort;
}

gfxRect
ThebesLayerComposite::GetCompositionBounds()
{
  
  
  
  
  
  gfxRect compositionBounds;
  ContainerLayer* scrollableLayer = nullptr;
  for (ContainerLayer* parent = GetParent(); parent; parent = parent->GetParent()) {
    const FrameMetrics& parentMetrics = parent->GetFrameMetrics();
    if (parentMetrics.IsScrollable())
      scrollableLayer = parent;
    if (!parentMetrics.mDisplayPort.IsEmpty() && scrollableLayer) {
      
      compositionBounds = gfxRect(parentMetrics.mCompositionBounds);

      
      
      Layer* rootLayer = Manager()->GetRoot();
      const gfx3DMatrix& rootTransform = rootLayer->GetTransform();
      float scaleX = rootTransform.GetXScale();
      float scaleY = rootTransform.GetYScale();

      
      const FrameMetrics& metrics = scrollableLayer->GetFrameMetrics();
      const nsIntSize& contentSize = metrics.mContentRect.Size();
      gfx::Point scrollOffset =
        gfx::Point((metrics.mScrollOffset.x * metrics.LayersPixelsPerCSSPixel().width) / scaleX,
                   (metrics.mScrollOffset.y * metrics.LayersPixelsPerCSSPixel().height) / scaleY);
      const nsIntPoint& contentOrigin = metrics.mContentRect.TopLeft() -
        nsIntPoint(NS_lround(scrollOffset.x), NS_lround(scrollOffset.y));
      gfxRect contentRect = gfxRect(contentOrigin.x, contentOrigin.y,
                                    contentSize.width, contentSize.height);
      gfxRect contentBounds = scrollableLayer->GetEffectiveTransform().
        TransformBounds(contentRect);

      
      compositionBounds.IntersectRect(compositionBounds, contentBounds);
      break;
    }
  }

  return compositionBounds;
}

#ifdef MOZ_LAYERS_HAVE_LOG
nsACString&
ThebesLayerComposite::PrintInfo(nsACString& aTo, const char* aPrefix)
{
  ThebesLayer::PrintInfo(aTo, aPrefix);
  aTo += "\n";
  if (mBuffer) {
    nsAutoCString pfx(aPrefix);
    pfx += "  ";
    mBuffer->PrintInfo(aTo, pfx.get());
  }
  return aTo;
}
#endif

} 
} 
