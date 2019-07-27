




#include "ContainerLayerComposite.h"
#include <algorithm>                    
#include "apz/src/AsyncPanZoomController.h"  
#include "FrameMetrics.h"               
#include "Units.h"                      
#include "gfx2DGlue.h"                  
#include "gfxPrefs.h"                   
#include "gfxUtils.h"                   
#include "mozilla/Assertions.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/UniquePtr.h"          
#include "mozilla/gfx/BaseRect.h"       
#include "mozilla/gfx/Matrix.h"         
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Rect.h"           
#include "mozilla/layers/Compositor.h"  
#include "mozilla/layers/CompositorTypes.h"  
#include "mozilla/layers/Effects.h"     
#include "mozilla/layers/TextureHost.h"  
#include "mozilla/layers/AsyncCompositionManager.h" 
#include "mozilla/layers/LayerMetricsWrapper.h" 
#include "mozilla/mozalloc.h"           
#include "nsRefPtr.h"                   
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsISupportsUtils.h"           
#include "nsRegion.h"                   
#include "nsTArray.h"                   
#include "TextRenderer.h"               
#include <vector>
#include "GeckoProfiler.h"              
#ifdef MOZ_ENABLE_PROFILER_SPS
#include "ProfilerMarkers.h"            
#endif

#define CULLING_LOG(...)


namespace mozilla {
namespace layers {

using namespace gfx;

static bool
LayerHasCheckerboardingAPZC(Layer* aLayer, gfxRGBA* aOutColor)
{
  for (LayerMetricsWrapper i(aLayer, LayerMetricsWrapper::StartAt::BOTTOM); i; i = i.GetParent()) {
    if (!i.Metrics().IsScrollable()) {
      continue;
    }
    if (i.GetApzc() && i.GetApzc()->IsCurrentlyCheckerboarding()) {
      if (aOutColor) {
        *aOutColor = i.Metrics().GetBackgroundColor();
      }
      return true;
    }
    break;
  }
  return false;
}

static void DrawLayerInfo(const RenderTargetIntRect& aClipRect,
                          LayerManagerComposite* aManager,
                          Layer* aLayer)
{

  if (aLayer->GetType() == Layer::LayerType::TYPE_CONTAINER) {
    
    
    
    return;
  }

  std::stringstream ss;
  aLayer->PrintInfo(ss, "");

  nsIntRegion visibleRegion = aLayer->GetVisibleRegion();

  uint32_t maxWidth = std::min<uint32_t>(visibleRegion.GetBounds().width, 500);

  IntPoint topLeft = visibleRegion.GetBounds().TopLeft();
  aManager->GetTextRenderer()->RenderText(ss.str().c_str(), topLeft,
                                          aLayer->GetEffectiveTransform(), 16,
                                          maxWidth);
}

template<class ContainerT>
static gfx::IntRect ContainerVisibleRect(ContainerT* aContainer)
{
  gfx::IntRect surfaceRect = aContainer->GetEffectiveVisibleRegion().GetBounds();
  return surfaceRect;
}

static void PrintUniformityInfo(Layer* aLayer)
{
#ifdef MOZ_ENABLE_PROFILER_SPS
  if (!profiler_is_active()) {
    return;
  }

  
  if (aLayer->GetEffectiveVisibleRegion().GetBounds().width < 300 ||
      aLayer->GetEffectiveVisibleRegion().GetBounds().height < 300) {
    return;
  }

  Matrix4x4 transform = aLayer->AsLayerComposite()->GetShadowTransform();
  if (!transform.Is2D()) {
    return;
  }

  Point translation = transform.As2D().GetTranslation();
  LayerTranslationPayload* payload = new LayerTranslationPayload(aLayer, translation);
  PROFILER_MARKER_PAYLOAD("LayerTranslation", payload);
#endif
}


struct PreparedLayer
{
  PreparedLayer(LayerComposite *aLayer, RenderTargetIntRect aClipRect) :
    mLayer(aLayer), mClipRect(aClipRect) {}
  LayerComposite* mLayer;
  RenderTargetIntRect mClipRect;
};


template<class ContainerT> void
ContainerRenderVR(ContainerT* aContainer,
                  LayerManagerComposite* aManager,
                  const gfx::IntRect& aClipRect,
                  gfx::VRHMDInfo* aHMD)
{
  RefPtr<CompositingRenderTarget> surface;

  Compositor* compositor = aManager->GetCompositor();

  RefPtr<CompositingRenderTarget> previousTarget = compositor->GetCurrentRenderTarget();

  gfx::IntRect visibleRect = aContainer->GetEffectiveVisibleRegion().GetBounds();

  float opacity = aContainer->GetEffectiveOpacity();

  gfx::IntRect surfaceRect = gfx::IntRect(visibleRect.x, visibleRect.y,
                                          visibleRect.width, visibleRect.height);
  
  
  
  
  
  
  int32_t maxTextureSize = compositor->GetMaxTextureSize();
  surfaceRect.width = std::min(maxTextureSize, surfaceRect.width);
  surfaceRect.height = std::min(maxTextureSize, surfaceRect.height);

  
  surface = compositor->CreateRenderTarget(surfaceRect, INIT_MODE_NONE);
  if (!surface) {
    return;
  }

  compositor->SetRenderTarget(surface);

  nsAutoTArray<Layer*, 12> children;
  aContainer->SortChildrenBy3DZOrder(children);

  


  gfx::IntRect surfaceClipRect(0, 0, surfaceRect.width, surfaceRect.height);
  RenderTargetIntRect rtClipRect(0, 0, surfaceRect.width, surfaceRect.height);
  for (uint32_t i = 0; i < children.Length(); i++) {
    LayerComposite* layerToRender = static_cast<LayerComposite*>(children.ElementAt(i)->ImplData());
    Layer* layer = layerToRender->GetLayer();

    if (layer->GetEffectiveVisibleRegion().IsEmpty() &&
        !layer->AsContainerLayer()) {
      continue;
    }

    RenderTargetIntRect clipRect = layer->CalculateScissorRect(rtClipRect);
    if (clipRect.IsEmpty()) {
      continue;
    }

    layerToRender->Prepare(rtClipRect);
    layerToRender->RenderLayer(surfaceClipRect);
  }

  
#ifdef MOZ_DUMP_PAINTING
  if (gfxUtils::sDumpPainting) {
    RefPtr<gfx::DataSourceSurface> surf = surface->Dump(aManager->GetCompositor());
    if (surf) {
      WriteSnapshotToDumpFile(aContainer, surf);
    }
  }
#endif

  compositor->SetRenderTarget(previousTarget);

  gfx::Rect rect(visibleRect.x, visibleRect.y, visibleRect.width, visibleRect.height);
  gfx::Rect clipRect(aClipRect.x, aClipRect.y, aClipRect.width, aClipRect.height);

  
  
  
  
  
  EffectChain solidEffect(aContainer);
  solidEffect.mPrimaryEffect = new EffectSolidColor(Color(0.0, 0.0, 0.0, 1.0));
  aManager->GetCompositor()->DrawQuad(rect, clipRect, solidEffect, opacity,
                                      aContainer->GetEffectiveTransform());

  
  EffectChain vrEffect(aContainer);
  vrEffect.mPrimaryEffect = new EffectVRDistortion(aHMD, surface);

  
  
  
  aManager->GetCompositor()->DrawQuad(rect, clipRect, vrEffect, opacity,
                                      aContainer->GetEffectiveTransform());
}


struct PreparedData
{
  RefPtr<CompositingRenderTarget> mTmpTarget;
  nsAutoTArray<PreparedLayer, 12> mLayers;
  bool mNeedsSurfaceCopy;
};


template<class ContainerT> void
ContainerPrepare(ContainerT* aContainer,
                 LayerManagerComposite* aManager,
                 const RenderTargetIntRect& aClipRect)
{
  aContainer->mPrepared = MakeUnique<PreparedData>();
  aContainer->mPrepared->mNeedsSurfaceCopy = false;

  gfx::VRHMDInfo *hmdInfo = aContainer->GetVRHMDInfo();
  if (hmdInfo && hmdInfo->GetConfiguration().IsValid()) {
    
    
    
    
    return;
  }

  


  nsAutoTArray<Layer*, 12> children;
  aContainer->SortChildrenBy3DZOrder(children);

  for (uint32_t i = 0; i < children.Length(); i++) {
    LayerComposite* layerToRender = static_cast<LayerComposite*>(children.ElementAt(i)->ImplData());

    RenderTargetIntRect clipRect = layerToRender->GetLayer()->
        CalculateScissorRect(aClipRect);

    if (!layerToRender->GetLayer()->AsContainerLayer()) {
      if (layerToRender->GetLayer()->GetEffectiveVisibleRegion().IsEmpty()) {
        CULLING_LOG("Sublayer %p has no effective visible region\n", layerToRender->GetLayer());
        continue;
      }

      if (clipRect.IsEmpty()) {
        CULLING_LOG("Sublayer %p has an empty world clip rect\n", layerToRender->GetLayer());
        continue;
      }
    }

    CULLING_LOG("Preparing sublayer %p\n", layerToRender->GetLayer());

    layerToRender->Prepare(clipRect);
    aContainer->mPrepared->mLayers.AppendElement(PreparedLayer(layerToRender, clipRect));
  }

  CULLING_LOG("Preparing container layer %p\n", aContainer->GetLayer());

  



  gfx::IntRect surfaceRect = ContainerVisibleRect(aContainer);
  if (surfaceRect.IsEmpty()) {
    return;
  }

  bool surfaceCopyNeeded;
  
  aContainer->DefaultComputeSupportsComponentAlphaChildren(&surfaceCopyNeeded);
  if (aContainer->UseIntermediateSurface()) {
    if (!surfaceCopyNeeded) {
      RefPtr<CompositingRenderTarget> surface = nullptr;

      RefPtr<CompositingRenderTarget>& lastSurf = aContainer->mLastIntermediateSurface;
      if (lastSurf && !aContainer->mChildrenChanged && lastSurf->GetRect().IsEqualEdges(surfaceRect)) {
        surface = lastSurf;
      }

      if (!surface) {
        
        
        surface = CreateOrRecycleTarget(aContainer, aManager);

        MOZ_PERFORMANCE_WARNING("gfx", "[%p] Container layer requires intermediate surface rendering\n", aContainer);
        RenderIntermediate(aContainer, aManager, RenderTargetPixel::ToUntyped(aClipRect), surface);
        aContainer->SetChildrenChanged(false);
      }

      aContainer->mPrepared->mTmpTarget = surface;
    } else {
      MOZ_PERFORMANCE_WARNING("gfx", "[%p] Container layer requires intermediate surface copy\n", aContainer);
      aContainer->mPrepared->mNeedsSurfaceCopy = true;
      aContainer->mLastIntermediateSurface = nullptr;
    }
  } else {
    aContainer->mLastIntermediateSurface = nullptr;
  }
}

template<class ContainerT> void
RenderLayers(ContainerT* aContainer,
	     LayerManagerComposite* aManager,
	     const RenderTargetIntRect& aClipRect)
{
  Compositor* compositor = aManager->GetCompositor();

  for (size_t i = 0u; i < aContainer->mPrepared->mLayers.Length(); i++) {
    PreparedLayer& preparedData = aContainer->mPrepared->mLayers[i];
    LayerComposite* layerToRender = preparedData.mLayer;
    const RenderTargetIntRect& clipRect = preparedData.mClipRect;
    Layer* layer = layerToRender->GetLayer();

    gfxRGBA color;
    if ((layer->GetContentFlags() & Layer::CONTENT_OPAQUE) &&
        LayerHasCheckerboardingAPZC(layer, &color)) {
      
      
      
      
      
      
      gfx::IntRect layerBounds = layer->GetLayerBounds();
      EffectChain effectChain(layer);
      effectChain.mPrimaryEffect = new EffectSolidColor(ToColor(color));
      aManager->GetCompositor()->DrawQuad(gfx::Rect(layerBounds.x, layerBounds.y, layerBounds.width, layerBounds.height),
                                          gfx::Rect(clipRect.ToUnknownRect()),
                                          effectChain, layer->GetEffectiveOpacity(),
                                          layer->GetEffectiveTransform());
    }

    if (layerToRender->HasLayerBeenComposited()) {
      
      
      layerToRender->SetLayerComposited(false);
      gfx::IntRect clearRect = layerToRender->GetClearRect();
      if (!clearRect.IsEmpty()) {
        
        gfx::Rect fbRect(clearRect.x, clearRect.y, clearRect.width, clearRect.height);
        compositor->ClearRect(fbRect);
        layerToRender->SetClearRect(gfx::IntRect(0, 0, 0, 0));
      }
    } else {
      layerToRender->RenderLayer(RenderTargetPixel::ToUntyped(clipRect));
    }

    if (gfxPrefs::UniformityInfo()) {
      PrintUniformityInfo(layer);
    }

    if (gfxPrefs::DrawLayerInfo()) {
      DrawLayerInfo(clipRect, aManager, layer);
    }

    
    
    
    
    
    
    
    Matrix4x4 asyncTransform;
    for (uint32_t i = layer->GetFrameMetricsCount(); i > 0; --i) {
      if (layer->GetFrameMetrics(i - 1).IsScrollable()) {
        
        
        ParentLayerRect compositionBounds = layer->GetFrameMetrics(i - 1).mCompositionBounds;
        aManager->GetCompositor()->DrawDiagnostics(DiagnosticFlags::CONTAINER,
                                                   compositionBounds.ToUnknownRect(),
                                                   gfx::Rect(aClipRect.ToUnknownRect()),
                                                   asyncTransform * aContainer->GetEffectiveTransform());
        if (AsyncPanZoomController* apzc = layer->GetAsyncPanZoomController(i - 1)) {
          asyncTransform = apzc->GetCurrentAsyncTransformWithOverscroll()
                         * asyncTransform;
        }
      }
    }

    
    
  }
}

template<class ContainerT> RefPtr<CompositingRenderTarget>
CreateOrRecycleTarget(ContainerT* aContainer,
                      LayerManagerComposite* aManager)
{
  Compositor* compositor = aManager->GetCompositor();
  SurfaceInitMode mode = INIT_MODE_CLEAR;
  gfx::IntRect surfaceRect = ContainerVisibleRect(aContainer);
  if (aContainer->GetEffectiveVisibleRegion().GetNumRects() == 1 &&
      (aContainer->GetContentFlags() & Layer::CONTENT_OPAQUE))
  {
    mode = INIT_MODE_NONE;
  }

  RefPtr<CompositingRenderTarget>& lastSurf = aContainer->mLastIntermediateSurface;
  if (lastSurf && lastSurf->GetRect().IsEqualEdges(surfaceRect)) {
    if (mode == INIT_MODE_CLEAR) {
      lastSurf->ClearOnBind();
    }

    return lastSurf;
  } else {
    lastSurf = compositor->CreateRenderTarget(surfaceRect, mode);

    return lastSurf;
  }
}

template<class ContainerT> RefPtr<CompositingRenderTarget>
CreateTemporaryTargetAndCopyFromBackground(ContainerT* aContainer,
                                           LayerManagerComposite* aManager)
{
  Compositor* compositor = aManager->GetCompositor();
  gfx::IntRect visibleRect = aContainer->GetEffectiveVisibleRegion().GetBounds();
  RefPtr<CompositingRenderTarget> previousTarget = compositor->GetCurrentRenderTarget();
  gfx::IntRect surfaceRect = gfx::IntRect(visibleRect.x, visibleRect.y,
                                          visibleRect.width, visibleRect.height);

  gfx::IntPoint sourcePoint = gfx::IntPoint(visibleRect.x, visibleRect.y);

  gfx::Matrix4x4 transform = aContainer->GetEffectiveTransform();
  DebugOnly<gfx::Matrix> transform2d;
  MOZ_ASSERT(transform.Is2D(&transform2d) && !gfx::ThebesMatrix(transform2d).HasNonIntegerTranslation());
  sourcePoint += gfx::IntPoint(transform._41, transform._42);

  sourcePoint -= compositor->GetCurrentRenderTarget()->GetOrigin();

  return compositor->CreateRenderTargetFromSource(surfaceRect, previousTarget, sourcePoint);
}

template<class ContainerT> void
RenderIntermediate(ContainerT* aContainer,
                   LayerManagerComposite* aManager,
                   const gfx::IntRect& aClipRect,
                   RefPtr<CompositingRenderTarget> surface)
{
  Compositor* compositor = aManager->GetCompositor();
  RefPtr<CompositingRenderTarget> previousTarget = compositor->GetCurrentRenderTarget();

  if (!surface) {
    return;
  }

  compositor->SetRenderTarget(surface);
  
  RenderLayers(aContainer, aManager, RenderTargetPixel::FromUntyped(aClipRect));
  
  compositor->SetRenderTarget(previousTarget);
}

template<class ContainerT> void
ContainerRender(ContainerT* aContainer,
                 LayerManagerComposite* aManager,
                 const gfx::IntRect& aClipRect)
{
  MOZ_ASSERT(aContainer->mPrepared);

  gfx::VRHMDInfo *hmdInfo = aContainer->GetVRHMDInfo();
  if (hmdInfo && hmdInfo->GetConfiguration().IsValid()) {
    ContainerRenderVR(aContainer, aManager, aClipRect, hmdInfo);
    aContainer->mPrepared = nullptr;
    return;
  }

  if (aContainer->UseIntermediateSurface()) {
    RefPtr<CompositingRenderTarget> surface;

    if (aContainer->mPrepared->mNeedsSurfaceCopy) {
      
      surface = CreateTemporaryTargetAndCopyFromBackground(aContainer, aManager);
      RenderIntermediate(aContainer, aManager,
                         aClipRect, surface);
    } else {
      surface = aContainer->mPrepared->mTmpTarget;
    }

    if (!surface) {
      aContainer->mPrepared = nullptr;
      return;
    }

    float opacity = aContainer->GetEffectiveOpacity();

    gfx::IntRect visibleRect = aContainer->GetEffectiveVisibleRegion().GetBounds();
#ifdef MOZ_DUMP_PAINTING
    if (gfxUtils::sDumpPainting) {
      RefPtr<gfx::DataSourceSurface> surf = surface->Dump(aManager->GetCompositor());
      if (surf) {
        WriteSnapshotToDumpFile(aContainer, surf);
      }
    }
#endif

    EffectChain effectChain(aContainer);
    LayerManagerComposite::AutoAddMaskEffect autoMaskEffect(aContainer->GetMaskLayer(),
                                                            effectChain,
                                                            !aContainer->GetTransform().CanDraw2D());
    if (autoMaskEffect.Failed()) {
      NS_WARNING("Failed to apply a mask effect.");
      return;
    }

    aContainer->AddBlendModeEffect(effectChain);
    effectChain.mPrimaryEffect = new EffectRenderTarget(surface);

    gfx::Rect rect(visibleRect.x, visibleRect.y, visibleRect.width, visibleRect.height);
    gfx::Rect clipRect(aClipRect.x, aClipRect.y, aClipRect.width, aClipRect.height);
    aManager->GetCompositor()->DrawQuad(rect, clipRect, effectChain, opacity,
                                        aContainer->GetEffectiveTransform());
  } else {
    RenderLayers(aContainer, aManager, RenderTargetPixel::FromUntyped(aClipRect));
  }
  aContainer->mPrepared = nullptr;

  
  
  
  
  if (gfxPrefs::LayersDrawFPS() && aContainer->IsScrollInfoLayer()) {
    
    
    
    for (LayerMetricsWrapper i(aContainer); i; i = i.GetFirstChild()) {
      if (AsyncPanZoomController* apzc = i.GetApzc()) {
        if (!apzc->GetAsyncTransformAppliedToContent()
            && !Matrix4x4(apzc->GetCurrentAsyncTransform()).IsIdentity()) {
          aManager->UnusedApzTransformWarning();
          break;
        }
      }
    }
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
ContainerLayerComposite::RenderLayer(const gfx::IntRect& aClipRect)
{
  ContainerRender(this, mCompositeManager, aClipRect);
}

void
ContainerLayerComposite::Prepare(const RenderTargetIntRect& aClipRect)
{
  ContainerPrepare(this, mCompositeManager, aClipRect);
}

void
ContainerLayerComposite::CleanupResources()
{
  mLastIntermediateSurface = nullptr;

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
RefLayerComposite::RenderLayer(const gfx::IntRect& aClipRect)
{
  ContainerRender(this, mCompositeManager, aClipRect);
}

void
RefLayerComposite::Prepare(const RenderTargetIntRect& aClipRect)
{
  ContainerPrepare(this, mCompositeManager, aClipRect);
}


void
RefLayerComposite::CleanupResources()
{
  mLastIntermediateSurface = nullptr;
}

} 
} 

