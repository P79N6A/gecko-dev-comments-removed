




#include "ImageLayerComposite.h"
#include "CompositableHost.h"           
#include "Layers.h"                     
#include "gfx2DGlue.h"                  
#include "gfx3DMatrix.h"                
#include "gfxImageSurface.h"            
#include "gfxPoint.h"                   
#include "gfxRect.h"                    
#include "gfxUtils.h"                   
#include "mozilla/Assertions.h"         
#include "mozilla/gfx/Matrix.h"         
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Rect.h"           
#include "mozilla/layers/Compositor.h"  
#include "mozilla/layers/Effects.h"     
#include "mozilla/layers/TextureHost.h"  
#include "mozilla/mozalloc.h"           
#include "nsAString.h"
#include "nsAutoPtr.h"                  
#include "nsDebug.h"                    
#include "nsPoint.h"                    
#include "nsRect.h"                     
#include "nsString.h"                   
#include "nsTraceRefcnt.h"              

using namespace mozilla::gfx;

namespace mozilla {
namespace layers {

ImageLayerComposite::ImageLayerComposite(LayerManagerComposite* aManager)
  : ImageLayer(aManager, nullptr)
  , LayerComposite(aManager)
  , mImageHost(nullptr)
{
  MOZ_COUNT_CTOR(ImageLayerComposite);
  mImplData = static_cast<LayerComposite*>(this);
}

ImageLayerComposite::~ImageLayerComposite()
{
  MOZ_COUNT_DTOR(ImageLayerComposite);
  MOZ_ASSERT(mDestroyed);

  CleanupResources();
}

void
ImageLayerComposite::SetCompositableHost(CompositableHost* aHost)
{
  mImageHost = aHost;
}

void
ImageLayerComposite::Disconnect()
{
  Destroy();
}

LayerRenderState
ImageLayerComposite::GetRenderState()
{
  if (mImageHost && mImageHost->IsAttached()) {
    return mImageHost->GetRenderState();
  }
  return LayerRenderState();
}

Layer*
ImageLayerComposite::GetLayer()
{
  return this;
}

void
ImageLayerComposite::RenderLayer(const nsIntRect& aClipRect)
{
  if (!mImageHost || !mImageHost->IsAttached()) {
    return;
  }

#ifdef MOZ_DUMP_PAINTING
  if (gfxUtils::sDumpPainting) {
    nsRefPtr<gfxImageSurface> surf = mImageHost->GetAsSurface();
    WriteSnapshotToDumpFile(this, surf);
  }
#endif

  mCompositor->MakeCurrent();

  EffectChain effectChain;
  LayerManagerComposite::AutoAddMaskEffect autoMaskEffect(mMaskLayer, effectChain);

  gfx::Matrix4x4 transform;
  ToMatrix4x4(GetEffectiveTransform(), transform);
  gfx::Rect clipRect(aClipRect.x, aClipRect.y, aClipRect.width, aClipRect.height);
  mImageHost->SetCompositor(mCompositor);
  mImageHost->Composite(effectChain,
                        GetEffectiveOpacity(),
                        transform,
                        gfx::ToFilter(mFilter),
                        clipRect);
}

void 
ImageLayerComposite::ComputeEffectiveTransforms(const gfx3DMatrix& aTransformToSurface)
{
  gfx3DMatrix local = GetLocalTransform();

  
  gfxRect sourceRect(0, 0, 0, 0);
  if (mImageHost &&
      mImageHost->IsAttached() &&
      (mImageHost->GetDeprecatedTextureHost() || mImageHost->GetTextureHost())) {
    IntSize size =
      mImageHost->GetTextureHost() ? mImageHost->GetTextureHost()->GetSize()
                                   : mImageHost->GetDeprecatedTextureHost()->GetSize();
    sourceRect.SizeTo(size.width, size.height);
    if (mScaleMode != SCALE_NONE &&
        sourceRect.width != 0.0 && sourceRect.height != 0.0) {
      NS_ASSERTION(mScaleMode == SCALE_STRETCH,
                   "No other scalemodes than stretch and none supported yet.");
      local.Scale(mScaleToSize.width / sourceRect.width,
                  mScaleToSize.height / sourceRect.height, 1.0);
    }
  }
  
  
  
  
  mEffectiveTransform =
      SnapTransform(local, sourceRect, nullptr) *
      SnapTransformTranslation(aTransformToSurface, nullptr);
  ComputeEffectiveTransformForMaskLayer(aTransformToSurface);
}

CompositableHost*
ImageLayerComposite::GetCompositableHost()
{
  if (mImageHost && mImageHost->IsAttached()) {
    return mImageHost.get();
  }

  return nullptr;
}

void
ImageLayerComposite::CleanupResources()
{
  if (mImageHost) {
    mImageHost->Detach(this);
  }
  mImageHost = nullptr;
}

#ifdef MOZ_LAYERS_HAVE_LOG
nsACString&
ImageLayerComposite::PrintInfo(nsACString& aTo, const char* aPrefix)
{
  ImageLayer::PrintInfo(aTo, aPrefix);
  aTo += "\n";
  if (mImageHost && mImageHost->IsAttached()) {
    nsAutoCString pfx(aPrefix);
    pfx += "  ";
    mImageHost->PrintInfo(aTo, pfx.get());
  }
  return aTo;
}
#endif

} 
} 
