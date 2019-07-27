




#include "ImageHost.h"
#include "LayersLogging.h"              
#include "composite/CompositableHost.h"  
#include "ipc/IPCMessageUtils.h"        
#include "mozilla/layers/Compositor.h"  
#include "mozilla/layers/Effects.h"     
#include "nsAString.h"
#include "nsDebug.h"                    
#include "nsPrintfCString.h"            
#include "nsString.h"                   

class nsIntRegion;

namespace mozilla {
namespace gfx {
class Matrix4x4;
}

using namespace gfx;

namespace layers {

class ISurfaceAllocator;

ImageHost::ImageHost(const TextureInfo& aTextureInfo)
  : CompositableHost(aTextureInfo)
  , mHasPictureRect(false)
  , mLocked(false)
{}

ImageHost::~ImageHost()
{
}

void
ImageHost::UseTextureHost(TextureHost* aTexture)
{
  CompositableHost::UseTextureHost(aTexture);
  mFrontBuffer = aTexture;
  if (mFrontBuffer) {
    mFrontBuffer->PrepareTextureSource(mTextureSource);
  }
}

void
ImageHost::RemoveTextureHost(TextureHost* aTexture)
{
  CompositableHost::RemoveTextureHost(aTexture);
  if (aTexture && mFrontBuffer == aTexture) {
    mFrontBuffer->UnbindTextureSource();
    mTextureSource = nullptr;
    mFrontBuffer = nullptr;
  }
}

TextureHost*
ImageHost::GetAsTextureHost()
{
  return mFrontBuffer;
}

void
ImageHost::Composite(EffectChain& aEffectChain,
                     float aOpacity,
                     const gfx::Matrix4x4& aTransform,
                     const gfx::Filter& aFilter,
                     const gfx::Rect& aClipRect,
                     const nsIntRegion* aVisibleRegion)
{
  if (!GetCompositor()) {
    
    
    
    return;
  }
  if (!mFrontBuffer) {
    return;
  }

  
  mFrontBuffer->SetCompositor(GetCompositor());

  AutoLockCompositableHost autoLock(this);
  if (autoLock.Failed()) {
    NS_WARNING("failed to lock front buffer");
    return;
  }

  if (!mFrontBuffer->BindTextureSource(mTextureSource)) {
    return;
  }

  if (!mTextureSource) {
    
    MOZ_ASSERT(false);
    return;
  }

  bool isAlphaPremultiplied = !(mFrontBuffer->GetFlags() & TextureFlags::NON_PREMULTIPLIED);
  RefPtr<TexturedEffect> effect = CreateTexturedEffect(mFrontBuffer->GetFormat(),
                                                       mTextureSource.get(),
                                                       aFilter,
                                                       isAlphaPremultiplied);
  if (!effect) {
    return;
  }

  aEffectChain.mPrimaryEffect = effect;
  IntSize textureSize = mTextureSource->GetSize();
  gfx::Rect gfxPictureRect
    = mHasPictureRect ? gfx::Rect(0, 0, mPictureRect.width, mPictureRect.height)
                      : gfx::Rect(0, 0, textureSize.width, textureSize.height);

  gfx::Rect pictureRect(0, 0,
                        mPictureRect.width,
                        mPictureRect.height);
  BigImageIterator* it = mTextureSource->AsBigImageIterator();
  if (it) {

    
    
    
    
    
    
    
    
    
    
    
    
    
    MOZ_ASSERT(it->GetTileCount() == 1 || !mTextureSource->GetNextSibling(),
               "Can't handle multi-plane BigImages");

    it->BeginBigImageIteration();
    do {
      nsIntRect tileRect = it->GetTileRect();
      gfx::Rect rect(tileRect.x, tileRect.y, tileRect.width, tileRect.height);
      if (mHasPictureRect) {
        rect = rect.Intersect(pictureRect);
        effect->mTextureCoords = Rect(Float(rect.x - tileRect.x)/ tileRect.width,
                                      Float(rect.y - tileRect.y) / tileRect.height,
                                      Float(rect.width) / tileRect.width,
                                      Float(rect.height) / tileRect.height);
      } else {
        effect->mTextureCoords = Rect(0, 0, 1, 1);
      }
      if (mFrontBuffer->GetFlags() & TextureFlags::ORIGIN_BOTTOM_LEFT) {
        effect->mTextureCoords.y = effect->mTextureCoords.YMost();
        effect->mTextureCoords.height = -effect->mTextureCoords.height;
      }
      GetCompositor()->DrawQuad(rect, aClipRect, aEffectChain,
                                aOpacity, aTransform);
      GetCompositor()->DrawDiagnostics(DiagnosticFlags::IMAGE | DiagnosticFlags::BIGIMAGE,
                                       rect, aClipRect, aTransform, mFlashCounter);
    } while (it->NextTile());
    it->EndBigImageIteration();
    
    GetCompositor()->DrawDiagnostics(DiagnosticFlags::IMAGE,
                                     gfxPictureRect, aClipRect,
                                     aTransform, mFlashCounter);
  } else {
    IntSize textureSize = mTextureSource->GetSize();
    gfx::Rect rect;
    if (mHasPictureRect) {
      effect->mTextureCoords = Rect(Float(mPictureRect.x) / textureSize.width,
                                    Float(mPictureRect.y) / textureSize.height,
                                    Float(mPictureRect.width) / textureSize.width,
                                    Float(mPictureRect.height) / textureSize.height);
      rect = pictureRect;
    } else {
      effect->mTextureCoords = Rect(0, 0, 1, 1);
      rect = gfx::Rect(0, 0, textureSize.width, textureSize.height);
    }

    if (mFrontBuffer->GetFlags() & TextureFlags::ORIGIN_BOTTOM_LEFT) {
      effect->mTextureCoords.y = effect->mTextureCoords.YMost();
      effect->mTextureCoords.height = -effect->mTextureCoords.height;
    }

    GetCompositor()->DrawQuad(rect, aClipRect, aEffectChain,
                              aOpacity, aTransform);
    GetCompositor()->DrawDiagnostics(DiagnosticFlags::IMAGE,
                                     rect, aClipRect,
                                     aTransform, mFlashCounter);
  }
}

void
ImageHost::SetCompositor(Compositor* aCompositor)
{
  if (mFrontBuffer && mCompositor != aCompositor) {
    mFrontBuffer->SetCompositor(aCompositor);
  }
  CompositableHost::SetCompositor(aCompositor);
}

void
ImageHost::PrintInfo(std::stringstream& aStream, const char* aPrefix)
{
  aStream << aPrefix;
  aStream << nsPrintfCString("ImageHost (0x%p)", this).get();

  AppendToString(aStream, mPictureRect, " [picture-rect=", "]");

  if (mFrontBuffer) {
    nsAutoCString pfx(aPrefix);
    pfx += "  ";
    aStream << "\n";
    mFrontBuffer->PrintInfo(aStream, pfx.get());
  }
}

void
ImageHost::Dump(std::stringstream& aStream,
                const char* aPrefix,
                bool aDumpHtml)
{
  if (mFrontBuffer) {
    aStream << aPrefix;
    aStream << (aDumpHtml ? "<ul><li>TextureHost: "
                             : "TextureHost: ");
    DumpTextureHost(aStream, mFrontBuffer);
    aStream << (aDumpHtml ? " </li></ul> " : " ");
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

TemporaryRef<gfx::DataSourceSurface>
ImageHost::GetAsSurface()
{
  return mFrontBuffer->GetAsSurface();
}

bool
ImageHost::Lock()
{
  MOZ_ASSERT(!mLocked);
  if (!mFrontBuffer->Lock()) {
    return false;
  }
  mLocked = true;
  return true;
}

void
ImageHost::Unlock()
{
  MOZ_ASSERT(mLocked);
  mFrontBuffer->Unlock();
  mLocked = false;
}

IntSize
ImageHost::GetImageSize() const
{
  if (mHasPictureRect) {
    return IntSize(mPictureRect.width, mPictureRect.height);
  }

  if (mFrontBuffer) {
    return mFrontBuffer->GetSize();
  }

  return IntSize();
}

TemporaryRef<TexturedEffect>
ImageHost::GenEffect(const gfx::Filter& aFilter)
{
  if (!mFrontBuffer->BindTextureSource(mTextureSource)) {
    return nullptr;
  }
  bool isAlphaPremultiplied = true;
  if (mFrontBuffer->GetFlags() & TextureFlags::NON_PREMULTIPLIED)
    isAlphaPremultiplied = false;

  return CreateTexturedEffect(mFrontBuffer->GetFormat(),
                              mTextureSource,
                              aFilter,
                              isAlphaPremultiplied);
}

#ifdef MOZ_WIDGET_GONK
ImageHostOverlay::ImageHostOverlay(const TextureInfo& aTextureInfo)
  : CompositableHost(aTextureInfo)
  , mHasPictureRect(false)
{
}

ImageHostOverlay::~ImageHostOverlay()
{
}

void
ImageHostOverlay::Composite(EffectChain& aEffectChain,
                            float aOpacity,
                            const gfx::Matrix4x4& aTransform,
                            const gfx::Filter& aFilter,
                            const gfx::Rect& aClipRect,
                            const nsIntRegion* aVisibleRegion)
{
  if (!GetCompositor()) {
    return;
  }

  if (mOverlay.handle().type() == OverlayHandle::Tnull_t)
    return;
  Color hollow(0.0f, 0.0f, 0.0f, 0.0f);
  aEffectChain.mPrimaryEffect = new EffectSolidColor(hollow);
  aEffectChain.mSecondaryEffects[EffectTypes::BLEND_MODE] = new EffectBlendMode(CompositionOp::OP_SOURCE);

  gfx::Rect rect;
  gfx::Rect clipRect(aClipRect.x, aClipRect.y,
                     aClipRect.width, aClipRect.height);
  if (mHasPictureRect) {
    rect.SetRect(mPictureRect.x, mPictureRect.y,
                 mPictureRect.width, mPictureRect.height);
  } else {
    rect.SetRect(0, 0,
                 mOverlay.size().width, mOverlay.size().height);
  }

  mCompositor->DrawQuad(rect, aClipRect, aEffectChain, aOpacity, aTransform);
  mCompositor->DrawDiagnostics(DiagnosticFlags::IMAGE | DiagnosticFlags::BIGIMAGE,
                               rect, aClipRect, aTransform, mFlashCounter);
}

LayerRenderState
ImageHostOverlay::GetRenderState()
{
  LayerRenderState state;
  if (mOverlay.handle().type() == OverlayHandle::Tint32_t) {
    state.SetOverlayId(mOverlay.handle().get_int32_t());
  }
  return state;
}

void
ImageHostOverlay::UseOverlaySource(OverlaySource aOverlay)
{
  mOverlay = aOverlay;
}

void
ImageHostOverlay::PrintInfo(std::stringstream& aStream, const char* aPrefix)
{
  aStream << aPrefix;
  aStream << nsPrintfCString("ImageHost (0x%p)", this).get();

  AppendToString(aStream, mPictureRect, " [picture-rect=", "]");

  if (mOverlay.handle().type() == OverlayHandle::Tint32_t) {
    nsAutoCString pfx(aPrefix);
    pfx += "  ";
    aStream << nsPrintfCString("Overlay: %d", mOverlay.handle().get_int32_t()).get();
  }
}

#endif
}
}
