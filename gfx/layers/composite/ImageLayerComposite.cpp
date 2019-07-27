




#include "ImageLayerComposite.h"
#include "CompositableHost.h"           
#include "Layers.h"                     
#include "gfx2DGlue.h"                  
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
#include "nsRefPtr.h"                   
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsString.h"                   

namespace mozilla {
namespace layers {

using namespace mozilla::gfx;

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

bool
ImageLayerComposite::SetCompositableHost(CompositableHost* aHost)
{
  switch (aHost->GetType()) {
    case CompositableType::IMAGE:
    case CompositableType::IMAGE_OVERLAY:
      mImageHost = aHost;
      return true;
    default:
      return false;
  }
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
ImageLayerComposite::RenderLayer(const IntRect& aClipRect)
{
  if (!mImageHost || !mImageHost->IsAttached()) {
    return;
  }

#ifdef MOZ_DUMP_PAINTING
  if (gfxUtils::sDumpPainting) {
    RefPtr<gfx::DataSourceSurface> surf = mImageHost->GetAsSurface();
    WriteSnapshotToDumpFile(this, surf);
  }
#endif

  mCompositor->MakeCurrent();

  EffectChain effectChain(this);
  LayerManagerComposite::AutoAddMaskEffect autoMaskEffect(mMaskLayer, effectChain);
  AddBlendModeEffect(effectChain);

  gfx::Rect clipRect(aClipRect.x, aClipRect.y, aClipRect.width, aClipRect.height);
  mImageHost->SetCompositor(mCompositor);
  mImageHost->Composite(effectChain,
                        GetEffectiveOpacity(),
                        GetEffectiveTransformForBuffer(),
                        GetEffectFilter(),
                        clipRect);
  mImageHost->BumpFlashCounter();
}

void
ImageLayerComposite::ComputeEffectiveTransforms(const gfx::Matrix4x4& aTransformToSurface)
{
  gfx::Matrix4x4 local = GetLocalTransform();

  
  gfxRect sourceRect(0, 0, 0, 0);
  if (mImageHost &&
      mImageHost->IsAttached()) {
    IntSize size = mImageHost->GetImageSize();
    sourceRect.SizeTo(size.width, size.height);
  }
  
  
  
  
  mEffectiveTransform =
      SnapTransform(local, sourceRect, nullptr) *
      SnapTransformTranslation(aTransformToSurface, nullptr);

  if (mScaleMode != ScaleMode::SCALE_NONE &&
      sourceRect.width != 0.0 && sourceRect.height != 0.0) {
    NS_ASSERTION(mScaleMode == ScaleMode::STRETCH,
                 "No other scalemodes than stretch and none supported yet.");
    local.PreScale(mScaleToSize.width / sourceRect.width,
                   mScaleToSize.height / sourceRect.height, 1.0);

    mEffectiveTransformForBuffer =
        SnapTransform(local, sourceRect, nullptr) *
        SnapTransformTranslation(aTransformToSurface, nullptr);
  } else {
    mEffectiveTransformForBuffer = mEffectiveTransform;
  }

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

gfx::Filter
ImageLayerComposite::GetEffectFilter()
{
  return gfx::ToFilter(mFilter);
}

void
ImageLayerComposite::GenEffectChain(EffectChain& aEffect)
{
  aEffect.mLayerRef = this;
  aEffect.mPrimaryEffect = mImageHost->GenEffect(GetEffectFilter());
}

void
ImageLayerComposite::PrintInfo(std::stringstream& aStream, const char* aPrefix)
{
  ImageLayer::PrintInfo(aStream, aPrefix);
  if (mImageHost && mImageHost->IsAttached()) {
    aStream << "\n";
    nsAutoCString pfx(aPrefix);
    pfx += "  ";
    mImageHost->PrintInfo(aStream, pfx.get());
  }
}

} 
} 
