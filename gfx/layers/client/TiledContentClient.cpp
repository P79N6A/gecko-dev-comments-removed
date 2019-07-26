




#include "mozilla/layers/TiledContentClient.h"
#include <math.h>                       
#include "ClientTiledThebesLayer.h"     
#include "GeckoProfiler.h"              
#include "ClientLayerManager.h"         
#include "gfxContext.h"                 
#include "gfxPlatform.h"                
#include "gfxRect.h"                    
#include "mozilla/MathAlgorithms.h"     
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Rect.h"           
#include "mozilla/layers/CompositableForwarder.h"
#include "mozilla/layers/ShadowLayers.h"  
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsSize.h"                     
#include "gfxReusableSharedImageSurfaceWrapper.h"
#include "nsMathUtils.h"               

#ifdef GFX_TILEDLAYER_DEBUG_OVERLAY
#include "cairo.h"
#include <sstream>
using mozilla::layers::Layer;
static void DrawDebugOverlay(gfxASurface* imgSurf, int x, int y)
{
  gfxContext c(imgSurf);

  
  c.NewPath();
  c.SetDeviceColor(gfxRGBA(0.0, 0.0, 0.0, 1.0));
  c.Rectangle(gfxRect(gfxPoint(0,0),imgSurf->GetSize()));
  c.Stroke();

  
  std::stringstream ss;
  ss << x << ", " << y;

  
  cairo_t* cr = c.GetCairo();
  cairo_set_font_size(cr, 25);
  cairo_text_extents_t extents;
  cairo_text_extents(cr, ss.str().c_str(), &extents);

  int textWidth = extents.width + 6;

  c.NewPath();
  c.SetDeviceColor(gfxRGBA(0.0, 0.0, 0.0, 1.0));
  c.Rectangle(gfxRect(gfxPoint(2,2),gfxSize(textWidth, 30)));
  c.Fill();

  c.NewPath();
  c.SetDeviceColor(gfxRGBA(1.0, 0.0, 0.0, 1.0));
  c.Rectangle(gfxRect(gfxPoint(2,2),gfxSize(textWidth, 30)));
  c.Stroke();

  c.NewPath();
  cairo_move_to(cr, 4, 28);
  cairo_show_text(cr, ss.str().c_str());

}

#endif

namespace mozilla {

using namespace gfx;

namespace layers {


TiledContentClient::TiledContentClient(ClientTiledThebesLayer* aThebesLayer,
                                       ClientLayerManager* aManager)
  : CompositableClient(aManager->AsShadowForwarder())
  , mTiledBuffer(aThebesLayer, aManager)
  , mLowPrecisionTiledBuffer(aThebesLayer, aManager)
{
  MOZ_COUNT_CTOR(TiledContentClient);

  mLowPrecisionTiledBuffer.SetResolution(gfxPlatform::GetLowPrecisionResolution());
}

void
TiledContentClient::LockCopyAndWrite(TiledBufferType aType)
{
  BasicTiledLayerBuffer* buffer = aType == LOW_PRECISION_TILED_BUFFER
    ? &mLowPrecisionTiledBuffer
    : &mTiledBuffer;

  
  
  
  buffer->ReadLock();

  mForwarder->PaintedTiledLayerBuffer(this, buffer->GetSurfaceDescriptorTiles());
  buffer->ClearPaintedRegion();
}

BasicTiledLayerBuffer::BasicTiledLayerBuffer(ClientTiledThebesLayer* aThebesLayer,
                                             ClientLayerManager* aManager)
  : mThebesLayer(aThebesLayer)
  , mManager(aManager)
  , mLastPaintOpaque(false)
{
}

bool
BasicTiledLayerBuffer::HasFormatChanged() const
{
  return mThebesLayer->CanUseOpaqueSurface() != mLastPaintOpaque;
}


gfxASurface::gfxContentType
BasicTiledLayerBuffer::GetContentType() const
{
  if (mThebesLayer->CanUseOpaqueSurface()) {
    return gfxASurface::CONTENT_COLOR;
  } else {
    return gfxASurface::CONTENT_COLOR_ALPHA;
  }
}


TileDescriptor
BasicTiledLayerTile::GetTileDescriptor()
{
  gfxReusableSurfaceWrapper* surface = GetSurface();
  switch (surface->GetType()) {
  case gfxReusableSurfaceWrapper::TYPE_IMAGE :
    return BasicTileDescriptor(uintptr_t(surface));

  case gfxReusableSurfaceWrapper::TYPE_SHARED_IMAGE :
    return BasicShmTileDescriptor(static_cast<gfxReusableSharedImageSurfaceWrapper*>(surface)->GetShmem());

  default :
    NS_NOTREACHED("Unhandled gfxReusableSurfaceWrapper type");
    return PlaceholderTileDescriptor();
  }
}


 BasicTiledLayerTile
BasicTiledLayerTile::OpenDescriptor(ISurfaceAllocator *aAllocator, const TileDescriptor& aDesc)
{
  switch (aDesc.type()) {
  case TileDescriptor::TBasicShmTileDescriptor : {
    nsRefPtr<gfxReusableSurfaceWrapper> surface =
      gfxReusableSharedImageSurfaceWrapper::Open(
        aAllocator, aDesc.get_BasicShmTileDescriptor().reusableSurface());
    return BasicTiledLayerTile(
      new DeprecatedTextureClientTile(nullptr, TextureInfo(BUFFER_TILED), surface));
  }

  case TileDescriptor::TBasicTileDescriptor : {
    nsRefPtr<gfxReusableSurfaceWrapper> surface =
      reinterpret_cast<gfxReusableSurfaceWrapper*>(
        aDesc.get_BasicTileDescriptor().reusableSurface());
    surface->ReadUnlock();
    return BasicTiledLayerTile(
      new DeprecatedTextureClientTile(nullptr, TextureInfo(BUFFER_TILED), surface));
  }

  default :
    NS_NOTREACHED("Unknown tile descriptor type!");
    return nullptr;
  }
}

SurfaceDescriptorTiles
BasicTiledLayerBuffer::GetSurfaceDescriptorTiles()
{
  InfallibleTArray<TileDescriptor> tiles;

  for (size_t i = 0; i < mRetainedTiles.Length(); i++) {
    TileDescriptor tileDesc;
    if (mRetainedTiles.SafeElementAt(i, GetPlaceholderTile()) == GetPlaceholderTile()) {
      tileDesc = PlaceholderTileDescriptor();
    } else {
      tileDesc = mRetainedTiles[i].GetTileDescriptor();
    }
    tiles.AppendElement(tileDesc);
  }
  return SurfaceDescriptorTiles(mValidRegion, mPaintedRegion,
                                tiles, mRetainedWidth, mRetainedHeight,
                                mResolution);
}

 BasicTiledLayerBuffer
BasicTiledLayerBuffer::OpenDescriptor(ISurfaceAllocator *aAllocator,
                                      const SurfaceDescriptorTiles& aDescriptor)
{
  return BasicTiledLayerBuffer(aAllocator,
                               aDescriptor.validRegion(),
                               aDescriptor.paintedRegion(),
                               aDescriptor.tiles(),
                               aDescriptor.retainedWidth(),
                               aDescriptor.retainedHeight(),
                               aDescriptor.resolution());
}

void
BasicTiledLayerBuffer::PaintThebes(const nsIntRegion& aNewValidRegion,
                                   const nsIntRegion& aPaintRegion,
                                   LayerManager::DrawThebesLayerCallback aCallback,
                                   void* aCallbackData)
{
  mCallback = aCallback;
  mCallbackData = aCallbackData;

#ifdef GFX_TILEDLAYER_PREF_WARNINGS
  long start = PR_IntervalNow();
#endif

  
  NS_ASSERTION(!aPaintRegion.GetBounds().IsEmpty(), "Empty paint region\n");

  bool useSinglePaintBuffer = UseSinglePaintBuffer();
  
  











  if (useSinglePaintBuffer) {
    nsRefPtr<gfxContext> ctxt;

    const nsIntRect bounds = aPaintRegion.GetBounds();
    {
      PROFILER_LABEL("BasicTiledLayerBuffer", "PaintThebesSingleBufferAlloc");
      gfxASurface::gfxImageFormat format =
        gfxPlatform::GetPlatform()->OptimalFormatForContent(
          GetContentType());

      if (gfxPlatform::GetPlatform()->SupportsAzureContent()) {
        mSinglePaintDrawTarget =
          gfxPlatform::GetPlatform()->CreateOffscreenContentDrawTarget(
            gfx::IntSize(ceilf(bounds.width * mResolution),
                         ceilf(bounds.height * mResolution)),
            gfx::ImageFormatToSurfaceFormat(format));

        ctxt = new gfxContext(mSinglePaintDrawTarget);
      } else {
        mSinglePaintBuffer = new gfxImageSurface(
          gfxIntSize(ceilf(bounds.width * mResolution),
                     ceilf(bounds.height * mResolution)),
          format,
          !mThebesLayer->CanUseOpaqueSurface());
        ctxt = new gfxContext(mSinglePaintBuffer);
      }

      mSinglePaintBufferOffset = nsIntPoint(bounds.x, bounds.y);
    }
    ctxt->NewPath();
    ctxt->Scale(mResolution, mResolution);
    ctxt->Translate(gfxPoint(-bounds.x, -bounds.y));
#ifdef GFX_TILEDLAYER_PREF_WARNINGS
    if (PR_IntervalNow() - start > 3) {
      printf_stderr("Slow alloc %i\n", PR_IntervalNow() - start);
    }
    start = PR_IntervalNow();
#endif
    PROFILER_LABEL("BasicTiledLayerBuffer", "PaintThebesSingleBufferDraw");

    mCallback(mThebesLayer, ctxt, aPaintRegion, nsIntRegion(), mCallbackData);
  }

#ifdef GFX_TILEDLAYER_PREF_WARNINGS
  if (PR_IntervalNow() - start > 30) {
    const nsIntRect bounds = aPaintRegion.GetBounds();
    printf_stderr("Time to draw %i: %i, %i, %i, %i\n", PR_IntervalNow() - start, bounds.x, bounds.y, bounds.width, bounds.height);
    if (aPaintRegion.IsComplex()) {
      printf_stderr("Complex region\n");
      nsIntRegionRectIterator it(aPaintRegion);
      for (const nsIntRect* rect = it.Next(); rect != nullptr; rect = it.Next()) {
        printf_stderr(" rect %i, %i, %i, %i\n", rect->x, rect->y, rect->width, rect->height);
      }
    }
  }
  start = PR_IntervalNow();
#endif

  PROFILER_LABEL("BasicTiledLayerBuffer", "PaintThebesUpdate");
  Update(aNewValidRegion, aPaintRegion);

#ifdef GFX_TILEDLAYER_PREF_WARNINGS
  if (PR_IntervalNow() - start > 10) {
    const nsIntRect bounds = aPaintRegion.GetBounds();
    printf_stderr("Time to tile %i: %i, %i, %i, %i\n", PR_IntervalNow() - start, bounds.x, bounds.y, bounds.width, bounds.height);
  }
#endif

  mLastPaintOpaque = mThebesLayer->CanUseOpaqueSurface();
  mCallback = nullptr;
  mCallbackData = nullptr;
  mSinglePaintBuffer = nullptr;
  mSinglePaintDrawTarget = nullptr;
}

BasicTiledLayerTile
BasicTiledLayerBuffer::ValidateTileInternal(BasicTiledLayerTile aTile,
                                            const nsIntPoint& aTileOrigin,
                                            const nsIntRect& aDirtyRect)
{
  if (aTile.IsPlaceholderTile()) {
    RefPtr<DeprecatedTextureClient> textureClient =
      new DeprecatedTextureClientTile(mManager, TextureInfo(BUFFER_TILED));
    aTile.mDeprecatedTextureClient = static_cast<DeprecatedTextureClientTile*>(textureClient.get());
  }
  aTile.mDeprecatedTextureClient->EnsureAllocated(gfx::IntSize(GetTileLength(), GetTileLength()), GetContentType());
  gfxImageSurface* writableSurface = aTile.mDeprecatedTextureClient->LockImageSurface();
  
  nsRefPtr<gfxContext> ctxt;

  RefPtr<gfx::DrawTarget> writableDrawTarget;
  if (gfxPlatform::GetPlatform()->SupportsAzureContent()) {
    
    
    
    gfx::SurfaceFormat format =
      gfx::ImageFormatToSurfaceFormat(writableSurface->Format());

    writableDrawTarget =
      gfxPlatform::GetPlatform()->CreateDrawTargetForData(
        writableSurface->Data(),
        gfx::IntSize(writableSurface->Width(), writableSurface->Height()),
        writableSurface->Stride(),
        format);
    ctxt = new gfxContext(writableDrawTarget);
  } else {
    ctxt = new gfxContext(writableSurface);
    ctxt->SetOperator(gfxContext::OPERATOR_SOURCE);
  }

  gfxRect drawRect(aDirtyRect.x - aTileOrigin.x, aDirtyRect.y - aTileOrigin.y,
                   aDirtyRect.width, aDirtyRect.height);

  if (mSinglePaintBuffer || mSinglePaintDrawTarget) {
    if (gfxPlatform::GetPlatform()->SupportsAzureContent()) {
      gfx::Rect drawRect(aDirtyRect.x - aTileOrigin.x,
                         aDirtyRect.y - aTileOrigin.y,
                         aDirtyRect.width,
                         aDirtyRect.height);
      drawRect.Scale(mResolution);

      RefPtr<gfx::SourceSurface> source = mSinglePaintDrawTarget->Snapshot();
      writableDrawTarget->CopySurface(
        source,
        gfx::IntRect(NS_roundf((aDirtyRect.x - mSinglePaintBufferOffset.x) * mResolution),
                     NS_roundf((aDirtyRect.y - mSinglePaintBufferOffset.y) * mResolution),
                     drawRect.width,
                     drawRect.height),
        gfx::IntPoint(NS_roundf(drawRect.x), NS_roundf(drawRect.y)));
    } else {
      gfxRect drawRect(aDirtyRect.x - aTileOrigin.x, aDirtyRect.y - aTileOrigin.y,
                       aDirtyRect.width, aDirtyRect.height);
      drawRect.Scale(mResolution, mResolution);

      ctxt->NewPath();
      ctxt->SetSource(mSinglePaintBuffer.get(),
                      gfxPoint((mSinglePaintBufferOffset.x - aDirtyRect.x) * mResolution + drawRect.x,
                               (mSinglePaintBufferOffset.y - aDirtyRect.y) * mResolution + drawRect.y));
      ctxt->SnappedRectangle(drawRect);
      ctxt->Fill();
    }
  } else {
    ctxt->NewPath();
    ctxt->Scale(mResolution, mResolution);
    ctxt->Translate(gfxPoint(-aTileOrigin.x, -aTileOrigin.y));
    nsIntPoint a = nsIntPoint(aTileOrigin.x, aTileOrigin.y);
    mCallback(mThebesLayer, ctxt,
              nsIntRegion(nsIntRect(a, nsIntSize(GetScaledTileLength(),
                                                 GetScaledTileLength()))),
              nsIntRegion(), mCallbackData);
  }

#ifdef GFX_TILEDLAYER_DEBUG_OVERLAY
  DrawDebugOverlay(writableSurface, aTileOrigin.x * mResolution,
                   aTileOrigin.y * mResolution);
#endif

  return aTile;
}

BasicTiledLayerTile
BasicTiledLayerBuffer::ValidateTile(BasicTiledLayerTile aTile,
                                    const nsIntPoint& aTileOrigin,
                                    const nsIntRegion& aDirtyRegion)
{
  PROFILER_LABEL("BasicTiledLayerBuffer", "ValidateTile");

#ifdef GFX_TILEDLAYER_PREF_WARNINGS
  if (aDirtyRegion.IsComplex()) {
    printf_stderr("Complex region\n");
  }
#endif

  nsIntRegionRectIterator it(aDirtyRegion);
  for (const nsIntRect* rect = it.Next(); rect != nullptr; rect = it.Next()) {
#ifdef GFX_TILEDLAYER_PREF_WARNINGS
    printf_stderr(" break into subrect %i, %i, %i, %i\n", rect->x, rect->y, rect->width, rect->height);
#endif
    aTile = ValidateTileInternal(aTile, aTileOrigin, *rect);
  }

  return aTile;
}

static nsIntRect
RoundedTransformViewportBounds(const gfx::Rect& aViewport,
                               const CSSPoint& aScrollOffset,
                               const gfxSize& aResolution,
                               float aScaleX,
                               float aScaleY,
                               const gfx3DMatrix& aTransform)
{
  gfxRect transformedViewport(aViewport.x - (aScrollOffset.x * aResolution.width),
                              aViewport.y - (aScrollOffset.y * aResolution.height),
                              aViewport.width, aViewport.height);
  transformedViewport.Scale((aScaleX / aResolution.width) / aResolution.width,
                            (aScaleY / aResolution.height) / aResolution.height);
  transformedViewport = aTransform.TransformBounds(transformedViewport);

  return nsIntRect((int32_t)floor(transformedViewport.x),
                   (int32_t)floor(transformedViewport.y),
                   (int32_t)ceil(transformedViewport.width),
                   (int32_t)ceil(transformedViewport.height));
}

bool
BasicTiledLayerBuffer::ComputeProgressiveUpdateRegion(const nsIntRegion& aInvalidRegion,
                                                      const nsIntRegion& aOldValidRegion,
                                                      nsIntRegion& aRegionToPaint,
                                                      BasicTiledLayerPaintData* aPaintData,
                                                      bool aIsRepeated)
{
  aRegionToPaint = aInvalidRegion;

  
  
  
  bool drawingLowPrecision = IsLowPrecision();

  
  nsIntRegion staleRegion;
  staleRegion.And(aInvalidRegion, aOldValidRegion);

  
  
  
  gfx::Rect viewport;
  float scaleX, scaleY;
  if (mManager->ProgressiveUpdateCallback(!staleRegion.Contains(aInvalidRegion),
                                          viewport,
                                          scaleX, scaleY, !drawingLowPrecision)) {
    PROFILER_LABEL("ContentClient", "Abort painting");
    aRegionToPaint.SetEmpty();
    return aIsRepeated;
  }

  
  nsIntRect roundedTransformedViewport =
    RoundedTransformViewportBounds(viewport, aPaintData->mScrollOffset, aPaintData->mResolution,
                                   scaleX, scaleY, aPaintData->mTransformScreenToLayer);

  
  
  
  
  nsIntRect criticalViewportRect = roundedTransformedViewport.Intersect(aPaintData->mCompositionBounds);
  aRegionToPaint.And(aInvalidRegion, criticalViewportRect);
  aRegionToPaint.Or(aRegionToPaint, staleRegion);
  bool drawingStale = !aRegionToPaint.IsEmpty();
  if (!drawingStale) {
    aRegionToPaint = aInvalidRegion;
  }

  
  bool paintVisible = false;
  if (aRegionToPaint.Intersects(roundedTransformedViewport)) {
    aRegionToPaint.And(aRegionToPaint, roundedTransformedViewport);
    paintVisible = true;
  }

  
  
  bool paintInSingleTransaction = paintVisible && (drawingStale || aPaintData->mFirstPaint);

  
  
  NS_ASSERTION(!aRegionToPaint.IsEmpty(), "Unexpectedly empty paint region!");
  nsIntRect paintBounds = aRegionToPaint.GetBounds();

  int startX, incX, startY, incY;
  int tileLength = GetScaledTileLength();
  if (aPaintData->mScrollOffset.x >= aPaintData->mLastScrollOffset.x) {
    startX = RoundDownToTileEdge(paintBounds.x);
    incX = tileLength;
  } else {
    startX = RoundDownToTileEdge(paintBounds.XMost() - 1);
    incX = -tileLength;
  }

  if (aPaintData->mScrollOffset.y >= aPaintData->mLastScrollOffset.y) {
    startY = RoundDownToTileEdge(paintBounds.y);
    incY = tileLength;
  } else {
    startY = RoundDownToTileEdge(paintBounds.YMost() - 1);
    incY = -tileLength;
  }

  
  nsIntRect tileBounds(startX, startY, tileLength, tileLength);
  int32_t scrollDiffX = aPaintData->mScrollOffset.x - aPaintData->mLastScrollOffset.x;
  int32_t scrollDiffY = aPaintData->mScrollOffset.y - aPaintData->mLastScrollOffset.y;
  
  
  
  while (true) {
    aRegionToPaint.And(aInvalidRegion, tileBounds);
    if (!aRegionToPaint.IsEmpty()) {
      break;
    }
    if (Abs(scrollDiffY) >= Abs(scrollDiffX)) {
      tileBounds.x += incX;
    } else {
      tileBounds.y += incY;
    }
  }

  if (!aRegionToPaint.Contains(aInvalidRegion)) {
    
    

    
    
    
    
    
    if (!drawingLowPrecision && paintInSingleTransaction) {
      return true;
    }

    mManager->SetRepeatTransaction();
    return false;
  }

  
  
  
  aPaintData->mPaintFinished = true;
  return false;
}

bool
BasicTiledLayerBuffer::ProgressiveUpdate(nsIntRegion& aValidRegion,
                                         nsIntRegion& aInvalidRegion,
                                         const nsIntRegion& aOldValidRegion,
                                         BasicTiledLayerPaintData* aPaintData,
                                         LayerManager::DrawThebesLayerCallback aCallback,
                                         void* aCallbackData)
{
  bool repeat = false;
  bool isBufferChanged = false;
  do {
    
    
    nsIntRegion regionToPaint;
    repeat = ComputeProgressiveUpdateRegion(aInvalidRegion,
                                            aOldValidRegion,
                                            regionToPaint,
                                            aPaintData,
                                            repeat);

    
    if (regionToPaint.IsEmpty()) {
      break;
    }

    isBufferChanged = true;

    
    aValidRegion.Or(aValidRegion, regionToPaint);

    
    
    
    nsIntRegion validOrStale;
    validOrStale.Or(aValidRegion, aOldValidRegion);

    
    PaintThebes(validOrStale, regionToPaint, aCallback, aCallbackData);
    aInvalidRegion.Sub(aInvalidRegion, regionToPaint);
  } while (repeat);

  
  
  return isBufferChanged;
}

}
}
