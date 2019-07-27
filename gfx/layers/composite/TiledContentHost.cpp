




#include "TiledContentHost.h"
#include "PaintedLayerComposite.h"      
#include "mozilla/gfx/BaseSize.h"       
#include "mozilla/gfx/Matrix.h"         
#include "mozilla/gfx/Point.h"          
#include "mozilla/layers/Compositor.h"  
#include "mozilla/layers/Effects.h"     
#include "mozilla/layers/LayerMetricsWrapper.h" 
#include "mozilla/layers/TextureHostOGL.h"  
#include "nsAString.h"
#include "nsDebug.h"                    
#include "nsPoint.h"                    
#include "nsPrintfCString.h"            
#include "nsRect.h"                     
#include "mozilla/layers/TiledContentClient.h"

class gfxReusableSurfaceWrapper;

namespace mozilla {
using namespace gfx;
namespace layers {

class Layer;

TiledLayerBufferComposite::TiledLayerBufferComposite()
  : mFrameResolution()
{}

TiledLayerBufferComposite::~TiledLayerBufferComposite()
{
  Clear();
}

 void
TiledLayerBufferComposite::RecycleCallback(TextureHost* textureHost, void* aClosure)
{
  textureHost->CompositorRecycle();
}

void
TiledLayerBufferComposite::SetCompositor(Compositor* aCompositor)
{
  MOZ_ASSERT(aCompositor);
  for (TileHost& tile : mRetainedTiles) {
    if (tile.IsPlaceholderTile()) continue;
    tile.mTextureHost->SetCompositor(aCompositor);
    if (tile.mTextureHostOnWhite) {
      tile.mTextureHostOnWhite->SetCompositor(aCompositor);
    }
  }
}

TiledContentHost::TiledContentHost(const TextureInfo& aTextureInfo)
  : ContentHost(aTextureInfo)
  , mTiledBuffer(TiledLayerBufferComposite())
  , mLowPrecisionTiledBuffer(TiledLayerBufferComposite())
{
  MOZ_COUNT_CTOR(TiledContentHost);
}

TiledContentHost::~TiledContentHost()
{
  MOZ_COUNT_DTOR(TiledContentHost);
}

void
TiledContentHost::Attach(Layer* aLayer,
                         Compositor* aCompositor,
                         AttachFlags aFlags )
{
  CompositableHost::Attach(aLayer, aCompositor, aFlags);
}

void
TiledContentHost::Detach(Layer* aLayer,
                         AttachFlags aFlags )
{
  if (!mKeepAttached || aLayer == mLayer || aFlags & FORCE_DETACH) {
    
    
    mTiledBuffer.Clear();
    mLowPrecisionTiledBuffer.Clear();
  }
  CompositableHost::Detach(aLayer,aFlags);
}

bool
TiledContentHost::UseTiledLayerBuffer(ISurfaceAllocator* aAllocator,
                                      const SurfaceDescriptorTiles& aTiledDescriptor)
{
  if (aTiledDescriptor.resolution() < 1) {
    if (!mLowPrecisionTiledBuffer.UseTiles(aTiledDescriptor, mCompositor, aAllocator)) {
      return false;
    }
  } else {
    if (!mTiledBuffer.UseTiles(aTiledDescriptor, mCompositor, aAllocator)) {
      return false;
    }
  }
  return true;
}

void
UseTileTexture(CompositableTextureHostRef& aTexture,
               CompositableTextureSourceRef& aTextureSource,
               const IntRect& aUpdateRect,
               TextureHost* aNewTexture,
               Compositor* aCompositor)
{
  MOZ_ASSERT(aNewTexture);
  if (!aNewTexture) {
    return;
  }

  if (aTexture) {
    aTexture->SetCompositor(aCompositor);
    aNewTexture->SetCompositor(aCompositor);

    if (aTexture->GetFormat() != aNewTexture->GetFormat()) {
      
      aTextureSource = nullptr;
      aTexture = nullptr;
    }
  }

  aTexture = aNewTexture;


  if (aCompositor) {
    aTexture->SetCompositor(aCompositor);
  }

  if (!aUpdateRect.IsEmpty()) {
#ifdef MOZ_GFX_OPTIMIZE_MOBILE
    aTexture->Updated(nullptr);
#else
    
    
    
    
    nsIntRegion region = aUpdateRect;
    aTexture->Updated(&region);
#endif
  }

  aTexture->PrepareTextureSource(aTextureSource);
}

bool
GetCopyOnWriteLock(const TileLock& ipcLock, TileHost& aTile, ISurfaceAllocator* aAllocator) {
  MOZ_ASSERT(aAllocator);

  nsRefPtr<gfxSharedReadLock> sharedLock;
  if (ipcLock.type() == TileLock::TShmemSection) {
    sharedLock = gfxShmSharedReadLock::Open(aAllocator, ipcLock.get_ShmemSection());
  } else {
    if (!aAllocator->IsSameProcess()) {
      
      
      NS_ERROR("A client process may be trying to peek at the host's address space!");
      return false;
    }
    sharedLock = reinterpret_cast<gfxMemorySharedReadLock*>(ipcLock.get_uintptr_t());
    if (sharedLock) {
      
      sharedLock.get()->Release();
    }
  }
  aTile.mSharedLock = sharedLock;
  return true;
}

bool
TiledLayerBufferComposite::UseTiles(const SurfaceDescriptorTiles& aTiles,
                                    Compositor* aCompositor,
                                    ISurfaceAllocator* aAllocator)
{
  if (mResolution != aTiles.resolution()) {
    Clear();
  }
  MOZ_ASSERT(aAllocator);
  MOZ_ASSERT(aCompositor);
  if (!aAllocator || !aCompositor) {
    return false;
  }

  if (aTiles.resolution() == 0 || IsNaN(aTiles.resolution())) {
    
    
    return false;
  }

  int newFirstTileX = aTiles.firstTileX();
  int newFirstTileY = aTiles.firstTileY();
  int oldFirstTileX = mFirstTileX;
  int oldFirstTileY = mFirstTileY;
  int newRetainedWidth = aTiles.retainedWidth();
  int newRetainedHeight = aTiles.retainedHeight();
  int oldRetainedWidth = mRetainedWidth;
  int oldRetainedHeight = mRetainedHeight;

  const InfallibleTArray<TileDescriptor>& tileDescriptors = aTiles.tiles();

  nsTArray<TileHost> oldTiles;
  mRetainedTiles.SwapElements(oldTiles);
  mRetainedTiles.SetLength(tileDescriptors.Length());

  
  
  
  
  for (size_t i = 0; i < oldTiles.Length(); ++i) {
    TileHost& tile = oldTiles[i];
    
    
    
    
    
    tile.ReadUnlockPrevious();

    if (tile.mTextureHost && !tile.mTextureHost->HasInternalBuffer()) {
      MOZ_ASSERT(tile.mSharedLock);
      int tileX = i / oldRetainedHeight + oldFirstTileX;
      int tileY = i % oldRetainedHeight + oldFirstTileY;

      if (tileX >= newFirstTileX && tileY >= newFirstTileY &&
          tileX < (newFirstTileX + newRetainedWidth) &&
          tileY < (newFirstTileY + newRetainedHeight)) {
        
        tile.mPreviousSharedLock = tile.mSharedLock;
        tile.mSharedLock = nullptr;
      } else {
        
        
        tile.ReadUnlock();
      }
    }

    
    MOZ_ASSERT(!tile.mSharedLock);
  }

  
  
  
  for (size_t i = 0; i < tileDescriptors.Length(); i++) {
    int tileX = i / newRetainedHeight + newFirstTileX;
    int tileY = i % newRetainedHeight + newFirstTileY;

    
    
    if (tileX < oldFirstTileX || tileY < oldFirstTileY ||
        tileX >= (oldFirstTileX + oldRetainedWidth) ||
        tileY >= (oldFirstTileY + oldRetainedHeight)) {
      mRetainedTiles[i] = GetPlaceholderTile();
    } else {
      mRetainedTiles[i] = oldTiles[(tileX - oldFirstTileX) * oldRetainedHeight +
                                   (tileY - oldFirstTileY)];
      
      
      
      MOZ_ASSERT(tileX == mRetainedTiles[i].x && tileY == mRetainedTiles[i].y);
    }
  }

  
  
  
  oldTiles.Clear();

  
  for (size_t i = 0; i < mRetainedTiles.Length(); i++) {
    const TileDescriptor& tileDesc = tileDescriptors[i];

    TileHost& tile = mRetainedTiles[i];

    switch (tileDesc.type()) {
      case TileDescriptor::TTexturedTileDescriptor: {
        const TexturedTileDescriptor& texturedDesc = tileDesc.get_TexturedTileDescriptor();

        const TileLock& ipcLock = texturedDesc.sharedLock();
        if (!GetCopyOnWriteLock(ipcLock, tile, aAllocator)) {
          return false;
        }

        RefPtr<TextureHost> textureHost = TextureHost::AsTextureHost(
          texturedDesc.textureParent()
        );

        RefPtr<TextureHost> textureOnWhite = nullptr;
        if (texturedDesc.textureOnWhite().type() == MaybeTexture::TPTextureParent) {
          textureOnWhite = TextureHost::AsTextureHost(
            texturedDesc.textureOnWhite().get_PTextureParent()
          );
        }

        UseTileTexture(tile.mTextureHost,
                       tile.mTextureSource,
                       texturedDesc.updateRect(),
                       textureHost,
                       aCompositor);

        if (textureOnWhite) {
          UseTileTexture(tile.mTextureHostOnWhite,
                         tile.mTextureSourceOnWhite,
                         texturedDesc.updateRect(),
                         textureOnWhite,
                         aCompositor);
        } else {
          
          tile.mTextureSourceOnWhite = nullptr;
          tile.mTextureHostOnWhite = nullptr;
        }

        if (textureHost->HasInternalBuffer()) {
          
          
          tile.ReadUnlock();
        }

        break;
      }
      default:
        NS_WARNING("Unrecognised tile descriptor type");
      case TileDescriptor::TPlaceholderTileDescriptor: {

        if (tile.mTextureHost) {
          tile.mTextureHost->UnbindTextureSource();
          tile.mTextureSource = nullptr;
        }
        if (tile.mTextureHostOnWhite) {
          tile.mTextureHostOnWhite->UnbindTextureSource();
          tile.mTextureSourceOnWhite = nullptr;
        }
        
        
        tile.ReadUnlockPrevious();
        tile = GetPlaceholderTile();

        break;
      }
    }

    tile.x = i / newRetainedHeight + newFirstTileX;
    tile.y = i % newRetainedHeight + newFirstTileY;
  }

  mFirstTileX = newFirstTileX;
  mFirstTileY = newFirstTileY;
  mRetainedWidth = newRetainedWidth;
  mRetainedHeight = newRetainedHeight;
  mValidRegion = aTiles.validRegion();

  mResolution = aTiles.resolution();
  mFrameResolution = CSSToParentLayerScale2D(aTiles.frameXResolution(),
                                             aTiles.frameYResolution());

  return true;
}

void
TiledLayerBufferComposite::Clear()
{
  for (TileHost& tile : mRetainedTiles) {
    tile.ReadUnlock();
    tile.ReadUnlockPrevious();
  }
  mRetainedTiles.Clear();
  mFirstTileX = 0;
  mFirstTileY = 0;
  mRetainedWidth = 0;
  mRetainedHeight = 0;
  mValidRegion = nsIntRegion();
  mPaintedRegion = nsIntRegion();
  mResolution = 1.0;
}

void
TiledContentHost::Composite(EffectChain& aEffectChain,
                            float aOpacity,
                            const gfx::Matrix4x4& aTransform,
                            const gfx::Filter& aFilter,
                            const gfx::Rect& aClipRect,
                            const nsIntRegion* aVisibleRegion )
{
  MOZ_ASSERT(mCompositor);
  
  
  
  
  
  
  
  
  
  
  
  gfxRGBA backgroundColor(0);
  if (aOpacity == 1.0f && gfxPrefs::LowPrecisionOpacity() < 1.0f) {
    
    
    for (LayerMetricsWrapper ancestor(GetLayer(), LayerMetricsWrapper::StartAt::BOTTOM); ancestor; ancestor = ancestor.GetParent()) {
      if (ancestor.Metrics().IsScrollable()) {
        backgroundColor = ancestor.Metrics().GetBackgroundColor();
        break;
      }
    }
  }
  float lowPrecisionOpacityReduction =
        (aOpacity == 1.0f && backgroundColor.a == 1.0f)
        ? gfxPrefs::LowPrecisionOpacity() : 1.0f;

  nsIntRegion tmpRegion;
  const nsIntRegion* renderRegion = aVisibleRegion;
#ifndef MOZ_IGNORE_PAINT_WILL_RESAMPLE
  if (PaintWillResample()) {
    
    
    
    tmpRegion = aVisibleRegion->GetBounds();
    renderRegion = &tmpRegion;
  }
#endif

  
  RenderLayerBuffer(mLowPrecisionTiledBuffer,
                    lowPrecisionOpacityReduction < 1.0f ? &backgroundColor : nullptr,
                    aEffectChain, lowPrecisionOpacityReduction * aOpacity,
                    aFilter, aClipRect, *renderRegion, aTransform);
  RenderLayerBuffer(mTiledBuffer, nullptr, aEffectChain, aOpacity, aFilter,
                    aClipRect, *renderRegion, aTransform);
}


void
TiledContentHost::RenderTile(TileHost& aTile,
                             const gfxRGBA* aBackgroundColor,
                             EffectChain& aEffectChain,
                             float aOpacity,
                             const gfx::Matrix4x4& aTransform,
                             const gfx::Filter& aFilter,
                             const gfx::Rect& aClipRect,
                             const nsIntRegion& aScreenRegion,
                             const IntPoint& aTextureOffset,
                             const IntSize& aTextureBounds,
                             const gfx::Rect& aVisibleRect)
{
  if (aTile.IsPlaceholderTile()) {
    
    
    return;
  }

  if (aBackgroundColor) {
    aEffectChain.mPrimaryEffect = new EffectSolidColor(ToColor(*aBackgroundColor));
    nsIntRegionRectIterator it(aScreenRegion);
    for (const IntRect* rect = it.Next(); rect != nullptr; rect = it.Next()) {
      Rect graphicsRect(rect->x, rect->y, rect->width, rect->height);
      mCompositor->DrawQuad(graphicsRect, aClipRect, aEffectChain, 1.0, aTransform);
    }
  }

  AutoLockTextureHost autoLock(aTile.mTextureHost);
  AutoLockTextureHost autoLockOnWhite(aTile.mTextureHostOnWhite);
  if (autoLock.Failed() ||
      autoLockOnWhite.Failed()) {
    NS_WARNING("Failed to lock tile");
    return;
  }

  if (!aTile.mTextureHost->BindTextureSource(aTile.mTextureSource)) {
    return;
  }

  if (aTile.mTextureHostOnWhite && !aTile.mTextureHostOnWhite->BindTextureSource(aTile.mTextureSourceOnWhite)) {
    return;
  }

  RefPtr<TexturedEffect> effect =
    CreateTexturedEffect(aTile.mTextureSource,
                         aTile.mTextureSourceOnWhite,
                         aFilter,
                         true,
                         aTile.mTextureHost->GetRenderState());
  if (!effect) {
    return;
  }

  aEffectChain.mPrimaryEffect = effect;

  nsIntRegionRectIterator it(aScreenRegion);
  for (const IntRect* rect = it.Next(); rect != nullptr; rect = it.Next()) {
    Rect graphicsRect(rect->x, rect->y, rect->width, rect->height);
    Rect textureRect(rect->x - aTextureOffset.x, rect->y - aTextureOffset.y,
                     rect->width, rect->height);

    effect->mTextureCoords = Rect(textureRect.x / aTextureBounds.width,
                                  textureRect.y / aTextureBounds.height,
                                  textureRect.width / aTextureBounds.width,
                                  textureRect.height / aTextureBounds.height);
    mCompositor->DrawQuad(graphicsRect, aClipRect, aEffectChain, aOpacity, aTransform, aVisibleRect);
  }
  DiagnosticFlags flags = DiagnosticFlags::CONTENT | DiagnosticFlags::TILE;
  if (aTile.mTextureHostOnWhite) {
    flags |= DiagnosticFlags::COMPONENT_ALPHA;
  }
  mCompositor->DrawDiagnostics(flags,
                               aScreenRegion, aClipRect, aTransform, mFlashCounter);
  aTile.ReadUnlockPrevious();
}

void
TiledContentHost::RenderLayerBuffer(TiledLayerBufferComposite& aLayerBuffer,
                                    const gfxRGBA* aBackgroundColor,
                                    EffectChain& aEffectChain,
                                    float aOpacity,
                                    const gfx::Filter& aFilter,
                                    const gfx::Rect& aClipRect,
                                    nsIntRegion aVisibleRegion,
                                    gfx::Matrix4x4 aTransform)
{
  if (!mCompositor) {
    NS_WARNING("Can't render tiled content host - no compositor");
    return;
  }
  float resolution = aLayerBuffer.GetResolution();
  gfx::Size layerScale(1, 1);

  
  
  
  if (aLayerBuffer.GetFrameResolution() != mTiledBuffer.GetFrameResolution()) {
    const CSSToParentLayerScale2D& layerResolution = aLayerBuffer.GetFrameResolution();
    const CSSToParentLayerScale2D& localResolution = mTiledBuffer.GetFrameResolution();
    layerScale.width = layerResolution.xScale / localResolution.xScale;
    layerScale.height = layerResolution.yScale / localResolution.yScale;
    aVisibleRegion.ScaleRoundOut(layerScale.width, layerScale.height);
  }

  
  
  
  nsIntRegion maskRegion;
  if (resolution != mTiledBuffer.GetResolution()) {
    maskRegion = mTiledBuffer.GetValidRegion();
    
    
    maskRegion.ScaleRoundOut(layerScale.width, layerScale.height);
  }

  
  
  aTransform.PreScale(1/(resolution * layerScale.width),
                      1/(resolution * layerScale.height), 1);

  DiagnosticFlags componentAlphaDiagnostic = DiagnosticFlags::NO_DIAGNOSTIC;

  uint32_t rowCount = 0;
  uint32_t tileX = 0;
  IntRect visibleRect = aVisibleRegion.GetBounds();
  gfx::IntSize scaledTileSize = aLayerBuffer.GetScaledTileSize();
  for (int32_t x = visibleRect.x; x < visibleRect.x + visibleRect.width;) {
    rowCount++;
    int32_t tileStartX = aLayerBuffer.GetTileStart(x, scaledTileSize.width);
    int32_t w = scaledTileSize.width - tileStartX;
    if (x + w > visibleRect.x + visibleRect.width) {
      w = visibleRect.x + visibleRect.width - x;
    }
    int tileY = 0;
    for (int32_t y = visibleRect.y; y < visibleRect.y + visibleRect.height;) {
      int32_t tileStartY = aLayerBuffer.GetTileStart(y, scaledTileSize.height);
      int32_t h = scaledTileSize.height - tileStartY;
      if (y + h > visibleRect.y + visibleRect.height) {
        h = visibleRect.y + visibleRect.height - y;
      }

      nsIntPoint tileOrigin = nsIntPoint(aLayerBuffer.RoundDownToTileEdge(x, scaledTileSize.width),
                                         aLayerBuffer.RoundDownToTileEdge(y, scaledTileSize.height));
      TileHost& tileTexture = aLayerBuffer.GetTile(tileOrigin);
      if (!tileTexture.IsPlaceholderTile()) {
        nsIntRegion tileDrawRegion;
        tileDrawRegion.And(IntRect(x, y, w, h), aLayerBuffer.GetValidRegion());
        tileDrawRegion.And(tileDrawRegion, aVisibleRegion);
        tileDrawRegion.Sub(tileDrawRegion, maskRegion);

        if (!tileDrawRegion.IsEmpty()) {
          tileDrawRegion.ScaleRoundOut(resolution, resolution);
          IntPoint tileOffset((x - tileStartX) * resolution,
                                (y - tileStartY) * resolution);
          gfx::IntSize tileSize = aLayerBuffer.GetTileSize();
          RenderTile(tileTexture, aBackgroundColor, aEffectChain, aOpacity, aTransform,
                     aFilter, aClipRect, tileDrawRegion, tileOffset,
                     IntSize(tileSize.width, tileSize.height),
                     gfx::Rect(visibleRect.x, visibleRect.y,
                               visibleRect.width, visibleRect.height));
          if (tileTexture.mTextureHostOnWhite) {
            componentAlphaDiagnostic = DiagnosticFlags::COMPONENT_ALPHA;
          }
        }
      }
      tileY++;
      y += h;
    }
    tileX++;
    x += w;
  }
  gfx::Rect rect(visibleRect.x, visibleRect.y,
                 visibleRect.width, visibleRect.height);
  GetCompositor()->DrawDiagnostics(DiagnosticFlags::CONTENT | componentAlphaDiagnostic,
                                   rect, aClipRect, aTransform, mFlashCounter);
}

void
TiledContentHost::PrintInfo(std::stringstream& aStream, const char* aPrefix)
{
  aStream << aPrefix;
  aStream << nsPrintfCString("TiledContentHost (0x%p)", this).get();

  if (gfxPrefs::LayersDumpTexture() || profiler_feature_active("layersdump")) {
    nsAutoCString pfx(aPrefix);
    pfx += "  ";

    Dump(aStream, pfx.get(), false);
  }
}

void
TiledContentHost::Dump(std::stringstream& aStream,
                       const char* aPrefix,
                       bool aDumpHtml)
{
  mTiledBuffer.Dump(aStream, aPrefix, aDumpHtml);
}

} 
} 
