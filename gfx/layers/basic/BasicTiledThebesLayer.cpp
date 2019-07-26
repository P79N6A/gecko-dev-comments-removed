



#include "mozilla/layers/PLayersChild.h"
#include "BasicTiledThebesLayer.h"
#include "gfxImageSurface.h"
#include "sampler.h"
#include "gfxPlatform.h"
#include <cstdlib> 
#include <cmath> 

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
      mSinglePaintBuffer = new gfxImageSurface(
        gfxIntSize(ceilf(bounds.width * mResolution),
                   ceilf(bounds.height * mResolution)),
        GetFormat(), !aLayer->CanUseOpaqueSurface());
      mSinglePaintBufferOffset = nsIntPoint(bounds.x, bounds.y);
    }
    nsRefPtr<gfxContext> ctxt = new gfxContext(mSinglePaintBuffer);
    ctxt->NewPath();
    ctxt->Scale(mResolution, mResolution);
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

  
  
  
  gfxImageSurface* writableSurface;
  aTile.mSurface = aTile.mSurface->GetWritable(&writableSurface);

  
  nsRefPtr<gfxContext> ctxt = new gfxContext(writableSurface);

  if (mSinglePaintBuffer) {
    gfxRect drawRect(aDirtyRect.x - aTileOrigin.x, aDirtyRect.y - aTileOrigin.y,
                     aDirtyRect.width, aDirtyRect.height);

    ctxt->SetOperator(gfxContext::OPERATOR_SOURCE);
    ctxt->NewPath();
    ctxt->SetSource(mSinglePaintBuffer.get(),
                    gfxPoint((mSinglePaintBufferOffset.x - aDirtyRect.x + drawRect.x) *
                             mResolution,
                             (mSinglePaintBufferOffset.y - aDirtyRect.y + drawRect.y) *
                             mResolution));
    drawRect.Scale(mResolution, mResolution);
    ctxt->Rectangle(drawRect, true);
    ctxt->Fill();
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

BasicTiledThebesLayer::BasicTiledThebesLayer(BasicShadowLayerManager* const aManager)
  : ThebesLayer(aManager, static_cast<BasicImplData*>(this))
  , mLastScrollOffset(0, 0)
  , mFirstPaint(true)
{
  MOZ_COUNT_CTOR(BasicTiledThebesLayer);
  mLowPrecisionTiledBuffer.SetResolution(gfxPlatform::GetLowPrecisionResolution());
}

BasicTiledThebesLayer::~BasicTiledThebesLayer()
{
  MOZ_COUNT_DTOR(BasicTiledThebesLayer);
}

void
BasicTiledThebesLayer::FillSpecificAttributes(SpecificLayerAttributes& aAttrs)
{
  aAttrs = ThebesLayerAttributes(GetValidRegion());
}

static nsIntRect
RoundedTransformViewportBounds(const gfx::Rect& aViewport,
                               const gfx::Point& aScrollOffset,
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
BasicTiledThebesLayer::ComputeProgressiveUpdateRegion(BasicTiledLayerBuffer& aTiledBuffer,
                                                      const nsIntRegion& aInvalidRegion,
                                                      const nsIntRegion& aOldValidRegion,
                                                      nsIntRegion& aRegionToPaint,
                                                      const gfx3DMatrix& aTransform,
                                                      const nsIntRect& aCompositionBounds,
                                                      const gfx::Point& aScrollOffset,
                                                      const gfxSize& aResolution,
                                                      bool aIsRepeated)
{
  aRegionToPaint = aInvalidRegion;

  
  
  
  bool drawingLowPrecision = aTiledBuffer.IsLowPrecision();

  
  nsIntRegion staleRegion;
  staleRegion.And(aInvalidRegion, aOldValidRegion);

  
  
  
  gfx::Rect viewport;
  float scaleX, scaleY;
  if (BasicManager()->ProgressiveUpdateCallback(!staleRegion.Contains(aInvalidRegion),
                                                viewport,
                                                scaleX, scaleY, !drawingLowPrecision)) {
    SAMPLE_MARKER("Abort painting");
    aRegionToPaint.SetEmpty();
    return aIsRepeated;
  }

  
  nsIntRect roundedTransformedViewport =
    RoundedTransformViewportBounds(viewport, aScrollOffset, aResolution,
                                   scaleX, scaleY, aTransform);

  
  
  
  
  nsIntRect criticalViewportRect = roundedTransformedViewport.Intersect(aCompositionBounds);
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

  
  
  bool paintInSingleTransaction = paintVisible && (drawingStale || mFirstPaint);

  
  
  NS_ASSERTION(!aRegionToPaint.IsEmpty(), "Unexpectedly empty paint region!");
  nsIntRect paintBounds = aRegionToPaint.GetBounds();

  int startX, incX, startY, incY;
  int tileLength = aTiledBuffer.GetScaledTileLength();
  if (aScrollOffset.x >= mLastScrollOffset.x) {
    startX = aTiledBuffer.RoundDownToTileEdge(paintBounds.x);
    incX = tileLength;
  } else {
    startX = aTiledBuffer.RoundDownToTileEdge(paintBounds.XMost() - 1);
    incX = -tileLength;
  }

  if (aScrollOffset.y >= mLastScrollOffset.y) {
    startY = aTiledBuffer.RoundDownToTileEdge(paintBounds.y);
    incY = tileLength;
  } else {
    startY = aTiledBuffer.RoundDownToTileEdge(paintBounds.YMost() - 1);
    incY = -tileLength;
  }

  
  nsIntRect tileBounds(startX, startY, tileLength, tileLength);
  int32_t scrollDiffX = aScrollOffset.x - mLastScrollOffset.x;
  int32_t scrollDiffY = aScrollOffset.y - mLastScrollOffset.y;
  
  
  
  while (true) {
    aRegionToPaint.And(aInvalidRegion, tileBounds);
    if (!aRegionToPaint.IsEmpty()) {
      break;
    }
    if (std::abs(scrollDiffY) >= std::abs(scrollDiffX)) {
      tileBounds.x += incX;
    } else {
      tileBounds.y += incY;
    }
  }

  if (!aRegionToPaint.Contains(aInvalidRegion)) {
    
    

    
    
    
    
    
    if (!drawingLowPrecision && paintInSingleTransaction) {
      return true;
    }

    BasicManager()->SetRepeatTransaction();
    return false;
  }

  
  
  
  mPaintData.mPaintFinished = true;
  return false;
}

bool
BasicTiledThebesLayer::ProgressiveUpdate(BasicTiledLayerBuffer& aTiledBuffer,
                                         nsIntRegion& aValidRegion,
                                         nsIntRegion& aInvalidRegion,
                                         const nsIntRegion& aOldValidRegion,
                                         const gfx3DMatrix& aTransform,
                                         const nsIntRect& aCompositionBounds,
                                         const gfx::Point& aScrollOffset,
                                         const gfxSize& aResolution,
                                         LayerManager::DrawThebesLayerCallback aCallback,
                                         void* aCallbackData)
{
  bool repeat = false;
  bool isBufferChanged = false;
  do {
    
    
    nsIntRegion regionToPaint;
    repeat = ComputeProgressiveUpdateRegion(aTiledBuffer,
                                            aInvalidRegion,
                                            aOldValidRegion,
                                            regionToPaint,
                                            aTransform,
                                            aCompositionBounds,
                                            aScrollOffset,
                                            aResolution,
                                            repeat);

    
    if (regionToPaint.IsEmpty()) {
      break;
    }

    isBufferChanged = true;

    
    aValidRegion.Or(aValidRegion, regionToPaint);

    
    
    
    nsIntRegion validOrStale;
    validOrStale.Or(aValidRegion, aOldValidRegion);

    
    aTiledBuffer.PaintThebes(this, validOrStale, regionToPaint, aCallback, aCallbackData);
    aInvalidRegion.Sub(aInvalidRegion, regionToPaint);
  } while (repeat);

  
  
  return isBufferChanged;
}

void
BasicTiledThebesLayer::BeginPaint()
{
  if (BasicManager()->IsRepeatTransaction()) {
    return;
  }

  mPaintData.mLowPrecisionPaintCount = 0;
  mPaintData.mPaintFinished = false;

  
  mPaintData.mTransformScreenToLayer = GetEffectiveTransform();
  
  
  
  for (ContainerLayer* parent = GetParent(); parent; parent = parent->GetParent()) {
    if (parent->UseIntermediateSurface()) {
      mPaintData.mTransformScreenToLayer.PreMultiply(parent->GetEffectiveTransform());
    }
  }
  mPaintData.mTransformScreenToLayer.Invert();

  
  mPaintData.mLayerCriticalDisplayPort.SetEmpty();
  const gfx::Rect& criticalDisplayPort = GetParent()->GetFrameMetrics().mCriticalDisplayPort;
  if (!criticalDisplayPort.IsEmpty()) {
    gfxRect transformedCriticalDisplayPort =
      mPaintData.mTransformScreenToLayer.TransformBounds(
        gfxRect(criticalDisplayPort.x, criticalDisplayPort.y,
                criticalDisplayPort.width, criticalDisplayPort.height));
    transformedCriticalDisplayPort.RoundOut();
    mPaintData.mLayerCriticalDisplayPort = nsIntRect(transformedCriticalDisplayPort.x,
                                             transformedCriticalDisplayPort.y,
                                             transformedCriticalDisplayPort.width,
                                             transformedCriticalDisplayPort.height);
  }

  
  mPaintData.mResolution.SizeTo(1, 1);
  for (ContainerLayer* parent = GetParent(); parent; parent = parent->GetParent()) {
    const FrameMetrics& metrics = parent->GetFrameMetrics();
    mPaintData.mResolution.width *= metrics.mResolution.width;
    mPaintData.mResolution.height *= metrics.mResolution.height;
  }

  
  
  mPaintData.mCompositionBounds.SetEmpty();
  mPaintData.mScrollOffset.MoveTo(0, 0);
  Layer* primaryScrollable = BasicManager()->GetPrimaryScrollableLayer();
  if (primaryScrollable) {
    const FrameMetrics& metrics = primaryScrollable->AsContainerLayer()->GetFrameMetrics();
    mPaintData.mScrollOffset = metrics.mScrollOffset;
    gfxRect transformedViewport = mPaintData.mTransformScreenToLayer.TransformBounds(
      gfxRect(metrics.mCompositionBounds.x, metrics.mCompositionBounds.y,
              metrics.mCompositionBounds.width, metrics.mCompositionBounds.height));
    transformedViewport.RoundOut();
    mPaintData.mCompositionBounds =
      nsIntRect(transformedViewport.x, transformedViewport.y,
                transformedViewport.width, transformedViewport.height);
  }
}

void
BasicTiledThebesLayer::EndPaint(bool aFinish)
{
  if (!aFinish && !mPaintData.mPaintFinished) {
    return;
  }

  mLastScrollOffset = mPaintData.mScrollOffset;
  mPaintData.mPaintFinished = true;
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
  if (invalidRegion.IsEmpty()) {
    EndPaint(true);
    return;
  }

  
  
  if (!gfxPlatform::UseProgressiveTilePainting() &&
      !gfxPlatform::UseLowPrecisionBuffer() &&
      GetParent()->GetFrameMetrics().mCriticalDisplayPort.IsEmpty()) {
    mValidRegion = mVisibleRegion;
    mTiledBuffer.PaintThebes(this, mValidRegion, invalidRegion, aCallback, aCallbackData);
    mTiledBuffer.ReadLock();

    if (aMaskLayer) {
      static_cast<BasicImplData*>(aMaskLayer->ImplData())->Paint(aContext, nullptr);
    }

    
    
    
    
    BasicTiledLayerBuffer *heapCopy = new BasicTiledLayerBuffer(mTiledBuffer);
    BasicManager()->PaintedTiledLayerBuffer(BasicManager()->Hold(this), heapCopy);
    mTiledBuffer.ClearPaintedRegion();

    return;
  }

  
  BeginPaint();
  if (mPaintData.mPaintFinished) {
    return;
  }

  
  
  if (!BasicManager()->IsRepeatTransaction()) {
    mValidRegion.And(mValidRegion, mVisibleRegion);
  }

  nsIntRegion lowPrecisionInvalidRegion;
  if (!mPaintData.mLayerCriticalDisplayPort.IsEmpty()) {
    
    
    if (!BasicManager()->IsRepeatTransaction()) {
      mValidRegion.And(mValidRegion, mPaintData.mLayerCriticalDisplayPort);
    }

    if (gfxPlatform::UseLowPrecisionBuffer()) {
      
      lowPrecisionInvalidRegion.Sub(mVisibleRegion, mLowPrecisionValidRegion);

      
      
      lowPrecisionInvalidRegion.Sub(lowPrecisionInvalidRegion, mValidRegion);
    }

    
    invalidRegion.And(invalidRegion, mPaintData.mLayerCriticalDisplayPort);
    if (invalidRegion.IsEmpty() && lowPrecisionInvalidRegion.IsEmpty()) {
      EndPaint(true);
      return;
    }
  }

  if (!invalidRegion.IsEmpty() && mPaintData.mLowPrecisionPaintCount == 0) {
    bool updatedBuffer = false;
    
    if (gfxPlatform::UseProgressiveTilePainting() &&
        !BasicManager()->HasShadowTarget() &&
        mTiledBuffer.GetFrameResolution() == mPaintData.mResolution) {
      
      
      
      nsIntRegion oldValidRegion = mTiledBuffer.GetValidRegion();
      oldValidRegion.And(oldValidRegion, mVisibleRegion);
      if (!mPaintData.mLayerCriticalDisplayPort.IsEmpty()) {
        oldValidRegion.And(oldValidRegion, mPaintData.mLayerCriticalDisplayPort);
      }

      updatedBuffer =
        ProgressiveUpdate(mTiledBuffer, mValidRegion, invalidRegion,
                          oldValidRegion, mPaintData.mTransformScreenToLayer,
                          mPaintData.mCompositionBounds, mPaintData.mScrollOffset,
                          mPaintData.mResolution, aCallback, aCallbackData);
    } else {
      updatedBuffer = true;
      mTiledBuffer.SetFrameResolution(mPaintData.mResolution);
      mValidRegion = mVisibleRegion;
      if (!mPaintData.mLayerCriticalDisplayPort.IsEmpty()) {
        mValidRegion.And(mValidRegion, mPaintData.mLayerCriticalDisplayPort);
      }
      mTiledBuffer.PaintThebes(this, mValidRegion, invalidRegion, aCallback, aCallbackData);
    }

    if (updatedBuffer) {
      mFirstPaint = false;
      mTiledBuffer.ReadLock();

      
      if (aMaskLayer && !BasicManager()->IsRepeatTransaction()) {
        static_cast<BasicImplData*>(aMaskLayer->ImplData())
          ->Paint(aContext, nullptr);
      }

      
      BasicTiledLayerBuffer *heapCopy = new BasicTiledLayerBuffer(mTiledBuffer);

      BasicManager()->PaintedTiledLayerBuffer(BasicManager()->Hold(this), heapCopy);
      mTiledBuffer.ClearPaintedRegion();

      
      
      if (!lowPrecisionInvalidRegion.IsEmpty() && mPaintData.mPaintFinished) {
        BasicManager()->SetRepeatTransaction();
        mPaintData.mLowPrecisionPaintCount = 1;
        mPaintData.mPaintFinished = false;
      }

      
      
      EndPaint(false);
      return;
    }
  }

  
  
  bool updatedLowPrecision = false;
  if (!lowPrecisionInvalidRegion.IsEmpty() &&
      !nsIntRegion(mPaintData.mLayerCriticalDisplayPort).Contains(mVisibleRegion)) {
    nsIntRegion oldValidRegion = mLowPrecisionTiledBuffer.GetValidRegion();
    oldValidRegion.And(oldValidRegion, mVisibleRegion);

    
    if (mLowPrecisionTiledBuffer.GetFrameResolution() != mPaintData.mResolution ||
        mLowPrecisionTiledBuffer.HasFormatChanged(this)) {
      if (!mLowPrecisionValidRegion.IsEmpty()) {
        updatedLowPrecision = true;
      }
      oldValidRegion.SetEmpty();
      mLowPrecisionValidRegion.SetEmpty();
      mLowPrecisionTiledBuffer.SetFrameResolution(mPaintData.mResolution);
      lowPrecisionInvalidRegion = mVisibleRegion;
    }

    
    if (mPaintData.mLowPrecisionPaintCount == 1) {
      mLowPrecisionValidRegion.And(mLowPrecisionValidRegion, mVisibleRegion);
    }
    mPaintData.mLowPrecisionPaintCount++;

    
    
    lowPrecisionInvalidRegion.Sub(lowPrecisionInvalidRegion, mValidRegion);

    if (!lowPrecisionInvalidRegion.IsEmpty()) {
      updatedLowPrecision =
        ProgressiveUpdate(mLowPrecisionTiledBuffer, mLowPrecisionValidRegion,
                          lowPrecisionInvalidRegion, oldValidRegion,
                          mPaintData.mTransformScreenToLayer,
                          mPaintData.mCompositionBounds, mPaintData.mScrollOffset,
                          mPaintData.mResolution, aCallback, aCallbackData);
    }
  } else if (!mLowPrecisionValidRegion.IsEmpty()) {
    
    updatedLowPrecision = true;
    mLowPrecisionValidRegion.SetEmpty();
    mLowPrecisionTiledBuffer.PaintThebes(this, mLowPrecisionValidRegion,
                                         mLowPrecisionValidRegion, aCallback,
                                         aCallbackData);
  }

  
  
  
  if (updatedLowPrecision) {
    mLowPrecisionTiledBuffer.ReadLock();
    
    BasicTiledLayerBuffer *heapCopy = new BasicTiledLayerBuffer(mLowPrecisionTiledBuffer);

    
    
    BasicManager()->PaintedTiledLayerBuffer(BasicManager()->Hold(this), heapCopy);
    mLowPrecisionTiledBuffer.ClearPaintedRegion();
  }

  EndPaint(false);
}

} 
} 
