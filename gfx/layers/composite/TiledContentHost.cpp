




#include "TiledContentHost.h"
#include "ThebesLayerComposite.h"       
#include "mozilla/gfx/BaseSize.h"       
#include "mozilla/gfx/Matrix.h"         
#include "mozilla/layers/Compositor.h"  
#include "mozilla/layers/Effects.h"     
#include "mozilla/layers/TextureHostOGL.h"  
#include "nsAString.h"
#include "nsDebug.h"                    
#include "nsPoint.h"                    
#include "nsPrintfCString.h"            
#include "nsRect.h"                     
#include "nsSize.h"                     
#include "mozilla/layers/TiledContentClient.h"

class gfxReusableSurfaceWrapper;

namespace mozilla {
using namespace gfx;
namespace layers {

class Layer;

TiledLayerBufferComposite::TiledLayerBufferComposite()
  : mFrameResolution(1.0)
  , mHasDoubleBufferedTiles(false)
  , mUninitialized(true)
{}

 void
TiledLayerBufferComposite::RecycleCallback(TextureHost* textureHost, void* aClosure)
{
  textureHost->CompositorRecycle();
}

TiledLayerBufferComposite::TiledLayerBufferComposite(ISurfaceAllocator* aAllocator,
                                                     const SurfaceDescriptorTiles& aDescriptor,
                                                     const nsIntRegion& aOldPaintedRegion)
{
  mUninitialized = false;
  mHasDoubleBufferedTiles = false;
  mValidRegion = aDescriptor.validRegion();
  mPaintedRegion = aDescriptor.paintedRegion();
  mRetainedWidth = aDescriptor.retainedWidth();
  mRetainedHeight = aDescriptor.retainedHeight();
  mResolution = aDescriptor.resolution();
  mFrameResolution = CSSToParentLayerScale(aDescriptor.frameResolution());

  
  nsIntRegion oldPaintedRegion(aOldPaintedRegion);
  oldPaintedRegion.And(oldPaintedRegion, mValidRegion);
  mPaintedRegion.Or(mPaintedRegion, oldPaintedRegion);

  const InfallibleTArray<TileDescriptor>& tiles = aDescriptor.tiles();
  for(size_t i = 0; i < tiles.Length(); i++) {
    RefPtr<TextureHost> texture;
    const TileDescriptor& tileDesc = tiles[i];
    switch (tileDesc.type()) {
      case TileDescriptor::TTexturedTileDescriptor : {
        texture = TextureHost::AsTextureHost(tileDesc.get_TexturedTileDescriptor().textureParent());
#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 17
        if (!gfxPrefs::LayersUseSimpleTiles()) {
          texture->SetRecycleCallback(RecycleCallback, nullptr);
        }
#endif
        const TileLock& ipcLock = tileDesc.get_TexturedTileDescriptor().sharedLock();
        nsRefPtr<gfxSharedReadLock> sharedLock;
        if (ipcLock.type() == TileLock::TShmemSection) {
          sharedLock = gfxShmSharedReadLock::Open(aAllocator, ipcLock.get_ShmemSection());
        } else {
          sharedLock = reinterpret_cast<gfxMemorySharedReadLock*>(ipcLock.get_uintptr_t());
          if (sharedLock) {
            
            sharedLock->Release();
          }
        }

        mRetainedTiles.AppendElement(TileHost(sharedLock, texture));
        break;
      }
      default:
        NS_WARNING("Unrecognised tile descriptor type");
        
      case TileDescriptor::TPlaceholderTileDescriptor :
        mRetainedTiles.AppendElement(GetPlaceholderTile());
        break;
    }
    if (texture && !texture->HasInternalBuffer()) {
      mHasDoubleBufferedTiles = true;
    }
  }
}

void
TiledLayerBufferComposite::ReadUnlock()
{
  if (!IsValid()) {
    return;
  }
  for (size_t i = 0; i < mRetainedTiles.Length(); i++) {
    mRetainedTiles[i].ReadUnlock();
  }
}

void
TiledLayerBufferComposite::ReleaseTextureHosts()
{
  if (!IsValid()) {
    return;
  }
  for (size_t i = 0; i < mRetainedTiles.Length(); i++) {
    mRetainedTiles[i].mTextureHost = nullptr;
  }
}

void
TiledLayerBufferComposite::Upload()
{
  if(!IsValid()) {
    return;
  }
  
  
  Update(mValidRegion, mPaintedRegion);
  ClearPaintedRegion();
}

TileHost
TiledLayerBufferComposite::ValidateTile(TileHost aTile,
                                        const nsIntPoint& aTileOrigin,
                                        const nsIntRegion& aDirtyRect)
{
  if (aTile.IsPlaceholderTile()) {
    NS_WARNING("Placeholder tile encountered in painted region");
    return aTile;
  }

#ifdef GFX_TILEDLAYER_PREF_WARNINGS
  printf_stderr("Upload tile %i, %i\n", aTileOrigin.x, aTileOrigin.y);
  long start = PR_IntervalNow();
#endif

  MOZ_ASSERT(aTile.mTextureHost->GetFlags() & TextureFlags::IMMEDIATE_UPLOAD);
  
  
  
  
  aTile.mTextureHost->Updated(nullptr);

#ifdef GFX_TILEDLAYER_PREF_WARNINGS
  if (PR_IntervalNow() - start > 1) {
    printf_stderr("Tile Time to upload %i\n", PR_IntervalNow() - start);
  }
#endif
  return aTile;
}

void
TiledLayerBufferComposite::SetCompositor(Compositor* aCompositor)
{
  if (!IsValid()) {
    return;
  }
  for (size_t i = 0; i < mRetainedTiles.Length(); i++) {
    if (mRetainedTiles[i].IsPlaceholderTile()) continue;
    mRetainedTiles[i].mTextureHost->SetCompositor(aCompositor);
  }
}

#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 17
void
TiledLayerBufferComposite::SetReleaseFence(const android::sp<android::Fence>& aReleaseFence)
{
  for (size_t i = 0; i < mRetainedTiles.Length(); i++) {
    if (!mRetainedTiles[i].mTextureHost) {
      continue;
    }
    TextureHostOGL* texture = mRetainedTiles[i].mTextureHost->AsHostOGL();
    if (!texture) {
      continue;
    }
    texture->SetReleaseFence(new android::Fence(aReleaseFence->dup()));
  }
}
#endif

TiledContentHost::TiledContentHost(const TextureInfo& aTextureInfo)
  : ContentHost(aTextureInfo)
  , mTiledBuffer(TiledLayerBufferComposite())
  , mLowPrecisionTiledBuffer(TiledLayerBufferComposite())
  , mOldTiledBuffer(TiledLayerBufferComposite())
  , mOldLowPrecisionTiledBuffer(TiledLayerBufferComposite())
  , mPendingUpload(false)
  , mPendingLowPrecisionUpload(false)
{
  MOZ_COUNT_CTOR(TiledContentHost);
}

TiledContentHost::~TiledContentHost()
{
  MOZ_COUNT_DTOR(TiledContentHost);

  
  
  
  
  if (mPendingUpload) {
    mTiledBuffer.ReadUnlock();
    if (mOldTiledBuffer.HasDoubleBufferedTiles()) {
      mOldTiledBuffer.ReadUnlock();
    }
  } else if (mTiledBuffer.HasDoubleBufferedTiles()) {
    mTiledBuffer.ReadUnlock();
  }

  if (mPendingLowPrecisionUpload) {
    mLowPrecisionTiledBuffer.ReadUnlock();
    if (mOldLowPrecisionTiledBuffer.HasDoubleBufferedTiles()) {
      mOldLowPrecisionTiledBuffer.ReadUnlock();
    }
  } else if (mLowPrecisionTiledBuffer.HasDoubleBufferedTiles()) {
    mLowPrecisionTiledBuffer.ReadUnlock();
  }
}

void
TiledContentHost::Attach(Layer* aLayer,
                         Compositor* aCompositor,
                         AttachFlags aFlags )
{
  CompositableHost::Attach(aLayer, aCompositor, aFlags);
  static_cast<ThebesLayerComposite*>(aLayer)->EnsureTiled();
}

void
TiledContentHost::UseTiledLayerBuffer(ISurfaceAllocator* aAllocator,
                                      const SurfaceDescriptorTiles& aTiledDescriptor)
{
  if (aTiledDescriptor.resolution() < 1) {
    if (mPendingLowPrecisionUpload) {
      mLowPrecisionTiledBuffer.ReadUnlock();
    } else {
      mPendingLowPrecisionUpload = true;
      
      
      
      
      
      
      if (mLowPrecisionTiledBuffer.HasDoubleBufferedTiles()) {
        mOldLowPrecisionTiledBuffer = mLowPrecisionTiledBuffer;
        mOldLowPrecisionTiledBuffer.ReleaseTextureHosts();
      }
    }
    mLowPrecisionTiledBuffer =
      TiledLayerBufferComposite(aAllocator, aTiledDescriptor,
                                mLowPrecisionTiledBuffer.GetPaintedRegion());
  } else {
    if (mPendingUpload) {
      mTiledBuffer.ReadUnlock();
    } else {
      mPendingUpload = true;
      if (mTiledBuffer.HasDoubleBufferedTiles()) {
        mOldTiledBuffer = mTiledBuffer;
        mOldTiledBuffer.ReleaseTextureHosts();
      }
    }
    mTiledBuffer = TiledLayerBufferComposite(aAllocator, aTiledDescriptor,
                                             mTiledBuffer.GetPaintedRegion());
  }
}

void
TiledContentHost::Composite(EffectChain& aEffectChain,
                            float aOpacity,
                            const gfx::Matrix4x4& aTransform,
                            const gfx::Filter& aFilter,
                            const gfx::Rect& aClipRect,
                            const nsIntRegion* aVisibleRegion ,
                            TiledLayerProperties* aLayerProperties )
{
  MOZ_ASSERT(aLayerProperties, "aLayerProperties required for TiledContentHost");

  if (mPendingUpload) {
    mTiledBuffer.SetCompositor(mCompositor);
    mTiledBuffer.Upload();

    
    
    if (!mTiledBuffer.HasDoubleBufferedTiles()) {
      mTiledBuffer.ReadUnlock();
    }
  }
  if (mPendingLowPrecisionUpload) {
    mLowPrecisionTiledBuffer.SetCompositor(mCompositor);
    mLowPrecisionTiledBuffer.Upload();

    if (!mLowPrecisionTiledBuffer.HasDoubleBufferedTiles()) {
      mLowPrecisionTiledBuffer.ReadUnlock();
    }
  }

  RenderLayerBuffer(mLowPrecisionTiledBuffer, aEffectChain, aOpacity, aFilter,
                    aClipRect, aLayerProperties->mVisibleRegion, aTransform);
  RenderLayerBuffer(mTiledBuffer, aEffectChain, aOpacity, aFilter,
                    aClipRect, aLayerProperties->mVisibleRegion, aTransform);

  
  
  
  if (mPendingUpload && mOldTiledBuffer.HasDoubleBufferedTiles()) {
    mOldTiledBuffer.ReadUnlock();
    mOldTiledBuffer = TiledLayerBufferComposite();
  }
  if (mPendingLowPrecisionUpload && mOldLowPrecisionTiledBuffer.HasDoubleBufferedTiles()) {
    mOldLowPrecisionTiledBuffer.ReadUnlock();
    mOldLowPrecisionTiledBuffer = TiledLayerBufferComposite();
  }
  mPendingUpload = mPendingLowPrecisionUpload = false;
}


void
TiledContentHost::RenderTile(const TileHost& aTile,
                             EffectChain& aEffectChain,
                             float aOpacity,
                             const gfx::Matrix4x4& aTransform,
                             const gfx::Filter& aFilter,
                             const gfx::Rect& aClipRect,
                             const nsIntRegion& aScreenRegion,
                             const nsIntPoint& aTextureOffset,
                             const nsIntSize& aTextureBounds)
{
  if (aTile.IsPlaceholderTile()) {
    
    
    return;
  }

  nsIntRect screenBounds = aScreenRegion.GetBounds();
  Rect quad(screenBounds.x, screenBounds.y, screenBounds.width, screenBounds.height);
  quad = aTransform.TransformBounds(quad);

  if (!quad.Intersects(mCompositor->ClipRectInLayersCoordinates(aClipRect))) {
    return;
  }

  AutoLockTextureHost autoLock(aTile.mTextureHost);
  if (autoLock.Failed()) {
    NS_WARNING("Failed to lock tile");
    return;
  }
  RefPtr<NewTextureSource> source = aTile.mTextureHost->GetTextureSources();
  if (!source) {
    return;
  }

  RefPtr<TexturedEffect> effect =
    CreateTexturedEffect(aTile.mTextureHost->GetFormat(), source, aFilter);
  if (!effect) {
    return;
  }

  aEffectChain.mPrimaryEffect = effect;

  nsIntRegionRectIterator it(aScreenRegion);
  for (const nsIntRect* rect = it.Next(); rect != nullptr; rect = it.Next()) {
    Rect graphicsRect(rect->x, rect->y, rect->width, rect->height);
    Rect textureRect(rect->x - aTextureOffset.x, rect->y - aTextureOffset.y,
                     rect->width, rect->height);

    effect->mTextureCoords = Rect(textureRect.x / aTextureBounds.width,
                                  textureRect.y / aTextureBounds.height,
                                  textureRect.width / aTextureBounds.width,
                                  textureRect.height / aTextureBounds.height);
    mCompositor->DrawQuad(graphicsRect, aClipRect, aEffectChain, aOpacity, aTransform);
  }
  mCompositor->DrawDiagnostics(DiagnosticFlags::CONTENT | DiagnosticFlags::TILE,
                               aScreenRegion, aClipRect, aTransform, mFlashCounter);
}

void
TiledContentHost::RenderLayerBuffer(TiledLayerBufferComposite& aLayerBuffer,
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
    const CSSToParentLayerScale& layerResolution = aLayerBuffer.GetFrameResolution();
    const CSSToParentLayerScale& localResolution = mTiledBuffer.GetFrameResolution();
    layerScale.width = layerScale.height = layerResolution.scale / localResolution.scale;
    aVisibleRegion.ScaleRoundOut(layerScale.width, layerScale.height);
  }

  
  
  
  nsIntRegion maskRegion;
  if (resolution != mTiledBuffer.GetResolution()) {
    maskRegion = mTiledBuffer.GetValidRegion();
    
    
    maskRegion.ScaleRoundOut(layerScale.width, layerScale.height);
  }

  
  
  aTransform.Scale(1/(resolution * layerScale.width),
                   1/(resolution * layerScale.height), 1);

  uint32_t rowCount = 0;
  uint32_t tileX = 0;
  nsIntRect visibleRect = aVisibleRegion.GetBounds();
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

      TileHost tileTexture = aLayerBuffer.
        GetTile(nsIntPoint(aLayerBuffer.RoundDownToTileEdge(x, scaledTileSize.width),
                           aLayerBuffer.RoundDownToTileEdge(y, scaledTileSize.height)));
      if (tileTexture != aLayerBuffer.GetPlaceholderTile()) {
        nsIntRegion tileDrawRegion;
        tileDrawRegion.And(nsIntRect(x, y, w, h), aLayerBuffer.GetValidRegion());
        tileDrawRegion.And(tileDrawRegion, aVisibleRegion);
        tileDrawRegion.Sub(tileDrawRegion, maskRegion);

        if (!tileDrawRegion.IsEmpty()) {
          tileDrawRegion.ScaleRoundOut(resolution, resolution);
          nsIntPoint tileOffset((x - tileStartX) * resolution,
                                (y - tileStartY) * resolution);
          gfx::IntSize tileSize = aLayerBuffer.GetTileSize();
          RenderTile(tileTexture, aEffectChain, aOpacity, aTransform, aFilter, aClipRect, tileDrawRegion,
                     tileOffset, nsIntSize(tileSize.width, tileSize.height));
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
  GetCompositor()->DrawDiagnostics(DiagnosticFlags::CONTENT,
                                   rect, aClipRect, aTransform, mFlashCounter);
}

void
TiledContentHost::PrintInfo(nsACString& aTo, const char* aPrefix)
{
  aTo += aPrefix;
  aTo += nsPrintfCString("TiledContentHost (0x%p)", this);

}

#ifdef MOZ_DUMP_PAINTING
void
TiledContentHost::Dump(FILE* aFile,
                       const char* aPrefix,
                       bool aDumpHtml)
{
  if (!aFile) {
    aFile = stderr;
  }

  TiledLayerBufferComposite::Iterator it = mTiledBuffer.TilesBegin();
  TiledLayerBufferComposite::Iterator stop = mTiledBuffer.TilesEnd();
  if (aDumpHtml) {
    fprintf_stderr(aFile, "<ul>");
  }
  for (;it != stop; ++it) {
    fprintf_stderr(aFile, "%s", aPrefix);
    fprintf_stderr(aFile, aDumpHtml ? "<li> <a href=" : "Tile ");
    if (it->IsPlaceholderTile()) {
      fprintf_stderr(aFile, "empty tile");
    } else {
      DumpTextureHost(aFile, it->mTextureHost);
    }
    fprintf_stderr(aFile, aDumpHtml ? " >Tile</a></li>" : " ");
  }
    if (aDumpHtml) {
    fprintf_stderr(aFile, "</ul>");
  }
}
#endif

} 
} 
