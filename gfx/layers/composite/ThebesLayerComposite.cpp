




#include "ThebesLayerComposite.h"
#include "CompositableHost.h"           
#include "FrameMetrics.h"               
#include "Units.h"                      
#include "gfx2DGlue.h"                  
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
#include "nsISupportsImpl.h"            
#include "nsMathUtils.h"                
#include "nsPoint.h"                    
#include "nsRect.h"                     
#include "nsSize.h"                     
#include "nsString.h"                   
#include "TextRenderer.h"
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

bool
ThebesLayerComposite::SetCompositableHost(CompositableHost* aHost)
{
  switch (aHost->GetType()) {
    case CompositableType::BUFFER_CONTENT_INC:
    case CompositableType::BUFFER_TILED:
    case CompositableType::CONTENT_SINGLE:
    case CompositableType::CONTENT_DOUBLE:
      mBuffer = static_cast<ContentHost*>(aHost);
      return true;
    default:
      return false;
  }
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
  if (!mBuffer) {
    return nullptr;
  }
  MOZ_ASSERT(mBuffer->IsAttached());
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
ThebesLayerComposite::RenderLayer(const nsIntRect& aClipRect)
{
  if (!mBuffer || !mBuffer->IsAttached()) {
    return;
  }
  PROFILER_LABEL("ThebesLayerComposite", "RenderLayer");

  MOZ_ASSERT(mBuffer->GetCompositor() == mCompositeManager->GetCompositor() &&
             mBuffer->GetLayer() == this,
             "buffer is corrupted");

  gfx::Rect clipRect(aClipRect.x, aClipRect.y, aClipRect.width, aClipRect.height);

#ifdef MOZ_DUMP_PAINTING
  if (gfxUtils::sDumpPainting) {
    RefPtr<gfx::DataSourceSurface> surf = mBuffer->GetAsSurface();
    if (surf) {
      WriteSnapshotToDumpFile(this, surf);
    }
  }
#endif

  EffectChain effectChain(this);
  LayerManagerComposite::AutoAddMaskEffect autoMaskEffect(mMaskLayer, effectChain);
  AddBlendModeEffect(effectChain);

  nsIntRegion visibleRegion = GetEffectiveVisibleRegion();

  TiledLayerProperties tiledLayerProps;
  if (mRequiresTiledProperties) {
    tiledLayerProps.mVisibleRegion = visibleRegion;
    tiledLayerProps.mEffectiveResolution = GetEffectiveResolution();
    tiledLayerProps.mValidRegion = mValidRegion;
  }

  mBuffer->SetPaintWillResample(MayResample());

  mBuffer->Composite(effectChain,
                     GetEffectiveOpacity(),
                     GetEffectiveTransform(),
                     gfx::Filter::LINEAR,
                     clipRect,
                     &visibleRegion,
                     mRequiresTiledProperties ? &tiledLayerProps
                                              : nullptr);
  mBuffer->BumpFlashCounter();

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

CSSToScreenScale
ThebesLayerComposite::GetEffectiveResolution()
{
  for (ContainerLayer* parent = GetParent(); parent; parent = parent->GetParent()) {
    const FrameMetrics& metrics = parent->GetFrameMetrics();
    if (metrics.GetScrollId() != FrameMetrics::NULL_SCROLL_ID) {
      return metrics.GetZoom();
    }
  }

  return CSSToScreenScale(1.0);
}

nsACString&
ThebesLayerComposite::PrintInfo(nsACString& aTo, const char* aPrefix)
{
  ThebesLayer::PrintInfo(aTo, aPrefix);
  if (mBuffer && mBuffer->IsAttached()) {
    aTo += "\n";
    nsAutoCString pfx(aPrefix);
    pfx += "  ";
    mBuffer->PrintInfo(aTo, pfx.get());
  }
  return aTo;
}

} 
} 
