




#include "ipc/AutoOpenSurface.h"
#include "ImageHost.h"

#include "mozilla/layers/Effects.h"
#include "LayersLogging.h"
#include "nsPrintfCString.h"

namespace mozilla {

using namespace gfx;

namespace layers {

ImageHost::ImageHost(const TextureInfo& aTextureInfo)
  : CompositableHost(aTextureInfo)
  , mFrontBuffer(nullptr)
  , mHasPictureRect(false)
{}

ImageHost::~ImageHost() {}

void
ImageHost::UseTextureHost(TextureHost* aTexture)
{
  mFrontBuffer = aTexture;
}

TextureHost*
ImageHost::GetTextureHost()
{
  return mFrontBuffer;
}

void
ImageHost::Composite(EffectChain& aEffectChain,
                     float aOpacity,
                     const gfx::Matrix4x4& aTransform,
                     const gfx::Point& aOffset,
                     const gfx::Filter& aFilter,
                     const gfx::Rect& aClipRect,
                     const nsIntRegion* aVisibleRegion,
                     TiledLayerProperties* aLayerProperties)
{
  if (!GetCompositor()) {
    
    
    
    return;
  }
  if (!mFrontBuffer) {
    return;
  }
  if (!mFrontBuffer->Lock()) {
    NS_WARNING("failed to lock front buffer");
    return;
  }
  RefPtr<NewTextureSource> source = mFrontBuffer->GetTextureSources();
  if (!source) {
    return;
  }
  RefPtr<TexturedEffect> effect = CreateTexturedEffect(mFrontBuffer->GetFormat(),
                                                       source,
                                                       aFilter);
  aEffectChain.mPrimaryEffect = effect;
  TileIterator* it = source->AsTileIterator();
  if (it) {
    it->BeginTileIteration();
    do {
      nsIntRect tileRect = it->GetTileRect();
      gfx::Rect rect(tileRect.x, tileRect.y, tileRect.width, tileRect.height);
      GetCompositor()->DrawQuad(rect, aClipRect, aEffectChain,
                                aOpacity, aTransform, aOffset);
      GetCompositor()->DrawDiagnostics(gfx::Color(0.5,0.0,0.0,1.0),
                                       rect, aClipRect, aTransform, aOffset);
    } while (it->NextTile());
    it->EndTileIteration();
  } else {
    IntSize textureSize = source->GetSize();
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

    if (mFrontBuffer->GetFlags() & NeedsYFlip) {
      effect->mTextureCoords.y = effect->mTextureCoords.YMost();
      effect->mTextureCoords.height = -effect->mTextureCoords.height;
    }

    GetCompositor()->DrawQuad(rect, aClipRect, aEffectChain,
                              aOpacity, aTransform, aOffset);
    GetCompositor()->DrawDiagnostics(gfx::Color(1.0,0.1,0.1,1.0),
                                     rect, aClipRect, aTransform, aOffset);
  }
  mFrontBuffer->Unlock();
}

#ifdef MOZ_LAYERS_HAVE_LOG
void
ImageHost::PrintInfo(nsACString& aTo, const char* aPrefix)
{
  aTo += aPrefix;
  aTo += nsPrintfCString("ImageHost (0x%p)", this);

  AppendToString(aTo, mPictureRect, " [picture-rect=", "]");

  if (mFrontBuffer) {
    nsAutoCString pfx(aPrefix);
    pfx += "  ";
    aTo += "\n";
    mFrontBuffer->PrintInfo(aTo, pfx.get());
  }
}
#endif

void
ImageHost::Dump(FILE* aFile,
                const char* aPrefix,
                bool aDumpHtml)
{
  if (!aFile) {
    aFile = stderr;
  }
  if (mFrontBuffer) {
    fprintf(aFile, "%s", aPrefix);
    fprintf(aFile, aDumpHtml ? "<ul><li>TextureHost: "
                             : "TextureHost: ");
    DumpTextureHost(aFile, mFrontBuffer);
    fprintf(aFile, aDumpHtml ? " </li></ul> " : " ");
  }
}

LayerRenderState
ImageHost::GetRenderState()
{
  if (mFrontBuffer) {
    return mFrontBuffer->GetRenderState();
  }
  return LayerRenderState();
}

#ifdef MOZ_DUMP_PAINTING
already_AddRefed<gfxImageSurface>
ImageHost::GetAsSurface()
{
  return mFrontBuffer->GetAsSurface();
}
#endif

void
DeprecatedImageHostSingle::SetCompositor(Compositor* aCompositor) {
  CompositableHost::SetCompositor(aCompositor);
  if (mDeprecatedTextureHost) {
    mDeprecatedTextureHost->SetCompositor(aCompositor);
  }
}

void
DeprecatedImageHostSingle::EnsureDeprecatedTextureHost(TextureIdentifier aTextureId,
                                                       const SurfaceDescriptor& aSurface,
                                                       ISurfaceAllocator* aAllocator,
                                                       const TextureInfo& aTextureInfo)
{
  if (mDeprecatedTextureHost &&
      mDeprecatedTextureHost->GetBuffer() &&
      mDeprecatedTextureHost->GetBuffer()->type() == aSurface.type()) {
    return;
  }

  MakeDeprecatedTextureHost(aTextureId,
                  aSurface,
                  aAllocator,
                  aTextureInfo);
}

void
DeprecatedImageHostSingle::MakeDeprecatedTextureHost(TextureIdentifier aTextureId,
                                                     const SurfaceDescriptor& aSurface,
                                                     ISurfaceAllocator* aAllocator,
                                                     const TextureInfo& aTextureInfo)
{
  mDeprecatedTextureHost = DeprecatedTextureHost::CreateDeprecatedTextureHost(aSurface.type(),
                                                mTextureInfo.mDeprecatedTextureHostFlags,
                                                mTextureInfo.mTextureFlags);

  NS_ASSERTION(mDeprecatedTextureHost, "Failed to create texture host");

  Compositor* compositor = GetCompositor();
  if (compositor && mDeprecatedTextureHost) {
    mDeprecatedTextureHost->SetCompositor(compositor);
  }
}

LayerRenderState
DeprecatedImageHostSingle::GetRenderState()
{
  if (mDeprecatedTextureHost) {
    return mDeprecatedTextureHost->GetRenderState();
  }
  return LayerRenderState();
}

void
DeprecatedImageHostSingle::Composite(EffectChain& aEffectChain,
                                     float aOpacity,
                                     const gfx::Matrix4x4& aTransform,
                                     const gfx::Point& aOffset,
                                     const gfx::Filter& aFilter,
                                     const gfx::Rect& aClipRect,
                                     const nsIntRegion* aVisibleRegion,
                                     TiledLayerProperties* aLayerProperties)
{
  if (!mDeprecatedTextureHost) {
    NS_WARNING("Can't composite an invalid or null DeprecatedTextureHost");
    return;
  }

  if (!mDeprecatedTextureHost->IsValid()) {
    NS_WARNING("Can't composite an invalid DeprecatedTextureHost");
    return;
  }

  if (!GetCompositor()) {
    
    return;
  }

  if (!mDeprecatedTextureHost->Lock()) {
    NS_ASSERTION(false, "failed to lock texture host");
    return;
  }

  RefPtr<TexturedEffect> effect =
    CreateTexturedEffect(mDeprecatedTextureHost, aFilter);

  aEffectChain.mPrimaryEffect = effect;

  TileIterator* it = mDeprecatedTextureHost->AsTileIterator();
  if (it) {
    it->BeginTileIteration();
    do {
      nsIntRect tileRect = it->GetTileRect();
      gfx::Rect rect(tileRect.x, tileRect.y, tileRect.width, tileRect.height);
      GetCompositor()->DrawQuad(rect, aClipRect, aEffectChain,
                                aOpacity, aTransform, aOffset);
      GetCompositor()->DrawDiagnostics(gfx::Color(0.5,0.0,0.0,1.0),
                                       rect, aClipRect, aTransform, aOffset);
    } while (it->NextTile());
    it->EndTileIteration();
  } else {
    IntSize textureSize = mDeprecatedTextureHost->GetSize();
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

    if (mDeprecatedTextureHost->GetFlags() & NeedsYFlip) {
      effect->mTextureCoords.y = effect->mTextureCoords.YMost();
      effect->mTextureCoords.height = -effect->mTextureCoords.height;
    }

    GetCompositor()->DrawQuad(rect, aClipRect, aEffectChain,
                              aOpacity, aTransform, aOffset);
    GetCompositor()->DrawDiagnostics(gfx::Color(1.0,0.1,0.1,1.0),
                                     rect, aClipRect, aTransform, aOffset);
  }

  mDeprecatedTextureHost->Unlock();
}

#ifdef MOZ_LAYERS_HAVE_LOG
void
DeprecatedImageHostSingle::PrintInfo(nsACString& aTo, const char* aPrefix)
{
  aTo += aPrefix;
  aTo += nsPrintfCString("DeprecatedImageHostSingle (0x%p)", this);

  AppendToString(aTo, mPictureRect, " [picture-rect=", "]");

  if (mDeprecatedTextureHost) {
    nsAutoCString pfx(aPrefix);
    pfx += "  ";
    aTo += "\n";
    mDeprecatedTextureHost->PrintInfo(aTo, pfx.get());
  }
}
#endif

bool
DeprecatedImageHostBuffered::Update(const SurfaceDescriptor& aImage,
                                    SurfaceDescriptor* aResult) {
  if (!GetDeprecatedTextureHost()) {
    *aResult = aImage;
    return false;
  }
  GetDeprecatedTextureHost()->SwapTextures(aImage, aResult);
  return GetDeprecatedTextureHost()->IsValid();
}

void
DeprecatedImageHostBuffered::MakeDeprecatedTextureHost(TextureIdentifier aTextureId,
                                                       const SurfaceDescriptor& aSurface,
                                                       ISurfaceAllocator* aAllocator,
                                                       const TextureInfo& aTextureInfo)
{
  DeprecatedImageHostSingle::MakeDeprecatedTextureHost(aTextureId,
                                                       aSurface,
                                                       aAllocator,
                                                       aTextureInfo);
  if (mDeprecatedTextureHost) {
    mDeprecatedTextureHost->SetBuffer(new SurfaceDescriptor(null_t()), aAllocator);
  }
}

void
DeprecatedImageHostSingle::Dump(FILE* aFile,
                                const char* aPrefix,
                                bool aDumpHtml)
{
  if (!aFile) {
    aFile = stderr;
  }
  if (mDeprecatedTextureHost) {
    fprintf(aFile, "%s", aPrefix);
    fprintf(aFile, aDumpHtml ? "<ul><li>DeprecatedTextureHost: "
                             : "DeprecatedTextureHost: ");
    DumpDeprecatedTextureHost(aFile, mDeprecatedTextureHost);
    fprintf(aFile, aDumpHtml ? " </li></ul> " : " ");
  }
}

#ifdef MOZ_DUMP_PAINTING
already_AddRefed<gfxImageSurface>
DeprecatedImageHostSingle::GetAsSurface()
{
  return mDeprecatedTextureHost->GetAsSurface();
}
#endif


}
}
