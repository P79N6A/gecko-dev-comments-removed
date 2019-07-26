



#include "mozilla/layers/PLayersChild.h"
#include "BasicTiledThebesLayer.h"
#include "gfxImageSurface.h"
#include "sampler.h"
#include "gfxPlatform.h"

#ifdef GFX_TILEDLAYER_DEBUG_OVERLAY
#include "cairo.h"
#include <sstream>
using mozilla::layers::Layer;
static void DrawDebugOverlay(gfxImageSurface* imgSurf, int x, int y)
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
namespace layers {

bool
BasicTiledLayerBuffer::HasFormatChanged(BasicTiledThebesLayer* aThebesLayer) const
{
  return aThebesLayer->CanUseOpaqueSurface() != mLastPaintOpaque;
}


gfxASurface::gfxImageFormat
BasicTiledLayerBuffer::GetFormat() const
{
  if (mThebesLayer->CanUseOpaqueSurface()) {
    return gfxASurface::ImageFormatRGB16_565;
  } else {
    return gfxASurface::ImageFormatARGB32;
  }
}

void
BasicTiledLayerBuffer::PaintThebes(BasicTiledThebesLayer* aLayer,
                                   const nsIntRegion& aNewValidRegion,
                                   const nsIntRegion& aPaintRegion,
                                   LayerManager::DrawThebesLayerCallback aCallback,
                                   void* aCallbackData)
{
  mThebesLayer = aLayer;
  mCallback = aCallback;
  mCallbackData = aCallbackData;

#ifdef GFX_TILEDLAYER_PREF_WARNINGS
  long start = PR_IntervalNow();
#endif

  
  NS_ASSERTION(!aPaintRegion.GetBounds().IsEmpty(), "Empty paint region\n");

  bool useSinglePaintBuffer = UseSinglePaintBuffer();
  if (useSinglePaintBuffer) {
    
    
    nsIntRect paintBounds = aPaintRegion.GetBounds();
    useSinglePaintBuffer = GetTileStart(paintBounds.x) !=
                           GetTileStart(paintBounds.XMost() - 1) ||
                           GetTileStart(paintBounds.y) !=
                           GetTileStart(paintBounds.YMost() - 1);
  }

  if (useSinglePaintBuffer) {
    const nsIntRect bounds = aPaintRegion.GetBounds();
    {
      SAMPLE_LABEL("BasicTiledLayerBuffer", "PaintThebesSingleBufferAlloc");
      mSinglePaintBuffer = new gfxImageSurface(gfxIntSize(bounds.width, bounds.height), GetFormat(), !aLayer->CanUseOpaqueSurface());
      mSinglePaintBufferOffset = nsIntPoint(bounds.x, bounds.y);
    }
    nsRefPtr<gfxContext> ctxt = new gfxContext(mSinglePaintBuffer);
    ctxt->NewPath();
    ctxt->Translate(gfxPoint(-bounds.x, -bounds.y));
#ifdef GFX_TILEDLAYER_PREF_WARNINGS
    if (PR_IntervalNow() - start > 3) {
      printf_stderr("Slow alloc %i\n", PR_IntervalNow() - start);
    }
    start = PR_IntervalNow();
#endif
    SAMPLE_LABEL("BasicTiledLayerBuffer", "PaintThebesSingleBufferDraw");

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

  SAMPLE_LABEL("BasicTiledLayerBuffer", "PaintThebesUpdate");
  Update(aNewValidRegion, aPaintRegion);

#ifdef GFX_TILEDLAYER_PREF_WARNINGS
  if (PR_IntervalNow() - start > 10) {
    const nsIntRect bounds = aPaintRegion.GetBounds();
    printf_stderr("Time to tile %i: %i, %i, %i, %i\n", PR_IntervalNow() - start, bounds.x, bounds.y, bounds.width, bounds.height);
  }
#endif

  mLastPaintOpaque = mThebesLayer->CanUseOpaqueSurface();
  mThebesLayer = nullptr;
  mCallback = nullptr;
  mCallbackData = nullptr;
  mSinglePaintBuffer = nullptr;
}

BasicTiledLayerTile
BasicTiledLayerBuffer::ValidateTileInternal(BasicTiledLayerTile aTile,
                                            const nsIntPoint& aTileOrigin,
                                            const nsIntRect& aDirtyRect)
{
  if (aTile == GetPlaceholderTile() || aTile.mSurface->Format() != GetFormat()) {
    gfxImageSurface* tmpTile = new gfxImageSurface(gfxIntSize(GetTileLength(), GetTileLength()),
                                                   GetFormat(), !mThebesLayer->CanUseOpaqueSurface());
    aTile = BasicTiledLayerTile(tmpTile);
  }

  gfxRect drawRect(aDirtyRect.x - aTileOrigin.x, aDirtyRect.y - aTileOrigin.y,
                   aDirtyRect.width, aDirtyRect.height);

  
  
  
  gfxImageSurface* writableSurface;
  aTile.mSurface = aTile.mSurface->GetWritable(&writableSurface);

  
  nsRefPtr<gfxContext> ctxt = new gfxContext(writableSurface);
  if (mSinglePaintBuffer) {
    ctxt->SetOperator(gfxContext::OPERATOR_SOURCE);
    ctxt->NewPath();
    ctxt->SetSource(mSinglePaintBuffer.get(),
                    gfxPoint(mSinglePaintBufferOffset.x - aDirtyRect.x + drawRect.x,
                             mSinglePaintBufferOffset.y - aDirtyRect.y + drawRect.y));
    ctxt->Rectangle(drawRect, true);
    ctxt->Fill();
  } else {
    ctxt->NewPath();
    ctxt->Translate(gfxPoint(-aTileOrigin.x, -aTileOrigin.y));
    nsIntPoint a = aTileOrigin;
    mCallback(mThebesLayer, ctxt, nsIntRegion(nsIntRect(a, nsIntSize(GetTileLength(), GetTileLength()))), nsIntRegion(), mCallbackData);
  }

#ifdef GFX_TILEDLAYER_DEBUG_OVERLAY
  DrawDebugOverlay(writableSurface, aTileOrigin.x, aTileOrigin.y);
#endif

  return aTile;
}

BasicTiledLayerTile
BasicTiledLayerBuffer::ValidateTile(BasicTiledLayerTile aTile,
                                    const nsIntPoint& aTileOrigin,
                                    const nsIntRegion& aDirtyRegion)
{

  SAMPLE_LABEL("BasicTiledLayerBuffer", "ValidateTile");

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

void
BasicTiledThebesLayer::FillSpecificAttributes(SpecificLayerAttributes& aAttrs)
{
  aAttrs = ThebesLayerAttributes(GetValidRegion());
}

void
BasicTiledThebesLayer::PaintThebes(gfxContext* aContext,
                                   Layer* aMaskLayer,
                                   LayerManager::DrawThebesLayerCallback aCallback,
                                   void* aCallbackData,
                                   ReadbackProcessor* aReadback)
{
  if (!aCallback) {
    BasicManager()->SetTransactionIncomplete();
    return;
  }

  if (!HasShadow()) {
    NS_ASSERTION(false, "Shadow requested for painting\n");
    return;
  }

  if (mTiledBuffer.HasFormatChanged(this)) {
    mValidRegion = nsIntRegion();
  }

  nsIntRegion invalidRegion = mVisibleRegion;
  invalidRegion.Sub(invalidRegion, mValidRegion);
  if (invalidRegion.IsEmpty())
    return;
  nsIntRegion regionToPaint = invalidRegion;

  gfxSize resolution(1, 1);
  for (ContainerLayer* parent = GetParent(); parent; parent = parent->GetParent()) {
    const FrameMetrics& metrics = parent->GetFrameMetrics();
    resolution.width *= metrics.mResolution.width;
    resolution.height *= metrics.mResolution.height;
  }

  
  
  if (mTiledBuffer.GetResolution() != resolution) {
    mValidRegion = nsIntRegion();
  }

  
  
  gfx::Point scrollOffset(0, 0);
  Layer* primaryScrollable = BasicManager()->GetPrimaryScrollableLayer();
  if (primaryScrollable) {
    const FrameMetrics& metrics = primaryScrollable->AsContainerLayer()->GetFrameMetrics();
    scrollOffset = metrics.mScrollOffset;
  }
  int32_t scrollDiffX = scrollOffset.x - mLastScrollOffset.x;
  int32_t scrollDiffY = scrollOffset.y - mLastScrollOffset.y;

  
  if (gfxPlatform::UseProgressiveTilePainting() &&
      mTiledBuffer.GetResolution() == resolution &&
      (scrollDiffX != 0 || scrollDiffY != 0)) {
    
    nsIntRegion freshRegion = mTiledBuffer.GetValidRegion();
    freshRegion.And(freshRegion, invalidRegion);
    freshRegion.Sub(invalidRegion, freshRegion);

    
    
    
    gfx::Rect viewport;
    float scaleX, scaleY;
    if (BasicManager()->ProgressiveUpdateCallback(!freshRegion.IsEmpty(), viewport, scaleX, scaleY)) {
      return;
    }

    

    
    gfx3DMatrix transform = GetEffectiveTransform();
    
    
    
    for (ContainerLayer* parent = GetParent(); parent; parent = parent->GetParent()) {
      if (parent->UseIntermediateSurface()) {
        transform.PreMultiply(parent->GetEffectiveTransform());
      }
    }
    transform.Invert();

    
    gfxRect transformedViewport(viewport.x - (scrollOffset.x * resolution.width),
                                viewport.y - (scrollOffset.y * resolution.height),
                                viewport.width, viewport.height);
    transformedViewport.Scale((scaleX / resolution.width) / resolution.width,
                              (scaleY / resolution.height) / resolution.height);
    transformedViewport = transform.TransformBounds(transformedViewport);

    nsIntRect roundedTransformedViewport((int32_t)floor(transformedViewport.x),
                                         (int32_t)floor(transformedViewport.y),
                                         (int32_t)ceil(transformedViewport.width),
                                         (int32_t)ceil(transformedViewport.height));

    
    if (!freshRegion.IsEmpty()) {
      regionToPaint = freshRegion;
    }
    if (regionToPaint.Intersects(roundedTransformedViewport)) {
      regionToPaint.And(regionToPaint, roundedTransformedViewport);
    }

    
    
    NS_ASSERTION(!regionToPaint.IsEmpty(), "Unexpectedly empty paint region!");
    nsIntRect paintBounds = regionToPaint.GetBounds();

    int startX, incX, startY, incY;
    if (scrollOffset.x >= mLastScrollOffset.x) {
      startX = mTiledBuffer.RoundDownToTileEdge(paintBounds.x);
      incX = mTiledBuffer.GetTileLength();
    } else {
      startX = mTiledBuffer.RoundDownToTileEdge(paintBounds.XMost() - 1);
      incX = -mTiledBuffer.GetTileLength();
    }

    if (scrollOffset.y >= mLastScrollOffset.y) {
      startY = mTiledBuffer.RoundDownToTileEdge(paintBounds.y);
      incY = mTiledBuffer.GetTileLength();
    } else {
      startY = mTiledBuffer.RoundDownToTileEdge(paintBounds.YMost() - 1);
      incY = -mTiledBuffer.GetTileLength();
    }

    
    nsIntRect tileBounds(startX, startY,
                         mTiledBuffer.GetTileLength(),
                         mTiledBuffer.GetTileLength());
    
    
    
    while (true) {
      regionToPaint.And(invalidRegion, tileBounds);
      if (!regionToPaint.IsEmpty()) {
        break;
      }
      if (NS_ABS(scrollDiffY) >= NS_ABS(scrollDiffX)) {
        tileBounds.x += incX;
      } else {
        tileBounds.y += incY;
      }
    }

    if (!regionToPaint.Contains(invalidRegion)) {
      
      
      BasicManager()->SetRepeatTransaction();

      
      mValidRegion.And(mValidRegion, mVisibleRegion);
    } else {
      
      mLastScrollOffset = scrollOffset;
    }

    
    mValidRegion.Or(mValidRegion, regionToPaint);
  } else {
    mTiledBuffer.SetResolution(resolution);
    mValidRegion = mVisibleRegion;
  }

  mTiledBuffer.PaintThebes(this, mValidRegion, regionToPaint, aCallback, aCallbackData);

  mTiledBuffer.ReadLock();
  if (aMaskLayer) {
    static_cast<BasicImplData*>(aMaskLayer->ImplData())
      ->Paint(aContext, nullptr);
  }

  
  
  
  
  BasicTiledLayerBuffer *heapCopy = new BasicTiledLayerBuffer(mTiledBuffer);

  BasicManager()->PaintedTiledLayerBuffer(BasicManager()->Hold(this), heapCopy);
}

} 
} 
