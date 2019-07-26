




#include "ThebesLayerComposite.h"
#include "CompositableHost.h"           
#include "FrameMetrics.h"               
#include "Units.h"                      
#include "gfx2DGlue.h"                  
#include "gfx3DMatrix.h"                
#include "gfxImageSurface.h"            
#include "gfxUtils.h"                   
#include "mozilla/Assertions.h"         
#include "mozilla/gfx/Matrix.h"         
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Rect.h"           
#include "mozilla/gfx/Types.h"          
#include "mozilla/layers/Compositor.h"  
#include "mozilla/layers/ContentHost.h"  
#include "mozilla/layers/Effects.h"     
#include "mozilla/mozalloc.h"           
#include "nsAString.h"
#include "nsAutoPtr.h"                  
#include "nsMathUtils.h"                
#include "nsPoint.h"                    
#include "nsRect.h"                     
#include "nsSize.h"                     
#include "nsString.h"                   
#include "nsTraceRefcnt.h"              
#include "GeckoProfiler.h"

namespace mozilla {
namespace layers {

class TiledLayerComposer;

ThebesLayerComposite::ThebesLayerComposite(LayerManagerComposite *aManager)
  : ThebesLayer(aManager, nullptr)
  , LayerComposite(aManager)
  , mBuffer(nullptr)
  , mRequiresTiledProperties(false)
{
  MOZ_COUNT_CTOR(ThebesLayerComposite);
  mImplData = static_cast<LayerComposite*>(this);
}

ThebesLayerComposite::~ThebesLayerComposite()
{
  MOZ_COUNT_DTOR(ThebesLayerComposite);
  CleanupResources();
}

void
ThebesLayerComposite::SetCompositableHost(CompositableHost* aHost)
{
  mBuffer = static_cast<ContentHost*>(aHost);
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
    CleanupResources();
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
  MOZ_ASSERT(mBuffer && mBuffer->IsAttached());
  return mBuffer->AsTiledLayerComposer();
}

LayerRenderState
ThebesLayerComposite::GetRenderState()
{
  if (!mBuffer || !mBuffer->IsAttached() || mDestroyed) {
    return LayerRenderState();
  }
  return mBuffer->GetRenderState();
}

void
ThebesLayerComposite::RenderLayer(const nsIntPoint& aOffset,
                                  const nsIntRect& aClipRect)
{
  if (!mBuffer || !mBuffer->IsAttached()) {
    return;
  }
  PROFILER_LABEL("ThebesLayerComposite", "RenderLayer");

  MOZ_ASSERT(mBuffer->GetCompositor() == mCompositeManager->GetCompositor() &&
             mBuffer->GetLayer() == this,
             "buffer is corrupted");

  gfx::Matrix4x4 transform;
  ToMatrix4x4(GetEffectiveTransform(), transform);
  gfx::Rect clipRect(aClipRect.x, aClipRect.y, aClipRect.width, aClipRect.height);

#ifdef MOZ_DUMP_PAINTING
  if (gfxUtils::sDumpPainting) {
    nsRefPtr<gfxImageSurface> surf = mBuffer->GetAsSurface();
    if (surf) {
      WriteSnapshotToDumpFile(this, surf);
    }
  }
#endif

  EffectChain effectChain;
  LayerManagerComposite::AutoAddMaskEffect autoMaskEffect(mMaskLayer, effectChain);

  nsIntRegion visibleRegion = GetEffectiveVisibleRegion();

  TiledLayerProperties tiledLayerProps;
  if (mRequiresTiledProperties) {
    
    
    tiledLayerProps.mVisibleRegion = visibleRegion;
    tiledLayerProps.mDisplayPort = GetDisplayPort();
    tiledLayerProps.mEffectiveResolution = GetEffectiveResolution();
    tiledLayerProps.mCompositionBounds = GetCompositionBounds();
    tiledLayerProps.mRetainTiles = !(mIsFixedPosition || mStickyPositionData);
    tiledLayerProps.mValidRegion = mValidRegion;
  }

  mBuffer->SetPaintWillResample(MayResample());

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
ThebesLayerComposite::GetCompositableHost()
{
  if (mBuffer && mBuffer->IsAttached()) {
    return mBuffer.get();
  }

  return nullptr;
}

void
ThebesLayerComposite::CleanupResources()
{
  if (mBuffer) {
    mBuffer->Detach(this);
  }
  mBuffer = nullptr;
}

gfxSize
ThebesLayerComposite::GetEffectiveResolution()
{
  
  
  
  
  
  gfxSize resolution(1, 1);
  for (ContainerLayer* parent = GetParent(); parent; parent = parent->GetParent()) {
    const FrameMetrics& metrics = parent->GetFrameMetrics();
    resolution.width *= metrics.mResolution.scale;
    resolution.height *= metrics.mResolution.scale;
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
      parentResolution.width /= metrics.mResolution.scale;
      parentResolution.height /= metrics.mResolution.scale;
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
      
      compositionBounds = gfxRect(parentMetrics.mCompositionBounds.x,
                                  parentMetrics.mCompositionBounds.y,
                                  parentMetrics.mCompositionBounds.width,
                                  parentMetrics.mCompositionBounds.height);

      
      
      Layer* rootLayer = Manager()->GetRoot();
      const gfx3DMatrix& rootTransform = rootLayer->GetTransform();
      LayerToCSSScale scale(rootTransform.GetXScale(),
                            rootTransform.GetYScale());

      
      const FrameMetrics& metrics = scrollableLayer->GetFrameMetrics();
      const LayerIntRect content = RoundedToInt(metrics.mScrollableRect / scale);
      
      gfx::Point scrollOffset =
        gfx::Point((metrics.mScrollOffset.x * metrics.LayersPixelsPerCSSPixel().scale) / scale.scale,
                   (metrics.mScrollOffset.y * metrics.LayersPixelsPerCSSPixel().scale) / scale.scale);
      const nsIntPoint contentOrigin(
        content.x - NS_lround(scrollOffset.x),
        content.y - NS_lround(scrollOffset.y));
      gfxRect contentRect = gfxRect(contentOrigin.x, contentOrigin.y,
                                    content.width, content.height);
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
  if (mBuffer && mBuffer->IsAttached()) {
    nsAutoCString pfx(aPrefix);
    pfx += "  ";
    mBuffer->PrintInfo(aTo, pfx.get());
  }
  return aTo;
}
#endif

} 
} 
