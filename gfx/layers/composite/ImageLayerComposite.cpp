




#include "gfxSharedImageSurface.h"

#include "ipc/AutoOpenSurface.h"
#include "ImageLayerComposite.h"
#include "ImageHost.h"
#include "gfxImageSurface.h"
#include "gfx2DGlue.h"

#include "mozilla/layers/Compositor.h"
#include "mozilla/layers/CompositorTypes.h" 
#include "mozilla/layers/Effects.h"
#include "CompositableHost.h"

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
  mImageHost = static_cast<ImageHost*>(aHost);
}

void
ImageLayerComposite::Disconnect()
{
  Destroy();
}

LayerRenderState
ImageLayerComposite::GetRenderState()
{
  if (!mImageHost) {
    return LayerRenderState();
  }
  return mImageHost->GetRenderState();
}

Layer*
ImageLayerComposite::GetLayer()
{
  return this;
}

void
ImageLayerComposite::RenderLayer(const nsIntPoint& aOffset,
                                 const nsIntRect& aClipRect)
{
  if (!mImageHost) {
    return;
  }

  mCompositor->MakeCurrent();

  EffectChain effectChain;
  LayerManagerComposite::AddMaskEffect(mMaskLayer, effectChain);

  gfx::Matrix4x4 transform;
  ToMatrix4x4(GetEffectiveTransform(), transform);
  gfx::Rect clipRect(aClipRect.x, aClipRect.y, aClipRect.width, aClipRect.height);
  mImageHost->SetCompositor(mCompositor);
  mImageHost->Composite(effectChain,
                        GetEffectiveOpacity(),
                        transform,
                        gfx::Point(aOffset.x, aOffset.y),
                        gfx::ToFilter(mFilter),
                        clipRect);
}

void 
ImageLayerComposite::ComputeEffectiveTransforms(const gfx3DMatrix& aTransformToSurface)
{
  gfx3DMatrix local = GetLocalTransform();

  
  gfxRect sourceRect(0, 0, 0, 0);
  if (mImageHost && mImageHost->GetTextureHost()) {
    IntSize size = mImageHost->GetTextureHost()->GetSize();
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
ImageLayerComposite::GetCompositableHost() {
  return mImageHost.get();
}

void
ImageLayerComposite::CleanupResources()
{
  if (mImageHost) {
    mImageHost->Detach();
  }
  mImageHost = nullptr;
}

#ifdef MOZ_LAYERS_HAVE_LOG
nsACString&
ImageLayerComposite::PrintInfo(nsACString& aTo, const char* aPrefix)
{
  ImageLayer::PrintInfo(aTo, aPrefix);
  aTo += "\n";
  if (mImageHost) {
    nsAutoCString pfx(aPrefix);
    pfx += "  ";
    mImageHost->PrintInfo(aTo, pfx.get());
  }
  return aTo;
}
#endif

} 
} 
