




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
#include "ipc/AutoOpenSurface.h"        
#include "mozilla/layers/TextureHostOGL.h"  

namespace mozilla {
namespace gfx {
class Matrix4x4;
}
using namespace gfx;

namespace layers {

ContentHostBase::ContentHostBase(const TextureInfo& aTextureInfo)
  : ContentHost(aTextureInfo)
  , mPaintWillResample(false)
  , mInitialised(false)
{}

ContentHostBase::~ContentHostBase()
{
}

struct AutoLockContentHost
{
  AutoLockContentHost(ContentHostBase* aHost)
    : mHost(aHost)
  {
    mSucceeded = mHost->Lock();
  }

  ~AutoLockContentHost()
  {
    if (mSucceeded) {
      mHost->Unlock();
    }
  }

  bool Failed() { return !mSucceeded; }

  ContentHostBase* mHost;
  bool mSucceeded;
};

void
ContentHostBase::Composite(EffectChain& aEffectChain,
                           float aOpacity,
                           const gfx::Matrix4x4& aTransform,
                           const Filter& aFilter,
                           const Rect& aClipRect,
                           const nsIntRegion* aVisibleRegion,
                           TiledLayerProperties* aLayerProperties)
{
  NS_ASSERTION(aVisibleRegion, "Requires a visible region");

  AutoLockContentHost lock(this);
  if (lock.Failed()) {
    return;
  }

  RefPtr<NewTextureSource> source = GetTextureSource();
  RefPtr<NewTextureSource> sourceOnWhite = GetTextureSourceOnWhite();

  if (!source) {
    return;
  }
  RefPtr<TexturedEffect> effect =
    CreateTexturedEffect(source, sourceOnWhite, aFilter);

  if (!effect) {
    return;
  }

  aEffectChain.mPrimaryEffect = effect;

  nsIntRegion tmpRegion;
  const nsIntRegion* renderRegion;
  if (PaintWillResample()) {
    
    
    
    tmpRegion = aVisibleRegion->GetBounds();
    renderRegion = &tmpRegion;
  } else {
    renderRegion = aVisibleRegion;
  }

  nsIntRegion region(*renderRegion);
  nsIntPoint origin = GetOriginOffset();
  
  region.MoveBy(-origin);

  
  gfx::IntSize texSize = source->GetSize();
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

  TileIterator* tileIter = source->AsTileIterator();
  TileIterator* iterOnWhite = nullptr;
  if (tileIter) {
    tileIter->BeginTileIteration();
  }

  if (sourceOnWhite) {
    iterOnWhite = sourceOnWhite->AsTileIterator();
    MOZ_ASSERT(!tileIter || tileIter->GetTileCount() == iterOnWhite->GetTileCount(),
               "Tile count mismatch on component alpha texture");
    if (iterOnWhite) {
      iterOnWhite->BeginTileIteration();
    }
  }

  bool usingTiles = (tileIter && tileIter->GetTileCount() > 1);
  do {
    if (iterOnWhite) {
      MOZ_ASSERT(iterOnWhite->GetTileRect() == tileIter->GetTileRect(),
                 "component alpha textures should be the same size.");
    }

    nsIntRect texRect = tileIter ? tileIter->GetTileRect()
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
            DiagnosticTypes diagnostics = DIAGNOSTIC_CONTENT | DIAGNOSTIC_BIGIMAGE;
            diagnostics |= iterOnWhite ? DIAGNOSTIC_COMPONENT_ALPHA : 0;
            GetCompositor()->DrawDiagnostics(diagnostics, rect, aClipRect,
                                             aTransform, mFlashCounter);
          }
        }
      }
    }

    if (iterOnWhite) {
      iterOnWhite->NextTile();
    }
  } while (usingTiles && tileIter->NextTile());

  if (tileIter) {
    tileIter->EndTileIteration();
  }
  if (iterOnWhite) {
    iterOnWhite->EndTileIteration();
  }

  DiagnosticTypes diagnostics = DIAGNOSTIC_CONTENT;
  diagnostics |= iterOnWhite ? DIAGNOSTIC_COMPONENT_ALPHA : 0;
  GetCompositor()->DrawDiagnostics(diagnostics, *aVisibleRegion, aClipRect,
                                   aTransform, mFlashCounter);
}


void
ContentHostTexture::UseTextureHost(TextureHost* aTexture)
{
  ContentHostBase::UseTextureHost(aTexture);
  mTextureHost = aTexture;
  mTextureHostOnWhite = nullptr;
}

void
ContentHostTexture::UseComponentAlphaTextures(TextureHost* aTextureOnBlack,
                                              TextureHost* aTextureOnWhite)
{
  ContentHostBase::UseComponentAlphaTextures(aTextureOnBlack, aTextureOnWhite);
  mTextureHost = aTextureOnBlack;
  mTextureHostOnWhite = aTextureOnWhite;
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

#ifdef MOZ_DUMP_PAINTING
void
ContentHostTexture::Dump(FILE* aFile,
                         const char* aPrefix,
                         bool aDumpHtml)
{
  if (!aDumpHtml) {
    return;
  }
  if (!aFile) {
    aFile = stderr;
  }
  fprintf(aFile, "<ul>");
  if (mTextureHost) {
    fprintf(aFile, "%s", aPrefix);
    fprintf(aFile, "<li> <a href=");
    DumpTextureHost(aFile, mTextureHost);
    fprintf(aFile, "> Front buffer </a></li> ");
  }
  if (mTextureHostOnWhite) {
    fprintf(aFile, "%s", aPrefix);
    fprintf(aFile, "<li> <a href=");
    DumpTextureHost(aFile, mTextureHostOnWhite);
    fprintf(aFile, "> Front buffer on white </a> </li> ");
  }
  fprintf(aFile, "</ul>");
}
#endif

DeprecatedContentHostBase::DeprecatedContentHostBase(const TextureInfo& aTextureInfo)
  : ContentHost(aTextureInfo)
  , mPaintWillResample(false)
  , mInitialised(false)
{}

DeprecatedContentHostBase::~DeprecatedContentHostBase()
{}

DeprecatedTextureHost*
DeprecatedContentHostBase::GetDeprecatedTextureHost()
{
  return mDeprecatedTextureHost;
}

void
DeprecatedContentHostBase::DestroyFrontHost()
{
  MOZ_ASSERT(!mDeprecatedTextureHost || mDeprecatedTextureHost->GetDeAllocator(),
             "We won't be able to destroy our SurfaceDescriptor");
  MOZ_ASSERT(!mDeprecatedTextureHostOnWhite || mDeprecatedTextureHostOnWhite->GetDeAllocator(),
             "We won't be able to destroy our SurfaceDescriptor");
  mDeprecatedTextureHost = nullptr;
  mDeprecatedTextureHostOnWhite = nullptr;
}

void
DeprecatedContentHostBase::Composite(EffectChain& aEffectChain,
                           float aOpacity,
                           const gfx::Matrix4x4& aTransform,
                           const Filter& aFilter,
                           const Rect& aClipRect,
                           const nsIntRegion* aVisibleRegion,
                           TiledLayerProperties* aLayerProperties)
{
  NS_ASSERTION(aVisibleRegion, "Requires a visible region");

  AutoLockDeprecatedTextureHost lock(mDeprecatedTextureHost);
  AutoLockDeprecatedTextureHost lockOnWhite(mDeprecatedTextureHostOnWhite);

  if (!mDeprecatedTextureHost ||
      !lock.IsValid() ||
      !lockOnWhite.IsValid()) {
    return;
  }

  RefPtr<TexturedEffect> effect =
    CreateTexturedEffect(mDeprecatedTextureHost, mDeprecatedTextureHostOnWhite, aFilter);
  if (!effect) {
    return;
  }

  aEffectChain.mPrimaryEffect = effect;

  nsIntRegion tmpRegion;
  const nsIntRegion* renderRegion;
  if (PaintWillResample()) {
    
    
    
    tmpRegion = aVisibleRegion->GetBounds();
    renderRegion = &tmpRegion;
  } else {
    renderRegion = aVisibleRegion;
  }

  nsIntRegion region(*renderRegion);
  nsIntPoint origin = GetOriginOffset();
  region.MoveBy(-origin);           

  
  TextureSource* source = mDeprecatedTextureHost;
  MOZ_ASSERT(source);
  gfx::IntSize texSize = source->GetSize();
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

  TileIterator* tileIter = source->AsTileIterator();
  TileIterator* iterOnWhite = nullptr;
  if (tileIter) {
    tileIter->BeginTileIteration();
  }

  if (mDeprecatedTextureHostOnWhite) {
    iterOnWhite = mDeprecatedTextureHostOnWhite->AsTileIterator();
    MOZ_ASSERT(!tileIter || tileIter->GetTileCount() == iterOnWhite->GetTileCount(),
               "Tile count mismatch on component alpha texture");
    if (iterOnWhite) {
      iterOnWhite->BeginTileIteration();
    }
  }

  bool usingTiles = (tileIter && tileIter->GetTileCount() > 1);
  do {
    if (iterOnWhite) {
      MOZ_ASSERT(iterOnWhite->GetTileRect() == tileIter->GetTileRect(),
                 "component alpha textures should be the same size.");
    }

    nsIntRect texRect = tileIter ? tileIter->GetTileRect()
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
              DiagnosticTypes diagnostics = DIAGNOSTIC_CONTENT | DIAGNOSTIC_BIGIMAGE;
              diagnostics |= iterOnWhite ? DIAGNOSTIC_COMPONENT_ALPHA : 0;
              GetCompositor()->DrawDiagnostics(diagnostics, rect, aClipRect,
                                               aTransform, mFlashCounter);
            }
        }
      }
    }

    if (iterOnWhite) {
      iterOnWhite->NextTile();
    }
  } while (usingTiles && tileIter->NextTile());

  if (tileIter) {
    tileIter->EndTileIteration();
  }
  if (iterOnWhite) {
    iterOnWhite->EndTileIteration();
  }

  DiagnosticTypes diagnostics = DIAGNOSTIC_CONTENT;
  diagnostics |= iterOnWhite ? DIAGNOSTIC_COMPONENT_ALPHA : 0;
  GetCompositor()->DrawDiagnostics(diagnostics, *aVisibleRegion, aClipRect,
                                   aTransform, mFlashCounter);
}

void
DeprecatedContentHostBase::SetCompositor(Compositor* aCompositor)
{
  CompositableHost::SetCompositor(aCompositor);
  if (mDeprecatedTextureHost) {
    mDeprecatedTextureHost->SetCompositor(aCompositor);
  }
  if (mDeprecatedTextureHostOnWhite) {
    mDeprecatedTextureHostOnWhite->SetCompositor(aCompositor);
  }
}

#ifdef MOZ_DUMP_PAINTING

void
DeprecatedContentHostBase::Dump(FILE* aFile,
                      const char* aPrefix,
                      bool aDumpHtml)
{
  if (!aDumpHtml) {
    return;
  }
  if (!aFile) {
    aFile = stderr;
  }
  fprintf_stderr(aFile, "<ul>");
  if (mDeprecatedTextureHost) {
    fprintf_stderr(aFile, "%s", aPrefix);
    fprintf_stderr(aFile, "<li> <a href=");
    DumpDeprecatedTextureHost(aFile, mDeprecatedTextureHost);
    fprintf_stderr(aFile, "> Front buffer </a></li> ");
  }
  if (mDeprecatedTextureHostOnWhite) {
    fprintf_stderr(aFile, "%s", aPrefix);
    fprintf_stderr(aFile, "<li> <a href=");
    DumpDeprecatedTextureHost(aFile, mDeprecatedTextureHostOnWhite);
    fprintf_stderr(aFile, "> Front buffer on white </a> </li> ");
  }
  fprintf_stderr(aFile, "</ul>");
}

#endif

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

  
  destRegion.MoveBy(aData.rotation());

  IntSize size = aData.rect().Size().ToIntSize();
  nsIntRect destBounds = destRegion.GetBounds();
  destRegion.MoveBy((destBounds.x >= size.width) ? -size.width : 0,
                    (destBounds.y >= size.height) ? -size.height : 0);

  
  
  if((destBounds.x % size.width) + destBounds.width > size.width ||
     (destBounds.y % size.height) + destBounds.height > size.height)
  {
    NS_ERROR("updated region lies across rotation boundaries!");
    return false;
  }

  mTextureHost->Updated(&destRegion);
  if (mTextureHostOnWhite) {
    mTextureHostOnWhite->Updated(&destRegion);
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
ContentHostIncremental::CreatedIncrementalTexture(ISurfaceAllocator* aAllocator,
                                                  const TextureInfo& aTextureInfo,
                                                  const nsIntRect& aBufferRect)
{
  mUpdateList.AppendElement(new TextureCreationRequest(aTextureInfo,
                                                       aBufferRect));
  mDeAllocator = aAllocator;
  FlushUpdateQueue();
}

void
ContentHostIncremental::UpdateIncremental(TextureIdentifier aTextureId,
                                          SurfaceDescriptor& aSurface,
                                          const nsIntRegion& aUpdated,
                                          const nsIntRect& aBufferRect,
                                          const nsIntPoint& aBufferRotation)
{
  mUpdateList.AppendElement(new TextureUpdateRequest(mDeAllocator,
                                                     aTextureId,
                                                     aSurface,
                                                     aUpdated,
                                                     aBufferRect,
                                                     aBufferRotation));
  FlushUpdateQueue();
}

void
ContentHostIncremental::FlushUpdateQueue()
{
  
  
  
  
  static const uint32_t kMaxUpdateCount = 6;
  if (mUpdateList.Length() >= kMaxUpdateCount) {
    ProcessTextureUpdates();
  }
}

void
ContentHostIncremental::ProcessTextureUpdates()
{
  for (uint32_t i = 0; i < mUpdateList.Length(); i++) {
    mUpdateList[i]->Execute(this);
  }
  mUpdateList.Clear();
}

NewTextureSource*
ContentHostIncremental::GetTextureSource()
{
  MOZ_ASSERT(mLocked);
  return mSource;
}

NewTextureSource*
ContentHostIncremental::GetTextureSourceOnWhite()
{
  MOZ_ASSERT(mLocked);
  return mSourceOnWhite;
}

void
ContentHostIncremental::TextureCreationRequest::Execute(ContentHostIncremental* aHost)
{
  Compositor* compositor = aHost->GetCompositor();
  MOZ_ASSERT(compositor);

  RefPtr<DataTextureSource> temp =
    compositor->CreateDataTextureSource(mTextureInfo.mTextureFlags);
  MOZ_ASSERT(temp->AsSourceOGL() &&
             temp->AsSourceOGL()->AsTextureImageTextureSource());
  RefPtr<TextureImageTextureSourceOGL> newSource =
    temp->AsSourceOGL()->AsTextureImageTextureSource();

  RefPtr<TextureImageTextureSourceOGL> newSourceOnWhite;
  if (mTextureInfo.mTextureFlags & TEXTURE_COMPONENT_ALPHA) {
    temp =
      compositor->CreateDataTextureSource(mTextureInfo.mTextureFlags);
    MOZ_ASSERT(temp->AsSourceOGL() &&
               temp->AsSourceOGL()->AsTextureImageTextureSource());
    newSourceOnWhite = temp->AsSourceOGL()->AsTextureImageTextureSource();
  }

  if (mTextureInfo.mDeprecatedTextureHostFlags & TEXTURE_HOST_COPY_PREVIOUS) {
    nsIntRect bufferRect = aHost->mBufferRect;
    nsIntPoint bufferRotation = aHost->mBufferRotation;
    nsIntRect overlap;

    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    

    nsIntRect srcBufferSpaceBottomRight(bufferRotation.x, bufferRotation.y, bufferRect.width - bufferRotation.x, bufferRect.height - bufferRotation.y);
    nsIntRect srcBufferSpaceTopRight(bufferRotation.x, 0, bufferRect.width - bufferRotation.x, bufferRotation.y);
    nsIntRect srcBufferSpaceTopLeft(0, 0, bufferRotation.x, bufferRotation.y);
    nsIntRect srcBufferSpaceBottomLeft(0, bufferRotation.y, bufferRotation.x, bufferRect.height - bufferRotation.y);

    overlap.IntersectRect(bufferRect, mBufferRect);

    nsIntRect srcRect(overlap), dstRect(overlap);
    srcRect.MoveBy(- bufferRect.TopLeft() + bufferRotation);

    nsIntRect srcRectDrawTopRight(srcRect);
    nsIntRect srcRectDrawTopLeft(srcRect);
    nsIntRect srcRectDrawBottomLeft(srcRect);
    
    srcRectDrawTopRight  .MoveBy(-nsIntPoint(0, bufferRect.height));
    srcRectDrawTopLeft   .MoveBy(-nsIntPoint(bufferRect.width, bufferRect.height));
    srcRectDrawBottomLeft.MoveBy(-nsIntPoint(bufferRect.width, 0));

    
    srcRect               = srcRect              .Intersect(srcBufferSpaceBottomRight);
    srcRectDrawTopRight   = srcRectDrawTopRight  .Intersect(srcBufferSpaceTopRight);
    srcRectDrawTopLeft    = srcRectDrawTopLeft   .Intersect(srcBufferSpaceTopLeft);
    srcRectDrawBottomLeft = srcRectDrawBottomLeft.Intersect(srcBufferSpaceBottomLeft);

    dstRect = srcRect;
    nsIntRect dstRectDrawTopRight(srcRectDrawTopRight);
    nsIntRect dstRectDrawTopLeft(srcRectDrawTopLeft);
    nsIntRect dstRectDrawBottomLeft(srcRectDrawBottomLeft);

    
    dstRect              .MoveBy(-bufferRotation);
    dstRectDrawTopRight  .MoveBy(-bufferRotation + nsIntPoint(0, bufferRect.height));
    dstRectDrawTopLeft   .MoveBy(-bufferRotation + nsIntPoint(bufferRect.width, bufferRect.height));
    dstRectDrawBottomLeft.MoveBy(-bufferRotation + nsIntPoint(bufferRect.width, 0));

    
    dstRect              .MoveBy(bufferRect.TopLeft());
    dstRectDrawTopRight  .MoveBy(bufferRect.TopLeft());
    dstRectDrawTopLeft   .MoveBy(bufferRect.TopLeft());
    dstRectDrawBottomLeft.MoveBy(bufferRect.TopLeft());

    
    dstRect              .MoveBy(-mBufferRect.TopLeft());
    dstRectDrawTopRight  .MoveBy(-mBufferRect.TopLeft());
    dstRectDrawTopLeft   .MoveBy(-mBufferRect.TopLeft());
    dstRectDrawBottomLeft.MoveBy(-mBufferRect.TopLeft());

    newSource->EnsureBuffer(mBufferRect.Size(),
                           ContentForFormat(aHost->mSource->GetFormat()));

    aHost->mSource->CopyTo(srcRect, newSource, dstRect);
    if (bufferRotation != nsIntPoint(0, 0)) {
      
      
      

      if (!srcRectDrawTopRight.IsEmpty())
        aHost->mSource->CopyTo(srcRectDrawTopRight,
                               newSource, dstRectDrawTopRight);
      if (!srcRectDrawTopLeft.IsEmpty())
        aHost->mSource->CopyTo(srcRectDrawTopLeft,
                               newSource, dstRectDrawTopLeft);
      if (!srcRectDrawBottomLeft.IsEmpty())
        aHost->mSource->CopyTo(srcRectDrawBottomLeft,
                               newSource, dstRectDrawBottomLeft);
    }

    if (newSourceOnWhite) {
      newSourceOnWhite->EnsureBuffer(mBufferRect.Size(),
                                    ContentForFormat(aHost->mSourceOnWhite->GetFormat()));
      aHost->mSourceOnWhite->CopyTo(srcRect, newSourceOnWhite, dstRect);
      if (bufferRotation != nsIntPoint(0, 0)) {
        
        if (!srcRectDrawTopRight.IsEmpty())
          aHost->mSourceOnWhite->CopyTo(srcRectDrawTopRight,
                                        newSourceOnWhite, dstRectDrawTopRight);
        if (!srcRectDrawTopLeft.IsEmpty())
          aHost->mSourceOnWhite->CopyTo(srcRectDrawTopLeft,
                                        newSourceOnWhite, dstRectDrawTopLeft);
        if (!srcRectDrawBottomLeft.IsEmpty())
          aHost->mSourceOnWhite->CopyTo(srcRectDrawBottomLeft,
                                        newSourceOnWhite, dstRectDrawBottomLeft);
      }
    }
  }

  aHost->mSource = newSource;
  aHost->mSourceOnWhite = newSourceOnWhite;

  aHost->mBufferRect = mBufferRect;
  aHost->mBufferRotation = nsIntPoint();
}

nsIntRect
ContentHostIncremental::TextureUpdateRequest::GetQuadrantRectangle(XSide aXSide,
                                                                   YSide aYSide) const
{
  
  
  nsIntPoint quadrantTranslation = -mBufferRotation;
  quadrantTranslation.x += aXSide == LEFT ? mBufferRect.width : 0;
  quadrantTranslation.y += aYSide == TOP ? mBufferRect.height : 0;
  return mBufferRect + quadrantTranslation;
}

void
ContentHostIncremental::TextureUpdateRequest::Execute(ContentHostIncremental* aHost)
{
  nsIntRect drawBounds = mUpdated.GetBounds();

  aHost->mBufferRect = mBufferRect;
  aHost->mBufferRotation = mBufferRotation;

  
  int32_t xBoundary = mBufferRect.XMost() - mBufferRotation.x;
  int32_t yBoundary = mBufferRect.YMost() - mBufferRotation.y;
  XSide sideX = drawBounds.XMost() <= xBoundary ? RIGHT : LEFT;
  YSide sideY = drawBounds.YMost() <= yBoundary ? BOTTOM : TOP;
  nsIntRect quadrantRect = GetQuadrantRectangle(sideX, sideY);
  NS_ASSERTION(quadrantRect.Contains(drawBounds), "Messed up quadrants");

  mUpdated.MoveBy(-nsIntPoint(quadrantRect.x, quadrantRect.y));

  IntPoint offset = ToIntPoint(-mUpdated.GetBounds().TopLeft());

  AutoOpenSurface surf(OPEN_READ_ONLY, mDescriptor);

  nsRefPtr<gfxImageSurface> thebesSurf = surf.GetAsImage();
  RefPtr<DataSourceSurface> sourceSurf =
    gfx::Factory::CreateWrappingDataSourceSurface(thebesSurf->Data(),
                                                  thebesSurf->Stride(),
                                                  ToIntSize(thebesSurf->GetSize()),
                                                  ImageFormatToSurfaceFormat(thebesSurf->Format()));

  if (mTextureId == TextureFront) {
    aHost->mSource->Update(sourceSurf, &mUpdated, &offset);
  } else {
    aHost->mSourceOnWhite->Update(sourceSurf, &mUpdated, &offset);
  }
}

void
ContentHostTexture::PrintInfo(nsACString& aTo, const char* aPrefix)
{
  aTo += aPrefix;
  aTo += nsPrintfCString("ContentHost (0x%p)", this);

  AppendToString(aTo, mBufferRect, " [buffer-rect=", "]");
  AppendToString(aTo, mBufferRotation, " [buffer-rotation=", "]");
  if (PaintWillResample()) {
    aTo += " [paint-will-resample]";
  }

  nsAutoCString pfx(aPrefix);
  pfx += "  ";

  if (mTextureHost) {
    aTo += "\n";
    mTextureHost->PrintInfo(aTo, pfx.get());
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
    result.mFlags |= LAYER_RENDER_STATE_BUFFER_ROTATION;
  }
  result.SetOffset(GetOriginOffset());
  return result;
}

LayerRenderState
DeprecatedContentHostBase::GetRenderState()
{
  LayerRenderState result = mDeprecatedTextureHost->GetRenderState();

  if (mBufferRotation != nsIntPoint()) {
    result.mFlags |= LAYER_RENDER_STATE_BUFFER_ROTATION;
  }
  result.SetOffset(GetOriginOffset());
  return result;
}

#ifdef MOZ_DUMP_PAINTING
TemporaryRef<gfx::DataSourceSurface>
ContentHostTexture::GetAsSurface()
{
  if (!mTextureHost) {
    return nullptr;
  }

  return mTextureHost->GetAsSurface();
}

TemporaryRef<gfx::DataSourceSurface>
DeprecatedContentHostBase::GetAsSurface()
{
  return mDeprecatedTextureHost->GetAsSurface();
}
#endif


} 
} 
