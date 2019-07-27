




#include "mozilla/layers/ContentHost.h"
#include "LayersLogging.h"              
#include "gfx2DGlue.h"                  
#include "mozilla/gfx/Point.h"          
#include "mozilla/Assertions.h"         
#include "mozilla/gfx/BaseRect.h"       
#include "mozilla/layers/Compositor.h"  
#include "mozilla/layers/Effects.h"     
#include "mozilla/layers/LayersMessages.h"  
#include "nsAString.h"
#include "nsPrintfCString.h"            
#include "nsString.h"                   
#include "mozilla/layers/TextureHostOGL.h"  

namespace mozilla {
namespace gfx {
class Matrix4x4;
}
using namespace gfx;

namespace layers {

ContentHostBase::ContentHostBase(const TextureInfo& aTextureInfo)
  : ContentHost(aTextureInfo)
  , mInitialised(false)
{}

ContentHostBase::~ContentHostBase()
{
}

void
ContentHostTexture::Composite(EffectChain& aEffectChain,
                              float aOpacity,
                              const gfx::Matrix4x4& aTransform,
                              const Filter& aFilter,
                              const Rect& aClipRect,
                              const nsIntRegion* aVisibleRegion)
{
  NS_ASSERTION(aVisibleRegion, "Requires a visible region");

  AutoLockCompositableHost lock(this);
  if (lock.Failed()) {
    return;
  }

  if (!mTextureHost->BindTextureSource(mTextureSource)) {
    return;
  }
  MOZ_ASSERT(mTextureSource.get());

  if (!mTextureHostOnWhite) {
    mTextureSourceOnWhite = nullptr;
  }
  if (mTextureHostOnWhite && !mTextureHostOnWhite->BindTextureSource(mTextureSourceOnWhite)) {
    return;
  }

  RefPtr<TexturedEffect> effect = CreateTexturedEffect(mTextureSource.get(),
                                                       mTextureSourceOnWhite.get(),
                                                       aFilter, true);
  if (!effect) {
    return;
  }

  aEffectChain.mPrimaryEffect = effect;

  nsIntRegion tmpRegion;
  const nsIntRegion* renderRegion;
#ifndef MOZ_IGNORE_PAINT_WILL_RESAMPLE
  if (PaintWillResample()) {
    
    
    
    tmpRegion = aVisibleRegion->GetBounds();
    renderRegion = &tmpRegion;
  } else {
    renderRegion = aVisibleRegion;
  }
#else
  renderRegion = aVisibleRegion;
#endif

  nsIntRegion region(*renderRegion);
  nsIntPoint origin = GetOriginOffset();
  
  region.MoveBy(-origin);

  
  gfx::IntSize texSize = mTextureSource->GetSize();
  nsIntRect textureRect = nsIntRect(0, 0, texSize.width, texSize.height);
  textureRect.MoveBy(region.GetBounds().TopLeft());
  nsIntRegion subregion;
  subregion.And(region, textureRect);
  if (subregion.IsEmpty()) {
    
    return;
  }

  nsIntRegion screenRects;
  nsIntRegion regionRects;

  
  nsIntRegionRectIterator iter(subregion);
  while (const nsIntRect* iterRect = iter.Next()) {
    nsIntRect regionRect = *iterRect;
    nsIntRect screenRect = regionRect;
    screenRect.MoveBy(origin);

    screenRects.Or(screenRects, screenRect);
    regionRects.Or(regionRects, regionRect);
  }

  BigImageIterator* bigImgIter = mTextureSource->AsBigImageIterator();
  BigImageIterator* iterOnWhite = nullptr;
  if (bigImgIter) {
    bigImgIter->BeginBigImageIteration();
  }

  if (mTextureSourceOnWhite) {
    iterOnWhite = mTextureSourceOnWhite->AsBigImageIterator();
    MOZ_ASSERT(!bigImgIter || bigImgIter->GetTileCount() == iterOnWhite->GetTileCount(),
               "Tile count mismatch on component alpha texture");
    if (iterOnWhite) {
      iterOnWhite->BeginBigImageIteration();
    }
  }

  bool usingTiles = (bigImgIter && bigImgIter->GetTileCount() > 1);
  do {
    if (iterOnWhite) {
      MOZ_ASSERT(iterOnWhite->GetTileRect() == bigImgIter->GetTileRect(),
                 "component alpha textures should be the same size.");
    }

    nsIntRect texRect = bigImgIter ? bigImgIter->GetTileRect()
                                 : nsIntRect(0, 0,
                                             texSize.width,
                                             texSize.height);

    
    
    
    
    for (int y = 0; y < (usingTiles ? 2 : 1); y++) {
      for (int x = 0; x < (usingTiles ? 2 : 1); x++) {
        nsIntRect currentTileRect(texRect);
        currentTileRect.MoveBy(x * texSize.width, y * texSize.height);

        nsIntRegionRectIterator screenIter(screenRects);
        nsIntRegionRectIterator regionIter(regionRects);

        const nsIntRect* screenRect;
        const nsIntRect* regionRect;
        while ((screenRect = screenIter.Next()) &&
               (regionRect = regionIter.Next())) {
          nsIntRect tileScreenRect(*screenRect);
          nsIntRect tileRegionRect(*regionRect);

          
          
          
          if (usingTiles) {
            tileScreenRect.MoveBy(-origin);
            tileScreenRect = tileScreenRect.Intersect(currentTileRect);
            tileScreenRect.MoveBy(origin);

            if (tileScreenRect.IsEmpty())
              continue;

            tileRegionRect = regionRect->Intersect(currentTileRect);
            tileRegionRect.MoveBy(-currentTileRect.TopLeft());
          }
          gfx::Rect rect(tileScreenRect.x, tileScreenRect.y,
                         tileScreenRect.width, tileScreenRect.height);

          effect->mTextureCoords = Rect(Float(tileRegionRect.x) / texRect.width,
                                        Float(tileRegionRect.y) / texRect.height,
                                        Float(tileRegionRect.width) / texRect.width,
                                        Float(tileRegionRect.height) / texRect.height);
          GetCompositor()->DrawQuad(rect, aClipRect, aEffectChain, aOpacity, aTransform);
          if (usingTiles) {
            DiagnosticFlags diagnostics = DiagnosticFlags::CONTENT | DiagnosticFlags::BIGIMAGE;
            if (iterOnWhite) {
              diagnostics |= DiagnosticFlags::COMPONENT_ALPHA;
            }
            GetCompositor()->DrawDiagnostics(diagnostics, rect, aClipRect,
                                             aTransform, mFlashCounter);
          }
        }
      }
    }

    if (iterOnWhite) {
      iterOnWhite->NextTile();
    }
  } while (usingTiles && bigImgIter->NextTile());

  if (bigImgIter) {
    bigImgIter->EndBigImageIteration();
  }
  if (iterOnWhite) {
    iterOnWhite->EndBigImageIteration();
  }

  DiagnosticFlags diagnostics = DiagnosticFlags::CONTENT;
  if (iterOnWhite) {
    diagnostics |= DiagnosticFlags::COMPONENT_ALPHA;
  }
  GetCompositor()->DrawDiagnostics(diagnostics, nsIntRegion(mBufferRect), aClipRect,
                                   aTransform, mFlashCounter);
}

void
ContentHostTexture::UseTextureHost(TextureHost* aTexture)
{
  ContentHostBase::UseTextureHost(aTexture);
  mTextureHost = aTexture;
  mTextureHostOnWhite = nullptr;
  mTextureSourceOnWhite = nullptr;
  if (mTextureHost) {
    mTextureHost->PrepareTextureSource(mTextureSource);
  }
}

void
ContentHostTexture::UseComponentAlphaTextures(TextureHost* aTextureOnBlack,
                                              TextureHost* aTextureOnWhite)
{
  ContentHostBase::UseComponentAlphaTextures(aTextureOnBlack, aTextureOnWhite);
  mTextureHost = aTextureOnBlack;
  mTextureHostOnWhite = aTextureOnWhite;
  if (mTextureHost) {
    mTextureHost->PrepareTextureSource(mTextureSource);
  }
  if (mTextureHostOnWhite) {
    mTextureHostOnWhite->PrepareTextureSource(mTextureSourceOnWhite);
  }
}

void
ContentHostTexture::SetCompositor(Compositor* aCompositor)
{
  ContentHostBase::SetCompositor(aCompositor);
  if (mTextureHost) {
    mTextureHost->SetCompositor(aCompositor);
  }
  if (mTextureHostOnWhite) {
    mTextureHostOnWhite->SetCompositor(aCompositor);
  }
}

void
ContentHostTexture::Dump(std::stringstream& aStream,
                         const char* aPrefix,
                         bool aDumpHtml)
{
#ifdef MOZ_DUMP_PAINTING
  if (!aDumpHtml) {
    return;
  }
  aStream << "<ul>";
  if (mTextureHost) {
    aStream << aPrefix;
    aStream << "<li> <a href=";
    DumpTextureHost(aStream, mTextureHost);
    aStream << "> Front buffer </a></li> ";
  }
  if (mTextureHostOnWhite) {
    aStream <<  aPrefix;
    aStream << "<li> <a href=";
    DumpTextureHost(aStream, mTextureHostOnWhite);
    aStream << "> Front buffer on white </a> </li> ";
  }
  aStream << "</ul>";
#endif
}

static inline void
AddWrappedRegion(const nsIntRegion& aInput, nsIntRegion& aOutput,
                 const nsIntSize& aSize, const nsIntPoint& aShift)
{
  nsIntRegion tempRegion;
  tempRegion.And(nsIntRect(aShift, aSize), aInput);
  tempRegion.MoveBy(-aShift);
  aOutput.Or(aOutput, tempRegion);
}

bool
ContentHostSingleBuffered::UpdateThebes(const ThebesBufferData& aData,
                                        const nsIntRegion& aUpdated,
                                        const nsIntRegion& aOldValidRegionBack,
                                        nsIntRegion* aUpdatedRegionBack)
{
  aUpdatedRegionBack->SetEmpty();

  if (!mTextureHost) {
    mInitialised = false;
    return true; 
  }              

  
  nsIntRegion destRegion(aUpdated);
  destRegion.MoveBy(-aData.rect().TopLeft());

  if (!aData.rect().Contains(aUpdated.GetBounds()) ||
      aData.rotation().x > aData.rect().width ||
      aData.rotation().y > aData.rect().height) {
    NS_ERROR("Invalid update data");
    return false;
  }

  
  
  
  

  
  destRegion.MoveBy(aData.rotation());

  nsIntSize bufferSize = aData.rect().Size();

  
  nsIntRegion finalRegion;
  finalRegion.And(IntRect(IntPoint(), bufferSize), destRegion);

  
  
  AddWrappedRegion(destRegion, finalRegion, bufferSize, nsIntPoint(aData.rect().width, 0));
  AddWrappedRegion(destRegion, finalRegion, bufferSize, nsIntPoint(aData.rect().width, aData.rect().height));
  AddWrappedRegion(destRegion, finalRegion, bufferSize, nsIntPoint(0, aData.rect().height));

  MOZ_ASSERT(nsIntRect(0, 0, aData.rect().width, aData.rect().height).Contains(finalRegion.GetBounds()));

  mTextureHost->Updated(&finalRegion);
  if (mTextureHostOnWhite) {
    mTextureHostOnWhite->Updated(&finalRegion);
  }
  mInitialised = true;

  mBufferRect = aData.rect();
  mBufferRotation = aData.rotation();

  return true;
}

bool
ContentHostDoubleBuffered::UpdateThebes(const ThebesBufferData& aData,
                                        const nsIntRegion& aUpdated,
                                        const nsIntRegion& aOldValidRegionBack,
                                        nsIntRegion* aUpdatedRegionBack)
{
  if (!mTextureHost) {
    mInitialised = false;

    *aUpdatedRegionBack = aUpdated;
    return true;
  }

  
  
  
  mTextureHost->Updated();
  if (mTextureHostOnWhite) {
    mTextureHostOnWhite->Updated();
  }
  mInitialised = true;

  mBufferRect = aData.rect();
  mBufferRotation = aData.rotation();

  *aUpdatedRegionBack = aUpdated;

  
  
  
  
  
  
  
  mValidRegionForNextBackBuffer = aOldValidRegionBack;

  return true;
}

void
ContentHostTexture::PrintInfo(std::stringstream& aStream, const char* aPrefix)
{
  aStream << aPrefix;
  aStream << nsPrintfCString("ContentHost (0x%p)", this).get();

  AppendToString(aStream, mBufferRect, " [buffer-rect=", "]");
  AppendToString(aStream, mBufferRotation, " [buffer-rotation=", "]");
  if (PaintWillResample()) {
    aStream << " [paint-will-resample]";
  }

  if (mTextureHost) {
    nsAutoCString pfx(aPrefix);
    pfx += "  ";

    aStream << "\n";
    mTextureHost->PrintInfo(aStream, pfx.get());
  }
}


LayerRenderState
ContentHostTexture::GetRenderState()
{
  if (!mTextureHost) {
    return LayerRenderState();
  }

  LayerRenderState result = mTextureHost->GetRenderState();

  if (mBufferRotation != nsIntPoint()) {
    result.mFlags |= LayerRenderStateFlags::BUFFER_ROTATION;
  }
  result.SetOffset(GetOriginOffset());
  return result;
}

TemporaryRef<TexturedEffect>
ContentHostTexture::GenEffect(const gfx::Filter& aFilter)
{
  if (!mTextureHost) {
    return nullptr;
  }
  if (!mTextureHost->BindTextureSource(mTextureSource)) {
    return nullptr;
  }
  if (!mTextureHostOnWhite) {
    mTextureSourceOnWhite = nullptr;
  }
  if (mTextureHostOnWhite && !mTextureHostOnWhite->BindTextureSource(mTextureSourceOnWhite)) {
    return nullptr;
  }
  return CreateTexturedEffect(mTextureSource.get(),
                              mTextureSourceOnWhite.get(),
                              aFilter, true);
}

TemporaryRef<gfx::DataSourceSurface>
ContentHostTexture::GetAsSurface()
{
  if (!mTextureHost) {
    return nullptr;
  }

  return mTextureHost->GetAsSurface();
}


} 
} 
