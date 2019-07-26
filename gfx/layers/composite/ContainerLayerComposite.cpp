




#include "ContainerLayerComposite.h"
#include <algorithm>                    
#include "FrameMetrics.h"               
#include "Units.h"                      
#include "gfx2DGlue.h"                  
#include "gfx3DMatrix.h"                
#include "gfxImageSurface.h"            
#include "gfxMatrix.h"                  
#include "gfxPlatform.h"                
#include "gfxUtils.h"                   
#include "mozilla/Assertions.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/gfx/BaseRect.h"       
#include "mozilla/gfx/Matrix.h"         
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Rect.h"           
#include "mozilla/layers/Compositor.h"  
#include "mozilla/layers/CompositorTypes.h"  
#include "mozilla/layers/Effects.h"     
#include "mozilla/layers/TextureHost.h"  
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsDebug.h"                    
#include "nsISupportsUtils.h"           
#include "nsPoint.h"                    
#include "nsRect.h"                     
#include "nsRegion.h"                   
#include "nsTArray.h"                   
#include "nsTraceRefcnt.h"              

namespace mozilla {
namespace layers {


static bool
HasOpaqueAncestorLayer(Layer* aLayer)
{
  for (Layer* l = aLayer->GetParent(); l; l = l->GetParent()) {
    if (l->GetContentFlags() & Layer::CONTENT_OPAQUE)
      return true;
  }
  return false;
}





static nsIntRect
GetOpaqueRect(Layer* aLayer)
{
  nsIntRect result;
  
  if (!aLayer->GetEffectiveTransform().IsIdentity() ||
      aLayer->GetEffectiveOpacity() != 1.0f ||
      aLayer->GetMaskLayer()) {
    return result;
  }
  if (aLayer->GetContentFlags() & Layer::CONTENT_OPAQUE) {
    result = aLayer->GetEffectiveVisibleRegion().GetLargestRectangle();
  } else {
    
    
    
    RefLayer* refLayer = aLayer->AsRefLayer();
    if (refLayer && refLayer->GetFirstChild()) {
      result = GetOpaqueRect(refLayer->GetFirstChild());
    }
  }
  const nsIntRect* clipRect = aLayer->GetEffectiveClipRect();
  if (clipRect) {
    result.IntersectRect(result, *clipRect);
  }
  return result;
}

template<class ContainerT> void
ContainerRender(ContainerT* aContainer,
                const nsIntPoint& aOffset,
                LayerManagerComposite* aManager,
                const nsIntRect& aClipRect)
{
  


  RefPtr<CompositingRenderTarget> surface;

  Compositor* compositor = aManager->GetCompositor();

  RefPtr<CompositingRenderTarget> previousTarget = compositor->GetCurrentRenderTarget();

  nsIntPoint childOffset(aOffset);
  nsIntRect visibleRect = aContainer->GetEffectiveVisibleRegion().GetBounds();

  aContainer->mSupportsComponentAlphaChildren = false;

  float opacity = aContainer->GetEffectiveOpacity();

  bool needsSurface = aContainer->UseIntermediateSurface();
  if (needsSurface) {
    SurfaceInitMode mode = INIT_MODE_CLEAR;
    bool surfaceCopyNeeded = false;
    gfx::IntRect surfaceRect = gfx::IntRect(visibleRect.x, visibleRect.y,
                                            visibleRect.width, visibleRect.height);
    
    
    
    
    
    
    int32_t maxTextureSize = compositor->GetMaxTextureSize();
    surfaceRect.width = std::min(maxTextureSize, surfaceRect.width);
    surfaceRect.height = std::min(maxTextureSize, surfaceRect.height);
    if (aContainer->GetEffectiveVisibleRegion().GetNumRects() == 1 &&
        (aContainer->GetContentFlags() & Layer::CONTENT_OPAQUE))
    {
      
      aContainer->mSupportsComponentAlphaChildren = true;
      mode = INIT_MODE_NONE;
    } else {
      const gfx3DMatrix& transform3D = aContainer->GetEffectiveTransform();
      gfxMatrix transform;
      
      
      
      
      if (HasOpaqueAncestorLayer(aContainer) &&
          transform3D.Is2D(&transform) && !transform.HasNonIntegerTranslation()) {
        mode = gfxPlatform::ComponentAlphaEnabled() ?
                                            INIT_MODE_COPY : INIT_MODE_CLEAR;
        surfaceCopyNeeded = (mode == INIT_MODE_COPY);
        surfaceRect.x += transform.x0;
        surfaceRect.y += transform.y0;
        aContainer->mSupportsComponentAlphaChildren
          = gfxPlatform::ComponentAlphaEnabled();
      }
    }

    surfaceRect -= gfx::IntPoint(aOffset.x, aOffset.y);
    if (surfaceCopyNeeded) {
      surface = compositor->CreateRenderTargetFromSource(surfaceRect, previousTarget);
    } else {
      surface = compositor->CreateRenderTarget(surfaceRect, mode);
    }

    if (!surface) {
      return;
    }

    compositor->SetRenderTarget(surface);
    childOffset.x = visibleRect.x;
    childOffset.y = visibleRect.y;
  } else {
    surface = previousTarget;
    aContainer->mSupportsComponentAlphaChildren = (aContainer->GetContentFlags() & Layer::CONTENT_OPAQUE) ||
      (aContainer->GetParent() && aContainer->GetParent()->SupportsComponentAlphaChildren());
  }

  nsAutoTArray<Layer*, 12> children;
  aContainer->SortChildrenBy3DZOrder(children);

  


  for (uint32_t i = 0; i < children.Length(); i++) {
    LayerComposite* layerToRender = static_cast<LayerComposite*>(children.ElementAt(i)->ImplData());

    if (layerToRender->GetLayer()->GetEffectiveVisibleRegion().IsEmpty() &&
        !layerToRender->GetLayer()->AsContainerLayer()) {
      continue;
    }

    if (i + 1 < children.Length() &&
        layerToRender->GetLayer()->GetEffectiveTransform().IsIdentity()) {
      LayerComposite* nextLayer = static_cast<LayerComposite*>(children.ElementAt(i + 1)->ImplData());
      nsIntRect nextLayerOpaqueRect;
      if (nextLayer && nextLayer->GetLayer()) {
        nextLayerOpaqueRect = GetOpaqueRect(nextLayer->GetLayer());
      }
      if (!nextLayerOpaqueRect.IsEmpty()) {
        nsIntRegion visibleRegion;
        visibleRegion.Sub(layerToRender->GetShadowVisibleRegion(), nextLayerOpaqueRect);
        layerToRender->SetShadowVisibleRegion(visibleRegion);
        if (visibleRegion.IsEmpty()) {
          continue;
        }
      }
    }

    nsIntRect clipRect = layerToRender->GetLayer()->
        CalculateScissorRect(aClipRect, &aManager->GetWorldTransform());
    if (clipRect.IsEmpty()) {
      continue;
    }

    layerToRender->RenderLayer(childOffset, clipRect);
    
    
  }

  if (needsSurface) {
    
#ifdef MOZ_DUMP_PAINTING
    if (gfxUtils::sDumpPainting) {
      nsRefPtr<gfxImageSurface> surf = surface->Dump(aManager->GetCompositor());
      WriteSnapshotToDumpFile(aContainer, surf);
    }
#endif

    compositor->SetRenderTarget(previousTarget);
    EffectChain effectChain;
    LayerManagerComposite::AutoAddMaskEffect autoMaskEffect(aContainer->GetMaskLayer(),
                                                            effectChain,
                                                            !aContainer->GetTransform().CanDraw2D());

    effectChain.mPrimaryEffect = new EffectRenderTarget(surface);

    gfx::Matrix4x4 transform;
    ToMatrix4x4(aContainer->GetEffectiveTransform(), transform);

    gfx::Rect rect(visibleRect.x, visibleRect.y, visibleRect.width, visibleRect.height);
    gfx::Rect clipRect(aClipRect.x, aClipRect.y, aClipRect.width, aClipRect.height);
    aManager->GetCompositor()->DrawQuad(rect, clipRect, effectChain, opacity,
                                        transform, gfx::Point(aOffset.x, aOffset.y));
  }

  if (aContainer->GetFrameMetrics().IsScrollable()) {
    gfx::Matrix4x4 transform;
    ToMatrix4x4(aContainer->GetEffectiveTransform(), transform);

    const FrameMetrics& frame = aContainer->GetFrameMetrics();
    LayerRect layerBounds = ScreenRect(frame.mCompositionBounds) * ScreenToLayerScale(1.0);
    gfx::Rect rect(layerBounds.x, layerBounds.y, layerBounds.width, layerBounds.height);
    gfx::Rect clipRect(aClipRect.x, aClipRect.y, aClipRect.width, aClipRect.height);
    aManager->GetCompositor()->DrawDiagnostics(DIAGNOSTIC_CONTAINER,
                                               rect, clipRect,
                                               transform, gfx::Point(aOffset.x, aOffset.y));
  }
}

ContainerLayerComposite::ContainerLayerComposite(LayerManagerComposite *aManager)
  : ContainerLayer(aManager, nullptr)
  , LayerComposite(aManager)
{
  MOZ_COUNT_CTOR(ContainerLayerComposite);
  mImplData = static_cast<LayerComposite*>(this);
}

ContainerLayerComposite::~ContainerLayerComposite()
{
  MOZ_COUNT_DTOR(ContainerLayerComposite);

  
  
  
  
  
  
  
  
  
  while (mFirstChild) {
    RemoveChild(mFirstChild);
  }
}

void
ContainerLayerComposite::Destroy()
{
  if (!mDestroyed) {
    while (mFirstChild) {
      static_cast<LayerComposite*>(GetFirstChild()->ImplData())->Destroy();
      RemoveChild(mFirstChild);
    }
    mDestroyed = true;
  }
}

LayerComposite*
ContainerLayerComposite::GetFirstChildComposite()
{
  if (!mFirstChild) {
    return nullptr;
   }
  return static_cast<LayerComposite*>(mFirstChild->ImplData());
}

void
ContainerLayerComposite::RenderLayer(const nsIntPoint& aOffset,
                                     const nsIntRect& aClipRect)
{
  ContainerRender(this, aOffset, mCompositeManager, aClipRect);
}

void
ContainerLayerComposite::CleanupResources()
{
  for (Layer* l = GetFirstChild(); l; l = l->GetNextSibling()) {
    LayerComposite* layerToCleanup = static_cast<LayerComposite*>(l->ImplData());
    layerToCleanup->CleanupResources();
  }
}

RefLayerComposite::RefLayerComposite(LayerManagerComposite* aManager)
  : RefLayer(aManager, nullptr)
  , LayerComposite(aManager)
{
  mImplData = static_cast<LayerComposite*>(this);
}

RefLayerComposite::~RefLayerComposite()
{
  Destroy();
}

void
RefLayerComposite::Destroy()
{
  MOZ_ASSERT(!mFirstChild);
  mDestroyed = true;
}

LayerComposite*
RefLayerComposite::GetFirstChildComposite()
{
  if (!mFirstChild) {
    return nullptr;
   }
  return static_cast<LayerComposite*>(mFirstChild->ImplData());
}

void
RefLayerComposite::RenderLayer(const nsIntPoint& aOffset,
                               const nsIntRect& aClipRect)
{
  ContainerRender(this, aOffset, mCompositeManager, aClipRect);
}

void
RefLayerComposite::CleanupResources()
{
}

} 
} 
