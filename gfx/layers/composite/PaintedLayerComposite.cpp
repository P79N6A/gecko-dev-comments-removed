




#include "PaintedLayerComposite.h"
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
#include "nsRefPtr.h"                   
#include "nsISupportsImpl.h"            
#include "nsMathUtils.h"                
#include "nsString.h"                   
#include "TextRenderer.h"
#include "GeckoProfiler.h"

namespace mozilla {
namespace layers {

class TiledLayerComposer;

PaintedLayerComposite::PaintedLayerComposite(LayerManagerComposite *aManager)
  : PaintedLayer(aManager, nullptr)
  , LayerComposite(aManager)
  , mBuffer(nullptr)
{
  MOZ_COUNT_CTOR(PaintedLayerComposite);
  mImplData = static_cast<LayerComposite*>(this);
}

PaintedLayerComposite::~PaintedLayerComposite()
{
  MOZ_COUNT_DTOR(PaintedLayerComposite);
  CleanupResources();
}

bool
PaintedLayerComposite::SetCompositableHost(CompositableHost* aHost)
{
  switch (aHost->GetType()) {
    case CompositableType::CONTENT_TILED:
    case CompositableType::CONTENT_SINGLE:
    case CompositableType::CONTENT_DOUBLE:
      mBuffer = static_cast<ContentHost*>(aHost);
      return true;
    default:
      return false;
  }
}

void
PaintedLayerComposite::Disconnect()
{
  Destroy();
}

void
PaintedLayerComposite::Destroy()
{
  if (!mDestroyed) {
    CleanupResources();
    mDestroyed = true;
  }
}

Layer*
PaintedLayerComposite::GetLayer()
{
  return this;
}

void
PaintedLayerComposite::SetLayerManager(LayerManagerComposite* aManager)
{
  LayerComposite::SetLayerManager(aManager);
  mManager = aManager;
  if (mBuffer && mCompositor) {
    mBuffer->SetCompositor(mCompositor);
  }
}

TiledLayerComposer*
PaintedLayerComposite::GetTiledLayerComposer()
{
  if (!mBuffer) {
    return nullptr;
  }
  MOZ_ASSERT(mBuffer->IsAttached());
  return mBuffer->AsTiledLayerComposer();
}

LayerRenderState
PaintedLayerComposite::GetRenderState()
{
  if (!mBuffer || !mBuffer->IsAttached() || mDestroyed) {
    return LayerRenderState();
  }
  return mBuffer->GetRenderState();
}

void
PaintedLayerComposite::RenderLayer(const gfx::IntRect& aClipRect)
{
  if (!mBuffer || !mBuffer->IsAttached()) {
    return;
  }
  PROFILER_LABEL("PaintedLayerComposite", "RenderLayer",
    js::ProfileEntry::Category::GRAPHICS);

  MOZ_ASSERT(mBuffer->GetCompositor() == mCompositeManager->GetCompositor() &&
             mBuffer->GetLayer() == this,
             "buffer is corrupted");

  const nsIntRegion& visibleRegion = GetEffectiveVisibleRegion();
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

  mBuffer->SetPaintWillResample(MayResample());

  mBuffer->Composite(effectChain,
                     GetEffectiveOpacity(),
                     GetEffectiveTransform(),
                     GetEffectFilter(),
                     clipRect,
                     &visibleRegion);
  mBuffer->BumpFlashCounter();

  mCompositeManager->GetCompositor()->MakeCurrent();
}

CompositableHost*
PaintedLayerComposite::GetCompositableHost()
{
  if (mBuffer && mBuffer->IsAttached()) {
    return mBuffer.get();
  }

  return nullptr;
}

void
PaintedLayerComposite::CleanupResources()
{
  if (mBuffer) {
    mBuffer->Detach(this);
  }
  mBuffer = nullptr;
}

void
PaintedLayerComposite::GenEffectChain(EffectChain& aEffect)
{
  aEffect.mLayerRef = this;
  aEffect.mPrimaryEffect = mBuffer->GenEffect(GetEffectFilter());
}

void
PaintedLayerComposite::PrintInfo(std::stringstream& aStream, const char* aPrefix)
{
  PaintedLayer::PrintInfo(aStream, aPrefix);
  if (mBuffer && mBuffer->IsAttached()) {
    aStream << "\n";
    nsAutoCString pfx(aPrefix);
    pfx += "  ";
    mBuffer->PrintInfo(aStream, pfx.get());
  }
}

} 
} 
