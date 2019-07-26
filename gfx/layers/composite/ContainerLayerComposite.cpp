




#include "ContainerLayerComposite.h"
#include <algorithm>                    
#include "FrameMetrics.h"               
#include "Units.h"                      
#include "gfx2DGlue.h"                  
#include "gfx3DMatrix.h"                
#include "gfxPrefs.h"                   
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
#include "nsISupportsImpl.h"            
#include "nsISupportsUtils.h"           
#include "nsPoint.h"                    
#include "nsRect.h"                     
#include "nsRegion.h"                   
#include "nsTArray.h"                   
#include "TextRenderer.h"               
#include <vector>

namespace mozilla {
namespace layers {





static nsIntRect
GetOpaqueRect(Layer* aLayer)
{
  nsIntRect result;
  gfx::Matrix matrix;
  bool is2D = aLayer->GetBaseTransform().Is2D(&matrix);

  
  if (!is2D || aLayer->GetMaskLayer() ||
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

struct LayerVelocityUserData : public LayerUserData {
public:
  LayerVelocityUserData() {
    MOZ_COUNT_CTOR(LayerVelocityUserData);
  }
  ~LayerVelocityUserData() {
    MOZ_COUNT_DTOR(LayerVelocityUserData);
  }

  struct VelocityData {
    VelocityData(TimeStamp frameTime, int scrollX, int scrollY)
      : mFrameTime(frameTime)
      , mPoint(scrollX, scrollY)
    {}

    TimeStamp mFrameTime;
    gfx::Point mPoint;
  };
  std::vector<VelocityData> mData;
};

static gfx::Point GetScrollData(Layer* aLayer) {
  gfx::Matrix matrix;
  if (aLayer->GetLocalTransform().Is2D(&matrix)) {
    return matrix.GetTranslation();
  }

  gfx::Point origin;
  return origin;
}

static void DrawLayerInfo(const nsIntRect& aClipRect,
                          LayerManagerComposite* aManager,
                          Layer* aLayer)
{

  if (aLayer->GetType() == Layer::LayerType::TYPE_CONTAINER) {
    
    
    
    return;
  }

  nsAutoCString layerInfo;
  aLayer->PrintInfo(layerInfo, "");

  nsIntRegion visibleRegion = aLayer->GetVisibleRegion();

  uint32_t maxWidth = std::min<uint32_t>(visibleRegion.GetBounds().width, 500);

  nsIntPoint topLeft = visibleRegion.GetBounds().TopLeft();
  aManager->GetTextRenderer()->RenderText(layerInfo.get(), gfx::IntPoint(topLeft.x, topLeft.y),
                                          aLayer->GetEffectiveTransform(), 16,
                                          maxWidth);

}

static LayerVelocityUserData* GetVelocityData(Layer* aLayer) {
  static char sLayerVelocityUserDataKey;
  void* key = reinterpret_cast<void*>(&sLayerVelocityUserDataKey);
  if (!aLayer->HasUserData(key)) {
    LayerVelocityUserData* newData = new LayerVelocityUserData();
    aLayer->SetUserData(key, newData);
  }

  return static_cast<LayerVelocityUserData*>(aLayer->GetUserData(key));
}

static void DrawVelGraph(const nsIntRect& aClipRect,
                         LayerManagerComposite* aManager,
                         Layer* aLayer) {
  Compositor* compositor = aManager->GetCompositor();
  gfx::Rect clipRect(aClipRect.x, aClipRect.y,
                     aClipRect.width, aClipRect.height);

  TimeStamp now = TimeStamp::Now();
  LayerVelocityUserData* velocityData = GetVelocityData(aLayer);

  if (velocityData->mData.size() >= 1 &&
    now > velocityData->mData[velocityData->mData.size() - 1].mFrameTime +
      TimeDuration::FromMilliseconds(200)) {
    
    velocityData->mData.clear();
  }

  const gfx::Point layerTransform = GetScrollData(aLayer);
  velocityData->mData.push_back(
    LayerVelocityUserData::VelocityData(now,
      static_cast<int>(layerTransform.x), static_cast<int>(layerTransform.y)));

  
  
  
  
  
  

  
  size_t circularBufferSize = 100;
  if (velocityData->mData.size() > circularBufferSize) {
    velocityData->mData.erase(velocityData->mData.begin());
  }

  if (velocityData->mData.size() == 1) {
    return;
  }

  
  for (size_t i = 1; i < velocityData->mData.size(); i++) {
    if (velocityData->mData[i - 1].mPoint != velocityData->mData[i].mPoint) {
      break;
    }
    if (i == velocityData->mData.size() - 1) {
      velocityData->mData.clear();
      return;
    }
  }

  if (aLayer->GetEffectiveVisibleRegion().GetBounds().width < 300 ||
      aLayer->GetEffectiveVisibleRegion().GetBounds().height < 300) {
    
    return;
  }

  aManager->SetDebugOverlayWantsNextFrame(true);

  const gfx::Matrix4x4& transform = aLayer->GetEffectiveTransform();
  nsIntRect bounds = aLayer->GetEffectiveVisibleRegion().GetBounds();
  gfx::Rect graphBounds = gfx::Rect(bounds.x, bounds.y,
                                    bounds.width, bounds.height);
  gfx::Rect graphRect = gfx::Rect(bounds.x, bounds.y, 200, 100);

  float opacity = 1.0;
  EffectChain effects;
  effects.mPrimaryEffect = new EffectSolidColor(gfx::Color(0.2f,0,0,1));
  compositor->DrawQuad(graphRect,
                       clipRect,
                       effects,
                       opacity,
                       transform);

  std::vector<gfx::Point> graph;
  int yScaleFactor = 3;
  for (int32_t i = (int32_t)velocityData->mData.size() - 2; i >= 0; i--) {
    const gfx::Point& p1 = velocityData->mData[i+1].mPoint;
    const gfx::Point& p2 = velocityData->mData[i].mPoint;
    int vel = sqrt((p1.x - p2.x) * (p1.x - p2.x) +
                   (p1.y - p2.y) * (p1.y - p2.y));
    graph.push_back(
      gfx::Point(bounds.x + graphRect.width / circularBufferSize * i,
                 graphBounds.y + graphRect.height - vel/yScaleFactor));
  }

  compositor->DrawLines(graph, clipRect, gfx::Color(0,1,0,1),
                        opacity, transform);
}


template<class ContainerT> void
ContainerRender(ContainerT* aContainer,
                LayerManagerComposite* aManager,
                const nsIntRect& aClipRect)
{
  


  RefPtr<CompositingRenderTarget> surface;

  Compositor* compositor = aManager->GetCompositor();

  RefPtr<CompositingRenderTarget> previousTarget = compositor->GetCurrentRenderTarget();

  nsIntRect visibleRect = aContainer->GetEffectiveVisibleRegion().GetBounds();

  float opacity = aContainer->GetEffectiveOpacity();

  bool needsSurface = aContainer->UseIntermediateSurface();
  bool surfaceCopyNeeded;
  aContainer->DefaultComputeSupportsComponentAlphaChildren(&surfaceCopyNeeded);
  if (needsSurface) {
    SurfaceInitMode mode = INIT_MODE_CLEAR;
    gfx::IntRect surfaceRect = gfx::IntRect(visibleRect.x, visibleRect.y,
                                            visibleRect.width, visibleRect.height);
    
    
    
    
    
    
    int32_t maxTextureSize = compositor->GetMaxTextureSize();
    surfaceRect.width = std::min(maxTextureSize, surfaceRect.width);
    surfaceRect.height = std::min(maxTextureSize, surfaceRect.height);
    if (aContainer->GetEffectiveVisibleRegion().GetNumRects() == 1 &&
        (aContainer->GetContentFlags() & Layer::CONTENT_OPAQUE))
    {
      mode = INIT_MODE_NONE;
    }

    if (surfaceCopyNeeded) {
      gfx::IntPoint sourcePoint = gfx::IntPoint(visibleRect.x, visibleRect.y);

      gfx::Matrix4x4 transform = aContainer->GetEffectiveTransform();
      DebugOnly<gfx::Matrix> transform2d;
      MOZ_ASSERT(transform.Is2D(&transform2d) && !gfx::ThebesMatrix(transform2d).HasNonIntegerTranslation());
      sourcePoint += gfx::IntPoint(transform._41, transform._42);

      sourcePoint -= compositor->GetCurrentRenderTarget()->GetOrigin();

      surface = compositor->CreateRenderTargetFromSource(surfaceRect, previousTarget, sourcePoint);
    } else {
      surface = compositor->CreateRenderTarget(surfaceRect, mode);
    }

    if (!surface) {
      return;
    }

    compositor->SetRenderTarget(surface);
  } else {
    surface = previousTarget;
  }

  nsAutoTArray<Layer*, 12> children;
  aContainer->SortChildrenBy3DZOrder(children);

  


  for (uint32_t i = 0; i < children.Length(); i++) {
    LayerComposite* layerToRender = static_cast<LayerComposite*>(children.ElementAt(i)->ImplData());

    if (layerToRender->GetLayer()->GetEffectiveVisibleRegion().IsEmpty() &&
        !layerToRender->GetLayer()->AsContainerLayer()) {
      continue;
    }

    nsIntRect clipRect = layerToRender->GetLayer()->
        CalculateScissorRect(aClipRect, &aManager->GetWorldTransform());
    if (clipRect.IsEmpty()) {
      continue;
    }

    nsIntRegion savedVisibleRegion;
    bool restoreVisibleRegion = false;
    if (i + 1 < children.Length() &&
        layerToRender->GetLayer()->GetEffectiveTransform().IsIdentity()) {
      LayerComposite* nextLayer = static_cast<LayerComposite*>(children.ElementAt(i + 1)->ImplData());
      nsIntRect nextLayerOpaqueRect;
      if (nextLayer && nextLayer->GetLayer()) {
        nextLayerOpaqueRect = GetOpaqueRect(nextLayer->GetLayer());
      }
      if (!nextLayerOpaqueRect.IsEmpty()) {
        savedVisibleRegion = layerToRender->GetShadowVisibleRegion();
        nsIntRegion visibleRegion;
        visibleRegion.Sub(savedVisibleRegion, nextLayerOpaqueRect);
        if (visibleRegion.IsEmpty()) {
          continue;
        }
        layerToRender->SetShadowVisibleRegion(visibleRegion);
        restoreVisibleRegion = true;
      }
    }

    if (layerToRender->HasLayerBeenComposited()) {
      
      
      layerToRender->SetLayerComposited(false);
      nsIntRect clearRect = layerToRender->GetClearRect();
      if (!clearRect.IsEmpty()) {
        
        gfx::Rect fbRect(clearRect.x, clearRect.y, clearRect.width, clearRect.height);
        compositor->ClearRect(fbRect);
        layerToRender->SetClearRect(nsIntRect(0, 0, 0, 0));
      }
    } else {
      layerToRender->RenderLayer(clipRect);
    }

    if (restoreVisibleRegion) {
      
      layerToRender->SetShadowVisibleRegion(savedVisibleRegion);
    }

    if (gfxPrefs::LayersScrollGraph()) {
      DrawVelGraph(clipRect, aManager, layerToRender->GetLayer());
    }

    if (gfxPrefs::DrawLayerInfo()) {
      DrawLayerInfo(clipRect, aManager, layerToRender->GetLayer());
    }
    
    
  }

  if (needsSurface) {
    
#ifdef MOZ_DUMP_PAINTING
    if (gfxUtils::sDumpPainting) {
      RefPtr<gfx::DataSourceSurface> surf = surface->Dump(aManager->GetCompositor());
      WriteSnapshotToDumpFile(aContainer, surf);
    }
#endif

    compositor->SetRenderTarget(previousTarget);
    EffectChain effectChain(aContainer);
    LayerManagerComposite::AutoAddMaskEffect autoMaskEffect(aContainer->GetMaskLayer(),
                                                            effectChain,
                                                            !aContainer->GetTransform().CanDraw2D());

    aContainer->AddBlendModeEffect(effectChain);
    effectChain.mPrimaryEffect = new EffectRenderTarget(surface);

    gfx::Rect rect(visibleRect.x, visibleRect.y, visibleRect.width, visibleRect.height);
    gfx::Rect clipRect(aClipRect.x, aClipRect.y, aClipRect.width, aClipRect.height);
    aManager->GetCompositor()->DrawQuad(rect, clipRect, effectChain, opacity,
                                        aContainer->GetEffectiveTransform());
  }

  if (aContainer->GetFrameMetrics().IsScrollable()) {
    const FrameMetrics& frame = aContainer->GetFrameMetrics();
    LayerRect layerBounds = ParentLayerRect(frame.mCompositionBounds) * ParentLayerToLayerScale(1.0);
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
RefLayerComposite::CleanupResources()
{
}

} 
} 
