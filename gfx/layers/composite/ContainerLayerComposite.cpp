




#include "ContainerLayerComposite.h"
#include <algorithm>                    
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
#include "mozilla/layers/AsyncPanZoomController.h"  
#include "mozilla/layers/AsyncCompositionManager.h" 
#include "mozilla/layers/LayerMetricsWrapper.h" 
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsISupportsUtils.h"           
#include "nsPoint.h"                    
#include "nsRect.h"                     
#include "nsRegion.h"                   
#include "nsTArray.h"                   
#include "TextRenderer.h"               
#include <vector>

#define CULLING_LOG(...)


namespace mozilla {
namespace layers {

using namespace gfx;





static nsIntRect
GetOpaqueRect(Layer* aLayer)
{
  nsIntRect result;
  gfx::Matrix matrix;
  bool is2D = aLayer->GetBaseTransform().Is2D(&matrix);

  
  if (!is2D || aLayer->GetMaskLayer() ||
    aLayer->GetIsFixedPosition() ||
    aLayer->GetIsStickyPosition() ||
    aLayer->GetEffectiveOpacity() != 1.0f ||
    matrix.HasNonIntegerTranslation()) {
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

  
  gfx::Point point = matrix.GetTranslation();
  result.MoveBy(static_cast<int>(point.x), static_cast<int>(point.y));

  const nsIntRect* clipRect = aLayer->GetEffectiveClipRect();
  if (clipRect) {
    result.IntersectRect(result, *clipRect);
  }

  return result;
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

  nsIntPoint topLeft = visibleRegion.GetBounds().TopLeft();
  aManager->GetTextRenderer()->RenderText(ss.str().c_str(), gfx::IntPoint(topLeft.x, topLeft.y),
                                          aLayer->GetEffectiveTransform(), 16,
                                          maxWidth);
}

static void PrintUniformityInfo(Layer* aLayer)
{
  
  if (aLayer->GetEffectiveVisibleRegion().GetBounds().width < 300 ||
      aLayer->GetEffectiveVisibleRegion().GetBounds().height < 300) {
    return;
  }

  Matrix4x4 transform = aLayer->AsLayerComposite()->GetShadowTransform();
  if (!transform.Is2D()) {
    return;
  }
  Point translation = transform.As2D().GetTranslation();
  printf_stderr("UniformityInfo Layer_Move %llu %p %f, %f\n",
            TimeStamp::Now(), aLayer, translation.x.value, translation.y.value);
}


struct PreparedLayer
{
  PreparedLayer(LayerComposite *aLayer, RenderTargetIntRect aClipRect, bool aRestoreVisibleRegion, nsIntRegion &aVisibleRegion) :
    mLayer(aLayer), mClipRect(aClipRect), mRestoreVisibleRegion(aRestoreVisibleRegion), mSavedVisibleRegion(aVisibleRegion) {}
  LayerComposite* mLayer;
  RenderTargetIntRect mClipRect;
  bool mRestoreVisibleRegion;
  nsIntRegion mSavedVisibleRegion;
};


struct PreparedData
{
  RefPtr<CompositingRenderTarget> mTmpTarget;
  nsAutoTArray<PreparedLayer, 12> mLayers;
};


template<class ContainerT> void
ContainerPrepare(ContainerT* aContainer,
                 LayerManagerComposite* aManager,
                 const RenderTargetIntRect& aClipRect)
{
  aContainer->mPrepared = MakeUnique<PreparedData>();

  


  nsAutoTArray<Layer*, 12> children;
  aContainer->SortChildrenBy3DZOrder(children);

  for (uint32_t i = 0; i < children.Length(); i++) {
    LayerComposite* layerToRender = static_cast<LayerComposite*>(children.ElementAt(i)->ImplData());

    if (layerToRender->GetLayer()->GetEffectiveVisibleRegion().IsEmpty() &&
        !layerToRender->GetLayer()->AsContainerLayer()) {
      continue;
    }

    RenderTargetIntRect clipRect = layerToRender->GetLayer()->
        CalculateScissorRect(aClipRect, &aManager->GetWorldTransform());
    if (clipRect.IsEmpty()) {
      continue;
    }

    RenderTargetRect quad = layerToRender->GetLayer()->
      TransformRectToRenderTarget(LayerPixel::FromUntyped(
        layerToRender->GetLayer()->GetEffectiveVisibleRegion().GetBounds()));

    Compositor* compositor = aManager->GetCompositor();
    if (!layerToRender->GetLayer()->AsContainerLayer() &&
        !quad.Intersects(compositor->ClipRectInLayersCoordinates(layerToRender->GetLayer(), clipRect))) {
      continue;
    }

    CULLING_LOG("Preparing sublayer %p\n", layerToRender->GetLayer());

    nsIntRegion savedVisibleRegion;
    bool restoreVisibleRegion = false;
    gfx::Matrix matrix;
    bool is2D = layerToRender->GetLayer()->GetBaseTransform().Is2D(&matrix);
    if (i + 1 < children.Length() &&
        is2D && !matrix.HasNonIntegerTranslation()) {
      LayerComposite* nextLayer = static_cast<LayerComposite*>(children.ElementAt(i + 1)->ImplData());
      CULLING_LOG("Culling against %p\n", nextLayer->GetLayer());
      nsIntRect nextLayerOpaqueRect;
      if (nextLayer && nextLayer->GetLayer()) {
        nextLayerOpaqueRect = GetOpaqueRect(nextLayer->GetLayer());
        gfx::Point point = matrix.GetTranslation();
        nextLayerOpaqueRect.MoveBy(static_cast<int>(-point.x), static_cast<int>(-point.y));
        CULLING_LOG("  point %i, %i\n", static_cast<int>(-point.x), static_cast<int>(-point.y));
        CULLING_LOG("  opaque rect %i, %i, %i, %i\n", nextLayerOpaqueRect.x, nextLayerOpaqueRect.y, nextLayerOpaqueRect.width, nextLayerOpaqueRect.height);
      }
      if (!nextLayerOpaqueRect.IsEmpty()) {
        CULLING_LOG("  draw\n");
        savedVisibleRegion = layerToRender->GetShadowVisibleRegion();
        nsIntRegion visibleRegion;
        visibleRegion.Sub(savedVisibleRegion, nextLayerOpaqueRect);
        if (visibleRegion.IsEmpty()) {
          continue;
        }
        layerToRender->SetShadowVisibleRegion(visibleRegion);
        restoreVisibleRegion = true;
      } else {
        CULLING_LOG("  skip\n");
      }
    }
    layerToRender->Prepare(clipRect);
    aContainer->mPrepared->mLayers.AppendElement(PreparedLayer(layerToRender, clipRect, restoreVisibleRegion, savedVisibleRegion));
  }

  CULLING_LOG("Preparing container layer %p\n", aContainer->GetLayer());

  



  bool surfaceCopyNeeded;
  
  aContainer->DefaultComputeSupportsComponentAlphaChildren(&surfaceCopyNeeded);
  if (aContainer->UseIntermediateSurface()) {
    MOZ_PERFORMANCE_WARNING("gfx", "[%p] Container layer requires intermediate surface\n", aContainer);
    if (!surfaceCopyNeeded) {
      
      
      RefPtr<CompositingRenderTarget> surface = CreateTemporaryTarget(aContainer, aManager);
      RenderIntermediate(aContainer, aManager, RenderTargetPixel::ToUntyped(aClipRect), surface);
      aContainer->mPrepared->mTmpTarget = surface;
    }
  }
}

template<class ContainerT> void
RenderLayers(ContainerT* aContainer,
	     LayerManagerComposite* aManager,
	     const RenderTargetIntRect& aClipRect)
{
  Compositor* compositor = aManager->GetCompositor();

  float opacity = aContainer->GetEffectiveOpacity();

  
  
  
  
  
  
  
  if (aContainer->HasScrollableFrameMetrics() && !aContainer->IsScrollInfoLayer()) {
    bool overscrolled = false;
    for (uint32_t i = 0; i < aContainer->GetFrameMetricsCount(); i++) {
      AsyncPanZoomController* apzc = aContainer->GetAsyncPanZoomController(i);
      if (apzc && apzc->IsOverscrolled()) {
        overscrolled = true;
        break;
      }
    }
    if (overscrolled) {
      gfxRGBA color = aContainer->GetBackgroundColor();
      
      
      
      if (color.a != 0.0) {
        EffectChain effectChain(aContainer);
        effectChain.mPrimaryEffect = new EffectSolidColor(ToColor(color));
        gfx::Rect clipRect(aClipRect.x, aClipRect.y, aClipRect.width, aClipRect.height);
        compositor->DrawQuad(
          RenderTargetPixel::ToUnknown(
            compositor->ClipRectInLayersCoordinates(aContainer, aClipRect)),
          clipRect, effectChain, opacity, Matrix4x4());
      }
    }
  }

  for (size_t i = 0u; i < aContainer->mPrepared->mLayers.Length(); i++) {
    PreparedLayer& preparedData = aContainer->mPrepared->mLayers[i];
    LayerComposite* layerToRender = preparedData.mLayer;
    const RenderTargetIntRect& clipRect = preparedData.mClipRect;

    if (layerToRender->HasLayerBeenComposited()) {
      
      
      layerToRender->SetLayerComposited(false);
      nsIntRect clearRect = layerToRender->GetClearRect();
      if (!clearRect.IsEmpty()) {
        
        gfx::Rect fbRect(clearRect.x, clearRect.y, clearRect.width, clearRect.height);
        compositor->ClearRect(fbRect);
        layerToRender->SetClearRect(nsIntRect(0, 0, 0, 0));
      }
    } else {
      layerToRender->RenderLayer(RenderTargetPixel::ToUntyped(clipRect));
    }

    if (preparedData.mRestoreVisibleRegion) {
      
      layerToRender->SetShadowVisibleRegion(preparedData.mSavedVisibleRegion);
    }

    if (gfxPrefs::UniformityInfo()) {
      PrintUniformityInfo(layerToRender->GetLayer());
    }

    if (gfxPrefs::DrawLayerInfo()) {
      DrawLayerInfo(clipRect, aManager, layerToRender->GetLayer());
    }
    
    
  }
}

template<class ContainerT> RefPtr<CompositingRenderTarget>
CreateTemporaryTarget(ContainerT* aContainer,
                      LayerManagerComposite* aManager)
{
  Compositor* compositor = aManager->GetCompositor();
  nsIntRect visibleRect = aContainer->GetEffectiveVisibleRegion().GetBounds();
  SurfaceInitMode mode = INIT_MODE_CLEAR;
  gfx::IntRect surfaceRect = gfx::IntRect(visibleRect.x, visibleRect.y,
                                          visibleRect.width, visibleRect.height);
  if (aContainer->GetEffectiveVisibleRegion().GetNumRects() == 1 &&
      (aContainer->GetContentFlags() & Layer::CONTENT_OPAQUE))
  {
    mode = INIT_MODE_NONE;
  }
  return compositor->CreateRenderTarget(surfaceRect, mode);
}

template<class ContainerT> RefPtr<CompositingRenderTarget>
CreateTemporaryTargetAndCopyFromBackground(ContainerT* aContainer,
                                           LayerManagerComposite* aManager)
{
  Compositor* compositor = aManager->GetCompositor();
  nsIntRect visibleRect = aContainer->GetEffectiveVisibleRegion().GetBounds();
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
                   const nsIntRect& aClipRect,
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
                 const nsIntRect& aClipRect)
{
  MOZ_ASSERT(aContainer->mPrepared);
  if (aContainer->UseIntermediateSurface()) {
    RefPtr<CompositingRenderTarget> surface;

    if (!aContainer->mPrepared->mTmpTarget) {
      
      surface = CreateTemporaryTargetAndCopyFromBackground(aContainer, aManager);
      RenderIntermediate(aContainer, aManager,
                         aClipRect, surface);
    } else {
      surface = aContainer->mPrepared->mTmpTarget;
    }

    float opacity = aContainer->GetEffectiveOpacity();

    nsIntRect visibleRect = aContainer->GetEffectiveVisibleRegion().GetBounds();
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

  for (uint32_t i = 0; i < aContainer->GetFrameMetricsCount(); i++) {
    if (!aContainer->GetFrameMetrics(i).IsScrollable()) {
      continue;
    }
    const FrameMetrics& frame = aContainer->GetFrameMetrics(i);
    LayerRect layerBounds = frame.mCompositionBounds * ParentLayerToLayerScale(1.0);
    gfx::Rect rect(layerBounds.x, layerBounds.y, layerBounds.width, layerBounds.height);
    gfx::Rect clipRect(aClipRect.x, aClipRect.y, aClipRect.width, aClipRect.height);
    aManager->GetCompositor()->DrawDiagnostics(DiagnosticFlags::CONTAINER,
                                               rect, clipRect,
                                               aContainer->GetEffectiveTransform());
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
ContainerLayerComposite::RenderLayer(const nsIntRect& aClipRect)
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
RefLayerComposite::RenderLayer(const nsIntRect& aClipRect)
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
}

} 
} 
