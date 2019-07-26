




#include "ipc/AutoOpenSurface.h"
#include "ImageHost.h"

#include "mozilla/layers/Effects.h"
#include "LayersLogging.h"
#include "nsPrintfCString.h"

namespace mozilla {

using namespace gfx;

namespace layers {

void
ImageHostSingle::SetCompositor(Compositor* aCompositor) {
  CompositableHost::SetCompositor(aCompositor);
  if (mTextureHost) {
    mTextureHost->SetCompositor(aCompositor);
  }
}

bool
ImageHostSingle::EnsureTextureHost(TextureIdentifier aTextureId,
                                   const SurfaceDescriptor& aSurface,
                                   ISurfaceAllocator* aAllocator,
                                   const TextureInfo& aTextureInfo)
{
  if (mTextureHost &&
      mTextureHost->GetBuffer() &&
      mTextureHost->GetBuffer()->type() == aSurface.type()) {
    return false;
  }

  mTextureHost = TextureHost::CreateTextureHost(aSurface.type(),
                                                mTextureInfo.mTextureHostFlags,
                                                mTextureInfo.mTextureFlags);

  Compositor* compositor = GetCompositor();
  if (compositor) {
    mTextureHost->SetCompositor(compositor);
  }

  return true;
}

void
ImageHostSingle::Composite(EffectChain& aEffectChain,
                           float aOpacity,
                           const gfx::Matrix4x4& aTransform,
                           const gfx::Point& aOffset,
                           const gfx::Filter& aFilter,
                           const gfx::Rect& aClipRect,
                           const nsIntRegion* aVisibleRegion,
                           TiledLayerProperties* aLayerProperties)
{
  if (!mTextureHost) {
    NS_WARNING("Can't composite an invalid or null TextureHost");
    return;
  }

  if (!mTextureHost->IsValid()) {
    NS_WARNING("Can't composite an invalid TextureHost");
    return;
  }

  if (!GetCompositor()) {
    
    return;
  }

  if (!mTextureHost->Lock()) {
    NS_ASSERTION(false, "failed to lock texture host");
    return;
  }

  RefPtr<TexturedEffect> effect =
    CreateTexturedEffect(mTextureHost, aFilter);

  aEffectChain.mPrimaryEffect = effect;

  TileIterator* it = mTextureHost->AsTileIterator();
  if (it) {
    it->BeginTileIteration();
    do {
      nsIntRect tileRect = it->GetTileRect();
      gfx::Rect rect(tileRect.x, tileRect.y, tileRect.width, tileRect.height);
      GetCompositor()->DrawQuad(rect, aClipRect, aEffectChain,
                                aOpacity, aTransform, aOffset);
    } while (it->NextTile());
    it->EndTileIteration();
  } else {
    IntSize textureSize = mTextureHost->GetSize();
    gfx::Rect rect(0, 0,
                   mPictureRect.width,
                   mPictureRect.height);
    if (mHasPictureRect) {
      effect->mTextureCoords = Rect(Float(mPictureRect.x) / textureSize.width,
                                    Float(mPictureRect.y) / textureSize.height,
                                    Float(mPictureRect.width) / textureSize.width,
                                    Float(mPictureRect.height) / textureSize.height);
    } else {
      effect->mTextureCoords = Rect(0, 0, 1, 1);
      rect = gfx::Rect(0, 0, textureSize.width, textureSize.height);
    }

    if (mTextureHost->GetFlags() & NeedsYFlip) {
      effect->mTextureCoords.y = effect->mTextureCoords.YMost();
      effect->mTextureCoords.height = -effect->mTextureCoords.height;
    }

    GetCompositor()->DrawQuad(rect, aClipRect, aEffectChain,
                              aOpacity, aTransform, aOffset);
  }

  mTextureHost->Unlock();
}

#ifdef MOZ_LAYERS_HAVE_LOG
void
ImageHostSingle::PrintInfo(nsACString& aTo, const char* aPrefix)
{
  aTo += aPrefix;
  aTo += nsPrintfCString("ImageHostSingle (0x%p)", this);

  AppendToString(aTo, mPictureRect, " [picture-rect=", "]");

  if (mTextureHost) {
    nsAutoCString pfx(aPrefix);
    pfx += "  ";
    aTo += "\n";
    mTextureHost->PrintInfo(aTo, pfx.get());
  }
}
#endif

bool
ImageHostBuffered::Update(const SurfaceDescriptor& aImage,
                          SurfaceDescriptor* aResult) {
  if (!GetTextureHost()) {
    *aResult = aImage;
    return false;
  }
  GetTextureHost()->SwapTextures(aImage, aResult);
  return GetTextureHost()->IsValid();
}

bool
ImageHostBuffered::EnsureTextureHost(TextureIdentifier aTextureId,
                                     const SurfaceDescriptor& aSurface,
                                     ISurfaceAllocator* aAllocator,
                                     const TextureInfo& aTextureInfo)
{
  bool result = ImageHostSingle::EnsureTextureHost(aTextureId,
                                                   aSurface,
                                                   aAllocator,
                                                   aTextureInfo);
  if (result) {
    mTextureHost->SetBuffer(new SurfaceDescriptor(null_t()), aAllocator);
    mPictureRect = nsIntRect(0, 0, -1, -1);
  }

  return result;
}

}
}
