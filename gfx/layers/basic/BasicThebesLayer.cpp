




#include "BasicThebesLayer.h"
#include <stdint.h>                     
#include "GeckoProfiler.h"              
#include "ReadbackLayer.h"              
#include "ReadbackProcessor.h"          
#include "RenderTrace.h"                
#include "BasicLayersImpl.h"            
#include "gfxContext.h"                 
#include "gfxRect.h"                    
#include "gfxUtils.h"                   
#include "mozilla/gfx/2D.h"             
#include "mozilla/gfx/BaseRect.h"       
#include "mozilla/gfx/Matrix.h"         
#include "mozilla/gfx/Rect.h"           
#include "mozilla/gfx/Types.h"          
#include "mozilla/layers/LayersTypes.h"
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsISupportsImpl.h"            
#include "nsPoint.h"                    
#include "nsRect.h"                     
#include "nsTArray.h"                   
#include "AutoMaskData.h"
#include "gfx2DGlue.h"

namespace mozilla {
namespace layers {

using namespace mozilla::gfx;

static nsIntRegion
IntersectWithClip(const nsIntRegion& aRegion, gfxContext* aContext)
{
  gfxRect clip = aContext->GetClipExtents();
  clip.RoundOut();
  nsIntRect r(clip.X(), clip.Y(), clip.Width(), clip.Height());
  nsIntRegion result;
  result.And(aRegion, r);
  return result;
}

void
BasicThebesLayer::PaintThebes(gfxContext* aContext,
                              Layer* aMaskLayer,
                              LayerManager::DrawThebesLayerCallback aCallback,
                              void* aCallbackData)
{
  PROFILER_LABEL("BasicThebesLayer", "PaintThebes",
    js::ProfileEntry::Category::GRAPHICS);

  NS_ASSERTION(BasicManager()->InDrawing(),
               "Can only draw in drawing phase");

  float opacity = GetEffectiveOpacity();
  CompositionOp effectiveOperator = GetEffectiveOperator(this);

  if (!BasicManager()->IsRetained()) {
    mValidRegion.SetEmpty();
    mContentClient->Clear();

    nsIntRegion toDraw = IntersectWithClip(GetEffectiveVisibleRegion(), aContext);

    RenderTraceInvalidateStart(this, "FFFF00", toDraw.GetBounds());

    if (!toDraw.IsEmpty() && !IsHidden()) {
      if (!aCallback) {
        BasicManager()->SetTransactionIncomplete();
        return;
      }

      aContext->Save();

      bool needsClipToVisibleRegion = GetClipToVisibleRegion();
      bool needsGroup = opacity != 1.0 ||
                        effectiveOperator != CompositionOp::OP_OVER ||
                        aMaskLayer;
      nsRefPtr<gfxContext> groupContext;
      if (needsGroup) {
        groupContext =
          BasicManager()->PushGroupForLayer(aContext, this, toDraw,
                                            &needsClipToVisibleRegion);
        if (effectiveOperator != CompositionOp::OP_OVER) {
          needsClipToVisibleRegion = true;
        }
      } else {
        groupContext = aContext;
      }
      SetAntialiasingFlags(this, groupContext->GetDrawTarget());
      aCallback(this, groupContext, toDraw, DrawRegionClip::CLIP_NONE, nsIntRegion(), aCallbackData);
      if (needsGroup) {
        aContext->PopGroupToSource();
        if (needsClipToVisibleRegion) {
          gfxUtils::ClipToRegion(aContext, toDraw);
        }
        AutoSetOperator setOptimizedOperator(aContext, ThebesOp(effectiveOperator));
        PaintWithMask(aContext, opacity, aMaskLayer);
      }

      aContext->Restore();
    }

    RenderTraceInvalidateEnd(this, "FFFF00");
    return;
  }

  if (BasicManager()->IsTransactionIncomplete())
    return;

  gfxRect clipExtents;
  clipExtents = aContext->GetClipExtents();

  
  
  AutoMoz2DMaskData mask;
  SourceSurface* maskSurface = nullptr;
  Matrix maskTransform;
  if (GetMaskData(aMaskLayer, aContext->GetDeviceOffset(), &mask)) {
    maskSurface = mask.GetSurface();
    maskTransform = mask.GetTransform();
  }

  if (!IsHidden() && !clipExtents.IsEmpty()) {
    mContentClient->DrawTo(this, aContext->GetDrawTarget(), opacity,
                           effectiveOperator,
                           maskSurface, &maskTransform);
  }
}

void
BasicThebesLayer::Validate(LayerManager::DrawThebesLayerCallback aCallback,
                           void* aCallbackData,
                           ReadbackProcessor* aReadback)
{
  if (!mContentClient) {
    
    
    mContentClient = new ContentClientBasic();
  }

  if (!BasicManager()->IsRetained()) {
    return;
  }

  nsTArray<ReadbackProcessor::Update> readbackUpdates;
  if (aReadback && UsedForReadback()) {
    aReadback->GetThebesLayerUpdates(this, &readbackUpdates);
  }

  uint32_t flags = 0;
#ifndef MOZ_WIDGET_ANDROID
  if (BasicManager()->CompositorMightResample()) {
    flags |= RotatedContentBuffer::PAINT_WILL_RESAMPLE;
  }
  if (!(flags & RotatedContentBuffer::PAINT_WILL_RESAMPLE)) {
    if (MayResample()) {
      flags |= RotatedContentBuffer::PAINT_WILL_RESAMPLE;
    }
  }
#endif
  if (mDrawAtomically) {
    flags |= RotatedContentBuffer::PAINT_NO_ROTATION;
  }
  PaintState state =
    mContentClient->BeginPaintBuffer(this, flags);
  mValidRegion.Sub(mValidRegion, state.mRegionToInvalidate);

  if (DrawTarget* target = mContentClient->BorrowDrawTargetForPainting(state)) {
    
    
    
    
    state.mRegionToInvalidate.And(state.mRegionToInvalidate,
                                  GetEffectiveVisibleRegion());
    SetAntialiasingFlags(this, target);

    RenderTraceInvalidateStart(this, "FFFF00", state.mRegionToDraw.GetBounds());

    nsRefPtr<gfxContext> ctx = gfxContext::ContextForDrawTarget(target);
    PaintBuffer(ctx,
                state.mRegionToDraw, state.mRegionToDraw, state.mRegionToInvalidate,
                state.mDidSelfCopy,
                state.mClip,
                aCallback, aCallbackData);
    MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) PaintThebes", this));
    Mutated();
    ctx = nullptr;
    mContentClient->ReturnDrawTargetToBuffer(target);

    RenderTraceInvalidateEnd(this, "FFFF00");
  } else {
    
    
    
    NS_WARN_IF_FALSE(state.mRegionToDraw.IsEmpty(),
                     "No context when we have something to draw, resource exhaustion?");
  }
  
  for (uint32_t i = 0; i < readbackUpdates.Length(); ++i) {
    ReadbackProcessor::Update& update = readbackUpdates[i];
    nsIntPoint offset = update.mLayer->GetBackgroundLayerOffset();
    nsRefPtr<gfxContext> ctx =
      update.mLayer->GetSink()->BeginUpdate(update.mUpdateRect + offset,
                                            update.mSequenceCounter);
    if (ctx) {
      NS_ASSERTION(GetEffectiveOpacity() == 1.0, "Should only read back opaque layers");
      NS_ASSERTION(!GetMaskLayer(), "Should only read back layers without masks");
      ctx->SetMatrix(ctx->CurrentMatrix().Translate(offset.x, offset.y));
      mContentClient->DrawTo(this, ctx->GetDrawTarget(), 1.0,
                             CompositionOpForOp(ctx->CurrentOperator()),
                             nullptr, nullptr);
      update.mLayer->GetSink()->EndUpdate(ctx, update.mUpdateRect + offset);
    }
  }
}

already_AddRefed<ThebesLayer>
BasicLayerManager::CreateThebesLayer()
{
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  nsRefPtr<ThebesLayer> layer = new BasicThebesLayer(this);
  return layer.forget();
}

}
}
