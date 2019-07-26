




#include "CanvasLayerComposite.h"
#include "composite/CompositableHost.h"  
#include "gfx2DGlue.h"                  
#include "gfxImageSurface.h"            
#include "GraphicsFilter.h"             
#include "gfxUtils.h"                   
#include "mozilla/gfx/Matrix.h"         
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Rect.h"           
#include "mozilla/layers/Compositor.h"  
#include "mozilla/layers/Effects.h"     
#include "mozilla/mozalloc.h"           
#include "nsAString.h"
#include "nsAutoPtr.h"                  
#include "nsPoint.h"                    
#include "nsString.h"                   
#include "nsTraceRefcnt.h"              

using namespace mozilla;
using namespace mozilla::layers;

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

void
CanvasLayerComposite::SetCompositableHost(CompositableHost* aHost) {
  mImageHost = aHost;
}

Layer*
CanvasLayerComposite::GetLayer()
{
  return this;
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
CanvasLayerComposite::RenderLayer(const nsIntPoint& aOffset,
                                  const nsIntRect& aClipRect)
{
  if (!mImageHost || !mImageHost->IsAttached()) {
    return;
  }

  mCompositor->MakeCurrent();

#ifdef MOZ_DUMP_PAINTING
  if (gfxUtils::sDumpPainting) {
    nsRefPtr<gfxImageSurface> surf = mImageHost->GetAsSurface();
    WriteSnapshotToDumpFile(this, surf);
  }
#endif

  GraphicsFilter filter = mFilter;
#ifdef ANDROID
  
  
  
  gfxMatrix matrix;
  bool is2D = GetEffectiveTransform().Is2D(&matrix);
  if (is2D && !matrix.HasNonTranslationOrFlip()) {
    filter = GraphicsFilter::FILTER_NEAREST;
  }
#endif

  EffectChain effectChain;
  LayerManagerComposite::AutoAddMaskEffect autoMaskEffect(mMaskLayer, effectChain);
  gfx::Matrix4x4 transform;
  ToMatrix4x4(GetEffectiveTransform(), transform);
  gfx::Rect clipRect(aClipRect.x, aClipRect.y, aClipRect.width, aClipRect.height);

  mImageHost->Composite(effectChain,
                        GetEffectiveOpacity(),
                        transform,
                        gfx::Point(aOffset.x, aOffset.y),
                        gfx::ToFilter(filter),
                        clipRect);
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

#ifdef MOZ_LAYERS_HAVE_LOG
nsACString&
CanvasLayerComposite::PrintInfo(nsACString& aTo, const char* aPrefix)
{
  CanvasLayer::PrintInfo(aTo, aPrefix);
  aTo += "\n";
  if (mImageHost && mImageHost->IsAttached()) {
    nsAutoCString pfx(aPrefix);
    pfx += "  ";
    mImageHost->PrintInfo(aTo, pfx.get());
  }
  return aTo;
}
#endif

