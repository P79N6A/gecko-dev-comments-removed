




#include "TiledContentHost.h"
#include "PaintedLayerComposite.h"      
#include "mozilla/gfx/BaseSize.h"       
#include "mozilla/gfx/Matrix.h"         
#include "mozilla/layers/Compositor.h"  
#include "mozilla/layers/Effects.h"     
#include "mozilla/layers/LayerMetricsWrapper.h" 
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
  : mFrameResolution()
  , mHasDoubleBufferedTiles(false)
  , mIsValid(false)
{}

 void
TiledLayerBufferComposite::RecycleCallback(TextureHost* textureHost, void* aClosure)
{
  textureHost->CompositorRecycle();
}

TiledLayerBufferComposite::TiledLayerBufferComposite(ISurfaceAllocator* aAllocator,
                                                     const SurfaceDescriptorTiles& aDescriptor,
                                                     const nsIntRegion& aOldPaintedRegion,
                                                     Compositor* aCompositor)
{
  mIsValid = true;
  mHasDoubleBufferedTiles = false;
  mValidRegion = aDescriptor.validRegion();
  mPaintedRegion = aDescriptor.paintedRegion();
  mRetainedWidth = aDescriptor.retainedWidth();
  mRetainedHeight = aDescriptor.retainedHeight();
  mResolution = aDescriptor.resolution();
  mFrameResolution = CSSToParentLayerScale2D(aDescriptor.frameXResolution(),
                                             aDescriptor.frameYResolution());
  if (mResolution == 0 || IsNaN(mResolution)) {
    
    
    mIsValid = false;
    return;
  }

  
  nsIntRegion oldPaintedRegion(aOldPaintedRegion);
  oldPaintedRegion.And(oldPaintedRegion, mValidRegion);
  mPaintedRegion.Or(mPaintedRegion, oldPaintedRegion);

  bool isSameProcess = aAllocator->IsSameProcess();

  const InfallibleTArray<TileDescriptor>& tiles = aDescriptor.tiles();
  for(size_t i = 0; i < tiles.Length(); i++) {
    CompositableTextureHostRef texture;
    CompositableTextureHostRef textureOnWhite;
    const TileDescriptor& tileDesc = tiles[i];
    switch (tileDesc.type()) {
      case TileDescriptor::TTexturedTileDescriptor : {
        texture = TextureHost::AsTextureHost(tileDesc.get_TexturedTileDescriptor().textureParent());
        MaybeTexture onWhite = tileDesc.get_TexturedTileDescriptor().textureOnWhite();
        if (onWhite.type() == MaybeTexture::TPTextureParent) {
          textureOnWhite = TextureHost::AsTextureHost(onWhite.get_PTextureParent());
        }
        const TileLock& ipcLock = tileDesc.get_TexturedTileDescriptor().sharedLock();
        nsRefPtr<gfxSharedReadLock> sharedLock;
        if (ipcLock.type() == TileLock::TShmemSection) {
          sharedLock = gfxShmSharedReadLock::Open(aAllocator, ipcLock.get_ShmemSection());
        } else {
          if (!isSameProcess) {
            
            
            NS_ERROR("A client process may be trying to peek at the host's address space!");
            
            
            mIsValid = false;

            mRetainedTiles.Clear();
            return;
          }
          sharedLock = reinterpret_cast<gfxMemorySharedReadLock*>(ipcLock.get_uintptr_t());
          if (sharedLock) {
            
            sharedLock.get()->Release();
          }
        }

        CompositableTextureSourceRef textureSource;
        CompositableTextureSourceRef textureSourceOnWhite;
        if (texture) {
          texture->SetCompositor(aCompositor);
          texture->PrepareTextureSource(textureSource);
        }
        if (textureOnWhite) {
          textureOnWhite->SetCompositor(aCompositor);
          textureOnWhite->PrepareTextureSource(textureSourceOnWhite);
        }
        mRetainedTiles.AppendElement(TileHost(sharedLock,
                                              texture.get(),
                                              textureOnWhite.get(),
                                              textureSource.get(),
                                              textureSourceOnWhite.get()));
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
    mRetainedTiles[i].mTextureHostOnWhite = nullptr;
    mRetainedTiles[i].mTextureSource = nullptr;
    mRetainedTiles[i].mTextureSourceOnWhite = nullptr;
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

#ifdef MOZ_GFX_OPTIMIZE_MOBILE
  MOZ_ASSERT(!aTile.mTextureHostOnWhite);
  
  
  
  
  aTile.mTextureHost->Updated(nullptr);
#else
  nsIntRegion tileUpdated = aDirtyRect.MovedBy(-aTileOrigin);
  aTile.mTextureHost->Updated(&tileUpdated);
  if (aTile.mTextureHostOnWhite) {
    aTile.mTextureHostOnWhite->Updated(&tileUpdated);
  }
#endif

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
  MOZ_ASSERT(aCompositor);
  if (!IsValid()) {
    return;
  }
  for (size_t i = 0; i < mRetainedTiles.Length(); i++) {
    if (mRetainedTiles[i].IsPlaceholderTile()) continue;
    mRetainedTiles[i].mTextureHost->SetCompositor(aCompositor);
    if (mRetainedTiles[i].mTextureHostOnWhite) {
      mRetainedTiles[i].mTextureHostOnWhite->SetCompositor(aCompositor);
    }
  }
}

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
}

void
TiledContentHost::Detach(Layer* aLayer,
                         AttachFlags aFlags )
{
  if (!mKeepAttached || aLayer == mLayer || aFlags & FORCE_DETACH) {

    
    
    
    
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

    mTiledBuffer = TiledLayerBufferComposite();
    mLowPrecisionTiledBuffer = TiledLayerBufferComposite();
    mOldTiledBuffer = TiledLayerBufferComposite();
    mOldLowPrecisionTiledBuffer = TiledLayerBufferComposite();
  }
  CompositableHost::Detach(aLayer,aFlags);
}

bool
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
      TiledLayerBufferComposite(aAllocator,
                                aTiledDescriptor,
                                mLowPrecisionTiledBuffer.GetPaintedRegion(),
                                mCompositor);
    if (!mLowPrecisionTiledBuffer.IsValid()) {
      
      
      
      mPendingLowPrecisionUpload = false;
      mPendingUpload = false;
      return false;
    }
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
    mTiledBuffer = TiledLayerBufferComposite(aAllocator,
                                             aTiledDescriptor,
                                             mTiledBuffer.GetPaintedRegion(),
                                             mCompositor);
    if (!mTiledBuffer.IsValid()) {
      
      
      
      mPendingLowPrecisionUpload = false;
      mPendingUpload = false;
      return false;
    }
  }
  return true;
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
                             const gfxRGBA* aBackgroundColor,
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

  if (aBackgroundColor) {
    aEffectChain.mPrimaryEffect = new EffectSolidColor(ToColor(*aBackgroundColor));
    nsIntRegionRectIterator it(aScreenRegion);
    for (const nsIntRect* rect = it.Next(); rect != nullptr; rect = it.Next()) {
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

  RefPtr<TexturedEffect> effect = CreateTexturedEffect(aTile.mTextureSource, aTile.mTextureSourceOnWhite, aFilter, true);
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
          RenderTile(tileTexture, aBackgroundColor, aEffectChain, aOpacity, aTransform,
                     aFilter, aClipRect, tileDrawRegion, tileOffset,
                     nsIntSize(tileSize.width, tileSize.height));
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
