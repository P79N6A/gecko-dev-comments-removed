




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
  , mImageHost(nullptr)
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
      mImageHost = aHost;
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
  if (mImageHost && mCompositor) {
    mImageHost->SetCompositor(mCompositor);
  }
}

LayerRenderState
CanvasLayerComposite::GetRenderState()
{
  if (mDestroyed || !mImageHost || !mImageHost->IsAttached()) {
    return LayerRenderState();
  }
  return mImageHost->GetRenderState();
}

void
CanvasLayerComposite::RenderLayer(const nsIntRect& aClipRect)
{
  if (!mImageHost || !mImageHost->IsAttached()) {
    return;
  }

  mCompositor->MakeCurrent();

#ifdef MOZ_DUMP_PAINTING
  if (gfxUtils::sDumpPainting) {
    RefPtr<gfx::DataSourceSurface> surf = mImageHost->GetAsSurface();
    WriteSnapshotToDumpFile(this, surf);
  }
#endif

  EffectChain effectChain(this);
  AddBlendModeEffect(effectChain);

  LayerManagerComposite::AutoAddMaskEffect autoMaskEffect(mMaskLayer, effectChain);
  gfx::Rect clipRect(aClipRect.x, aClipRect.y, aClipRect.width, aClipRect.height);

  mImageHost->Composite(effectChain,
                        GetEffectiveOpacity(),
                        GetEffectiveTransform(),
                        GetEffectFilter(),
                        clipRect);
  mImageHost->BumpFlashCounter();
}

CompositableHost*
CanvasLayerComposite::GetCompositableHost()
{
  if ( mImageHost && mImageHost->IsAttached()) {
    return mImageHost.get();
  }

  return nullptr;
}

void
CanvasLayerComposite::CleanupResources()
{
  if (mImageHost) {
    mImageHost->Detach(this);
  }
  mImageHost = nullptr;
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
  aEffect.mPrimaryEffect = mImageHost->GenEffect(GetEffectFilter());
}

void
CanvasLayerComposite::PrintInfo(std::stringstream& aStream, const char* aPrefix)
{
  CanvasLayer::PrintInfo(aStream, aPrefix);
  aStream << "\n";
  if (mImageHost && mImageHost->IsAttached()) {
    nsAutoCString pfx(aPrefix);
    pfx += "  ";
    mImageHost->PrintInfo(aStream, pfx.get());
  }
}

}
}
