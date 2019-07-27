




#include "CanvasLayerComposite.h"
#include "composite/CompositableHost.h"  
#include "gfx2DGlue.h"                  
#include "GraphicsFilter.h"             
#include "gfxUtils.h"                   
#include "mozilla/gfx/Matrix.h"         
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Rect.h"           
#include "mozilla/layers/Compositor.h"  
#include "mozilla/layers/Effects.h"     
#include "mozilla/mozalloc.h"           
#include "nsAString.h"
#include "nsRefPtr.h"                   
#include "nsISupportsImpl.h"            
#include "nsString.h"                   
#include "gfxVR.h"

namespace mozilla {
namespace layers {

using namespace mozilla::gfx;

CanvasLayerComposite::CanvasLayerComposite(LayerManagerComposite* aManager)
  : CanvasLayer(aManager, nullptr)
  , LayerComposite(aManager)
  , mCompositableHost(nullptr)
{
  MOZ_COUNT_CTOR(CanvasLayerComposite);
  mImplData = static_cast<LayerComposite*>(this);
}

CanvasLayerComposite::~CanvasLayerComposite()
{
  MOZ_COUNT_DTOR(CanvasLayerComposite);

  CleanupResources();
}

bool
CanvasLayerComposite::SetCompositableHost(CompositableHost* aHost)
{
  switch (aHost->GetType()) {
    case CompositableType::IMAGE:
      mCompositableHost = aHost;
      return true;
    default:
      return false;
  }

}

Layer*
CanvasLayerComposite::GetLayer()
{
  return this;
}

void
CanvasLayerComposite::SetLayerManager(LayerManagerComposite* aManager)
{
  LayerComposite::SetLayerManager(aManager);
  mManager = aManager;
  if (mCompositableHost && mCompositor) {
    mCompositableHost->SetCompositor(mCompositor);
  }
}

LayerRenderState
CanvasLayerComposite::GetRenderState()
{
  if (mDestroyed || !mCompositableHost || !mCompositableHost->IsAttached()) {
    return LayerRenderState();
  }
  return mCompositableHost->GetRenderState();
}

void
CanvasLayerComposite::RenderLayer(const IntRect& aClipRect)
{
  if (!mCompositableHost || !mCompositableHost->IsAttached()) {
    return;
  }

  mCompositor->MakeCurrent();

#ifdef MOZ_DUMP_PAINTING
  if (gfxUtils::sDumpPainting) {
    RefPtr<gfx::DataSourceSurface> surf = mCompositableHost->GetAsSurface();
    WriteSnapshotToDumpFile(this, surf);
  }
#endif

  RenderWithAllMasks(this, mCompositor, aClipRect,
                     [&](EffectChain& effectChain, const Rect& clipRect) {
    mCompositableHost->Composite(this, effectChain,
                          GetEffectiveOpacity(),
                          GetEffectiveTransform(),
                          GetEffectFilter(),
                          clipRect);
  });

  mCompositableHost->BumpFlashCounter();
}

CompositableHost*
CanvasLayerComposite::GetCompositableHost()
{
  if (mCompositableHost && mCompositableHost->IsAttached()) {
    return mCompositableHost.get();
  }

  return nullptr;
}

void
CanvasLayerComposite::CleanupResources()
{
  if (mCompositableHost) {
    mCompositableHost->Detach(this);
  }
  mCompositableHost = nullptr;
}

gfx::Filter
CanvasLayerComposite::GetEffectFilter()
{
  GraphicsFilter filter = mFilter;
#ifdef ANDROID
  
  
  
  Matrix matrix;
  bool is2D = GetEffectiveTransform().Is2D(&matrix);
  if (is2D && !ThebesMatrix(matrix).HasNonTranslationOrFlip()) {
    filter = GraphicsFilter::FILTER_NEAREST;
  }
#endif
  return gfx::ToFilter(filter);
}

void
CanvasLayerComposite::GenEffectChain(EffectChain& aEffect)
{
  aEffect.mLayerRef = this;
  aEffect.mPrimaryEffect = mCompositableHost->GenEffect(GetEffectFilter());
}

void
CanvasLayerComposite::PrintInfo(std::stringstream& aStream, const char* aPrefix)
{
  CanvasLayer::PrintInfo(aStream, aPrefix);
  aStream << "\n";
  if (mCompositableHost && mCompositableHost->IsAttached()) {
    nsAutoCString pfx(aPrefix);
    pfx += "  ";
    mCompositableHost->PrintInfo(aStream, pfx.get());
  }
}

}
}
