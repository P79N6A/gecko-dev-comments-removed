




































#include "ReadbackProcessor.h"

namespace mozilla {
namespace layers {

void
ReadbackProcessor::BuildUpdates(ContainerLayer* aContainer)
{
  NS_ASSERTION(mAllUpdates.IsEmpty(), "Some updates not processed?");

  if (!aContainer->mMayHaveReadbackChild)
    return;

  aContainer->mMayHaveReadbackChild = false;
  
  
  for (Layer* l = aContainer->GetLastChild(); l; l = l->GetPrevSibling()) {
    if (l->GetType() == Layer::TYPE_READBACK) {
      aContainer->mMayHaveReadbackChild = true;
      BuildUpdatesForLayer(static_cast<ReadbackLayer*>(l));
    }
  }
}

static Layer*
FindBackgroundLayer(ReadbackLayer* aLayer, nsIntPoint* aOffset)
{
  gfxMatrix transform;
  if (!aLayer->GetTransform().Is2D(&transform) ||
      transform.HasNonIntegerTranslation())
    return nsnull;
  nsIntPoint transformOffset(PRInt32(transform.x0), PRInt32(transform.y0));

  for (Layer* l = aLayer->GetPrevSibling(); l; l = l->GetPrevSibling()) {
    gfxMatrix backgroundTransform;
    if (!l->GetTransform().Is2D(&backgroundTransform) ||
        backgroundTransform.HasNonIntegerTranslation())
      return nsnull;

    nsIntPoint backgroundOffset(PRInt32(backgroundTransform.x0), PRInt32(backgroundTransform.y0));
    nsIntRect rectInBackground(transformOffset - backgroundOffset, aLayer->GetSize());
    const nsIntRegion& visibleRegion = l->GetEffectiveVisibleRegion();
    if (!visibleRegion.Intersects(rectInBackground))
      continue;
    
    
    if (!visibleRegion.Contains(rectInBackground))
      return nsnull;

    if (l->GetEffectiveOpacity() != 1.0 ||
        !(l->GetContentFlags() & Layer::CONTENT_OPAQUE))
      return nsnull;

    
    const nsIntRect* clipRect = l->GetEffectiveClipRect();
    if (clipRect && !clipRect->Contains(nsIntRect(transformOffset, aLayer->GetSize())))
      return nsnull;

    Layer::LayerType type = l->GetType();
    if (type != Layer::TYPE_COLOR && type != Layer::TYPE_THEBES)
      return nsnull;

    *aOffset = backgroundOffset - transformOffset;
    return l;
  }

  return nsnull;
}

void
ReadbackProcessor::BuildUpdatesForLayer(ReadbackLayer* aLayer)
{
  if (!aLayer->mSink)
    return;

  nsIntPoint offset;
  Layer* newBackground = FindBackgroundLayer(aLayer, &offset);
  if (!newBackground) {
    aLayer->SetUnknown();
    return;
  }

  if (newBackground->GetType() == Layer::TYPE_COLOR) {
    ColorLayer* colorLayer = static_cast<ColorLayer*>(newBackground);
    if (aLayer->mBackgroundColor != colorLayer->GetColor()) {
      aLayer->mBackgroundLayer = nsnull;
      aLayer->mBackgroundColor = colorLayer->GetColor();
      NS_ASSERTION(aLayer->mBackgroundColor.a == 1.0,
                   "Color layer said it was opaque!");
      nsRefPtr<gfxContext> ctx =
          aLayer->mSink->BeginUpdate(aLayer->GetRect(),
                                     aLayer->AllocateSequenceNumber());
      if (ctx) {
        ctx->SetColor(aLayer->mBackgroundColor);
        nsIntSize size = aLayer->GetSize();
        ctx->Rectangle(gfxRect(0, 0, size.width, size.height));
        ctx->Fill();
        aLayer->mSink->EndUpdate(ctx, aLayer->GetRect());
      }
    }
  } else {
    NS_ASSERTION(newBackground->AsThebesLayer(), "Must be ThebesLayer");
    ThebesLayer* thebesLayer = static_cast<ThebesLayer*>(newBackground);
    
    nsIntRect updateRect = aLayer->GetRect() - offset;
    if (thebesLayer != aLayer->mBackgroundLayer ||
        offset != aLayer->mBackgroundLayerOffset) {
      aLayer->mBackgroundLayer = thebesLayer;
      aLayer->mBackgroundLayerOffset = offset;
      aLayer->mBackgroundColor = gfxRGBA(0,0,0,0);
      thebesLayer->SetUsedForReadback(true);
    } else {
      nsIntRegion invalid;
      invalid.Sub(updateRect, thebesLayer->GetValidRegion());
      updateRect = invalid.GetBounds();
    }

    Update update = { aLayer, updateRect, aLayer->AllocateSequenceNumber() };
    mAllUpdates.AppendElement(update);
  }
}

void
ReadbackProcessor::GetThebesLayerUpdates(ThebesLayer* aLayer,
                                         nsTArray<Update>* aUpdates,
                                         nsIntRegion* aUpdateRegion)
{
  
  
  aLayer->SetUsedForReadback(false);
  if (aUpdateRegion) {
    aUpdateRegion->SetEmpty();
  }
  for (PRUint32 i = mAllUpdates.Length(); i > 0; --i) {
    const Update& update = mAllUpdates[i - 1];
    if (update.mLayer->mBackgroundLayer == aLayer) {
      aLayer->SetUsedForReadback(true);
      
      if (!update.mUpdateRect.IsEmpty()) {
        aUpdates->AppendElement(update);
        if (aUpdateRegion) {
          aUpdateRegion->Or(*aUpdateRegion, update.mUpdateRect);
        }
      }
      mAllUpdates.RemoveElementAt(i - 1);
    }
  }
}

ReadbackProcessor::~ReadbackProcessor()
{
  for (PRUint32 i = mAllUpdates.Length(); i > 0; --i) {
    const Update& update = mAllUpdates[i - 1];
    
    
    update.mLayer->SetUnknown();
  }
}

}
}
