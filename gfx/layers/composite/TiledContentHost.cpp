




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
               Compositor* aCompositor)
{
  MOZ_ASSERT(aTexture);
  if (!aTexture) {
    return;
  }

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

void
TiledLayerBufferComposite::MarkTilesForUnlock()
{
  
  
  
  
  for (TileHost& tile : mRetainedTiles) {
    
    
    if (tile.mTextureHost && tile.mSharedLock) {
      mDelayedUnlocks.AppendElement(tile.mSharedLock);
      tile.mSharedLock = nullptr;
    }
  }
}

class TextureSourceRecycler
{
public:
  explicit TextureSourceRecycler(nsTArray<TileHost>&& aTileSet)
    : mTiles(Move(aTileSet))
    , mFirstPossibility(0)
  {}

  
  
  void RecycleTextureSourceForTile(TileHost& aTile) {
    for (size_t i = mFirstPossibility; i < mTiles.Length(); i++) {
      
      
      if (!mTiles[i].mTextureSource) {
        if (i == mFirstPossibility) {
          mFirstPossibility++;
        }
        continue;
      }

      
      
      if (aTile.mTextureHost == mTiles[i].mTextureHost) {
        aTile.mTextureSource = Move(mTiles[i].mTextureSource);
        if (aTile.mTextureHostOnWhite) {
          aTile.mTextureSourceOnWhite = Move(mTiles[i].mTextureSourceOnWhite);
        }
        break;
      }
    }
  }

  
  
  void RecycleTextureSource(TileHost& aTile) {
    for (size_t i = mFirstPossibility; i < mTiles.Length(); i++) {
      if (!mTiles[i].mTextureSource) {
        if (i == mFirstPossibility) {
          mFirstPossibility++;
        }
        continue;
      }

      if (mTiles[i].mTextureSource &&
          mTiles[i].mTextureHost->GetFormat() == aTile.mTextureHost->GetFormat()) {
        aTile.mTextureSource = Move(mTiles[i].mTextureSource);
        if (aTile.mTextureHostOnWhite) {
          aTile.mTextureSourceOnWhite = Move(mTiles[i].mTextureSourceOnWhite);
        }
        break;
      }
    }
  }

protected:
  nsTArray<TileHost> mTiles;
  size_t mFirstPossibility;
};

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

  TilesPlacement newTiles(aTiles.firstTileX(), aTiles.firstTileY(),
                          aTiles.retainedWidth(), aTiles.retainedHeight());

  const InfallibleTArray<TileDescriptor>& tileDescriptors = aTiles.tiles();

  
  
  
  MarkTilesForUnlock();

  TextureSourceRecycler oldRetainedTiles(Move(mRetainedTiles));
  mRetainedTiles.SetLength(tileDescriptors.Length());

  
  
  
  
  
  
  
  for (size_t i = 0; i < tileDescriptors.Length(); i++) {
    const TileDescriptor& tileDesc = tileDescriptors[i];

    TileHost& tile = mRetainedTiles[i];

    if (tileDesc.type() != TileDescriptor::TTexturedTileDescriptor) {
      NS_WARN_IF_FALSE(tileDesc.type() == TileDescriptor::TPlaceholderTileDescriptor,
                       "Unrecognised tile descriptor type");
      continue;
    }

    const TexturedTileDescriptor& texturedDesc = tileDesc.get_TexturedTileDescriptor();

    const TileLock& ipcLock = texturedDesc.sharedLock();
    if (!GetCopyOnWriteLock(ipcLock, tile, aAllocator)) {
      return false;
    }

    tile.mTextureHost = TextureHost::AsTextureHost(texturedDesc.textureParent());

    if (texturedDesc.textureOnWhite().type() == MaybeTexture::TPTextureParent) {
      tile.mTextureHostOnWhite =
        TextureHost::AsTextureHost(texturedDesc.textureOnWhite().get_PTextureParent());
    }

    tile.mTilePosition = newTiles.TilePosition(i);

    
    
    oldRetainedTiles.RecycleTextureSourceForTile(tile);
  }

  
  
  
  
  for (TileHost& tile : mRetainedTiles) {
    if (!tile.mTextureHost || tile.mTextureSource) {
      continue;
    }
    oldRetainedTiles.RecycleTextureSource(tile);
  }

  
  
  for (size_t i = 0; i < mRetainedTiles.Length(); i++) {
    TileHost& tile = mRetainedTiles[i];
    if (!tile.mTextureHost) {
      continue;
    }

    const TileDescriptor& tileDesc = tileDescriptors[i];
    const TexturedTileDescriptor& texturedDesc = tileDesc.get_TexturedTileDescriptor();

    UseTileTexture(tile.mTextureHost,
                   tile.mTextureSource,
                   texturedDesc.updateRect(),
                   aCompositor);

    if (tile.mTextureHostOnWhite) {
      UseTileTexture(tile.mTextureHostOnWhite,
                     tile.mTextureSourceOnWhite,
                     texturedDesc.updateRect(),
                     aCompositor);
    }

    if (tile.mTextureHost->HasInternalBuffer()) {
      
      
      tile.ReadUnlock();
    }
  }

  mTiles = newTiles;
  mValidRegion = aTiles.validRegion();
  mResolution = aTiles.resolution();
  mFrameResolution = CSSToParentLayerScale2D(aTiles.frameXResolution(),
                                             aTiles.frameYResolution());

  return true;
}

void
TiledLayerBufferComposite::ProcessDelayedUnlocks()
{
  for (gfxSharedReadLock* lock : mDelayedUnlocks) {
    lock->ReadUnlock();
  }
  mDelayedUnlocks.Clear();
}

void
TiledLayerBufferComposite::Clear()
{
  for (TileHost& tile : mRetainedTiles) {
    tile.ReadUnlock();
  }
  mRetainedTiles.Clear();
  ProcessDelayedUnlocks();
  mTiles.mFirst = TileIntPoint();
  mTiles.mSize = TileIntSize();
  mValidRegion = nsIntRegion();
  mPaintedRegion = nsIntRegion();
  mResolution = 1.0;
}

void
TiledContentHost::Composite(LayerComposite* aLayer,
                            EffectChain& aEffectChain,
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
  mLowPrecisionTiledBuffer.ProcessDelayedUnlocks();
  mTiledBuffer.ProcessDelayedUnlocks();
}


void
TiledContentHost::RenderTile(TileHost& aTile,
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
  MOZ_ASSERT(!aTile.IsPlaceholderTile());

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

  nsIntRegion compositeRegion = aLayerBuffer.GetValidRegion();
  compositeRegion.AndWith(aVisibleRegion);
  compositeRegion.SubOut(maskRegion);

  IntRect visibleRect = aVisibleRegion.GetBounds();

  if (compositeRegion.IsEmpty()) {
    return;
  }

  if (aBackgroundColor) {
    nsIntRegion backgroundRegion = compositeRegion;
    backgroundRegion.ScaleRoundOut(resolution, resolution);
    EffectChain effect;
    effect.mPrimaryEffect = new EffectSolidColor(ToColor(*aBackgroundColor));
    nsIntRegionRectIterator it(backgroundRegion);
    for (const IntRect* rect = it.Next(); rect != nullptr; rect = it.Next()) {
      Rect graphicsRect(rect->x, rect->y, rect->width, rect->height);
      mCompositor->DrawQuad(graphicsRect, aClipRect, effect, 1.0, aTransform);
    }
  }

  for (size_t i = 0; i < aLayerBuffer.GetTileCount(); ++i) {
    TileHost& tile = aLayerBuffer.GetTile(i);
    if (tile.IsPlaceholderTile()) {
      continue;
    }

    TileIntPoint tilePosition = aLayerBuffer.GetPlacement().TilePosition(i);
    
    MOZ_ASSERT(tilePosition.x == tile.mTilePosition.x && tilePosition.y == tile.mTilePosition.y);

    IntPoint tileOffset = aLayerBuffer.GetTileOffset(tilePosition);
    nsIntRegion tileDrawRegion = IntRect(tileOffset, aLayerBuffer.GetScaledTileSize());
    tileDrawRegion.AndWith(compositeRegion);

    if (tileDrawRegion.IsEmpty()) {
      continue;
    }

    tileDrawRegion.ScaleRoundOut(resolution, resolution);
    RenderTile(tile, aEffectChain, aOpacity,
               aTransform, aFilter, aClipRect, tileDrawRegion,
               tileOffset * resolution, aLayerBuffer.GetTileSize(),
               gfx::Rect(visibleRect.x, visibleRect.y,
                         visibleRect.width, visibleRect.height));
    if (tile.mTextureHostOnWhite) {
      componentAlphaDiagnostic = DiagnosticFlags::COMPONENT_ALPHA;
    }
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
