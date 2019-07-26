




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
  , mFrontBuffer(nullptr)
  , mHasPictureRect(false)
{}

ImageHost::~ImageHost() {}

void
ImageHost::UseTextureHost(TextureHost* aTexture)
{
  CompositableHost::UseTextureHost(aTexture);
  mFrontBuffer = aTexture;
}

void
ImageHost::RemoveTextureHost(TextureHost* aTexture)
{
  CompositableHost::RemoveTextureHost(aTexture);
  if (aTexture && mFrontBuffer == aTexture) {
    aTexture->SetCompositableBackendSpecificData(nullptr);
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
                     const nsIntRegion* aVisibleRegion,
                     TiledLayerProperties* aLayerProperties)
{
  if (!GetCompositor()) {
    
    
    
    return;
  }
  if (!mFrontBuffer) {
    return;
  }

  
  mFrontBuffer->SetCompositor(GetCompositor());
  mFrontBuffer->SetCompositableBackendSpecificData(GetCompositableBackendSpecificData());

  AutoLockTextureHost autoLock(mFrontBuffer);
  if (autoLock.Failed()) {
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
  if (!effect) {
    return;
  }

  aEffectChain.mPrimaryEffect = effect;
  IntSize textureSize = source->GetSize();
  gfx::Rect gfxPictureRect
    = mHasPictureRect ? gfx::Rect(0, 0, mPictureRect.width, mPictureRect.height)
                      : gfx::Rect(0, 0, textureSize.width, textureSize.height);

  gfx::Rect pictureRect(0, 0,
                        mPictureRect.width,
                        mPictureRect.height);
  
  
  
  BigImageIterator* it = source->AsBigImageIterator();
  if (it) {
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
      if (mFrontBuffer->GetFlags() & TextureFlags::NEEDS_Y_FLIP) {
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
    IntSize textureSize = source->GetSize();
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

    if (mFrontBuffer->GetFlags() & TextureFlags::NEEDS_Y_FLIP) {
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

#ifdef MOZ_DUMP_PAINTING
void
ImageHost::Dump(FILE* aFile,
                const char* aPrefix,
                bool aDumpHtml)
{
  if (!aFile) {
    aFile = stderr;
  }
  if (mFrontBuffer) {
    fprintf_stderr(aFile, "%s", aPrefix);
    fprintf_stderr(aFile, aDumpHtml ? "<ul><li>TextureHost: "
                             : "TextureHost: ");
    DumpTextureHost(aFile, mFrontBuffer);
    fprintf_stderr(aFile, aDumpHtml ? " </li></ul> " : " ");
  }
}
#endif

LayerRenderState
ImageHost::GetRenderState()
{
  if (mFrontBuffer) {
    return mFrontBuffer->GetRenderState();
  }
  return LayerRenderState();
}

#ifdef MOZ_DUMP_PAINTING
TemporaryRef<gfx::DataSourceSurface>
ImageHost::GetAsSurface()
{
  return mFrontBuffer->GetAsSurface();
}
#endif

}
}
