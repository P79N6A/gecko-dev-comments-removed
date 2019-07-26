




#include "mozilla/dom/SVGFETileElement.h"

DOMCI_NODE_DATA(SVGFETileElement, nsSVGFETileElement)

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(FETile)

namespace mozilla {
namespace dom {

nsSVGElement::StringInfo nsSVGFETileElement::sStringInfo[2] =
{
  { &nsGkAtoms::result, kNameSpaceID_None, true },
  { &nsGkAtoms::in, kNameSpaceID_None, true }
};




NS_IMPL_ADDREF_INHERITED(nsSVGFETileElement,nsSVGFETileElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGFETileElement,nsSVGFETileElementBase)

NS_INTERFACE_TABLE_HEAD(nsSVGFETileElement)
  NS_NODE_INTERFACE_TABLE5(nsSVGFETileElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement,
                           nsIDOMSVGFilterPrimitiveStandardAttributes,
                           nsIDOMSVGFETileElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGFETileElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGFETileElementBase)





NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGFETileElement)






NS_IMETHODIMP nsSVGFETileElement::GetIn1(nsIDOMSVGAnimatedString * *aIn)
{
  return mStringAttributes[IN1].ToDOMAnimatedString(aIn, this);
}

void
nsSVGFETileElement::GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources)
{
  aSources.AppendElement(nsSVGStringInfo(&mStringAttributes[IN1], this));
}

nsIntRect
nsSVGFETileElement::ComputeTargetBBox(const nsTArray<nsIntRect>& aSourceBBoxes,
        const nsSVGFilterInstance& aInstance)
{
  return GetMaxRect();
}

void
nsSVGFETileElement::ComputeNeededSourceBBoxes(const nsIntRect& aTargetBBox,
          nsTArray<nsIntRect>& aSourceBBoxes, const nsSVGFilterInstance& aInstance)
{
  
}

nsIntRect
nsSVGFETileElement::ComputeChangeBBox(const nsTArray<nsIntRect>& aSourceChangeBoxes,
                                      const nsSVGFilterInstance& aInstance)
{
  return GetMaxRect();
}

static int32_t WrapInterval(int32_t aVal, int32_t aMax)
{
  aVal = aVal % aMax;
  return aVal < 0 ? aMax + aVal : aVal;
}



































static inline void
ComputePartialTileExtents(int32_t *aLesserSidePartialMatchSize,
                          int32_t *aHigherSidePartialMatchSize,
                          int32_t *aCentreSize,
                          int32_t aLesserTargetExtent,
                          int32_t aTargetSize,
                          int32_t aLesserTileExtent,
                          int32_t aTileSize)
{
  int32_t targetExtentMost = aLesserTargetExtent + aTargetSize;
  int32_t tileExtentMost = aLesserTileExtent + aTileSize;

  int32_t lesserSidePartialMatchSize;
  if (aLesserTileExtent < aLesserTargetExtent) {
    lesserSidePartialMatchSize = tileExtentMost - aLesserTargetExtent;
  } else {
    lesserSidePartialMatchSize = (aLesserTileExtent - aLesserTargetExtent) %
                                    aTileSize;
  }

  int32_t higherSidePartialMatchSize;
  if (lesserSidePartialMatchSize > aTargetSize) {
    lesserSidePartialMatchSize = aTargetSize;
    higherSidePartialMatchSize = 0;
  } else if (tileExtentMost > targetExtentMost) {
      higherSidePartialMatchSize = targetExtentMost - aLesserTileExtent;
  } else {
    higherSidePartialMatchSize = (targetExtentMost - tileExtentMost) %
                                    aTileSize;
  }

  if (lesserSidePartialMatchSize + higherSidePartialMatchSize >
        aTargetSize) {
    higherSidePartialMatchSize = aTargetSize - lesserSidePartialMatchSize;
  }

  



























  int32_t centreSize;
  if (lesserSidePartialMatchSize == aTargetSize ||
        lesserSidePartialMatchSize + higherSidePartialMatchSize ==
        aTargetSize) {
    centreSize = 0;
  } else {
    centreSize = aTargetSize -
                   (lesserSidePartialMatchSize + higherSidePartialMatchSize);
  }

  *aLesserSidePartialMatchSize = lesserSidePartialMatchSize;
  *aHigherSidePartialMatchSize = higherSidePartialMatchSize;
  *aCentreSize = centreSize;
}

static inline void
TilePixels(uint8_t *aTargetData,
           const uint8_t *aSourceData,
           const nsIntRect &targetRegion,
           const nsIntRect &aTile,
           uint32_t aStride)
{
  if (targetRegion.IsEmpty()) {
    return;
  }

  uint32_t tileRowCopyMemSize = aTile.width * 4;
  uint32_t numTimesToCopyTileRows = targetRegion.width / aTile.width;

  uint8_t *targetFirstRowOffset = aTargetData + 4 * targetRegion.x;
  const uint8_t *tileFirstRowOffset = aSourceData + 4 * aTile.x;

  int32_t tileYOffset = 0;
  for (int32_t targetY = targetRegion.y;
       targetY < targetRegion.YMost();
       ++targetY) {
    uint8_t *targetRowOffset = targetFirstRowOffset + aStride * targetY;
    const uint8_t *tileRowOffset = tileFirstRowOffset +
                                             aStride * (aTile.y + tileYOffset);

    for (uint32_t i = 0; i < numTimesToCopyTileRows; ++i) {
      memcpy(targetRowOffset + i * tileRowCopyMemSize,
             tileRowOffset,
             tileRowCopyMemSize);
    }

    tileYOffset = (tileYOffset + 1) % aTile.height;
  }
}

nsresult
nsSVGFETileElement::Filter(nsSVGFilterInstance *instance,
                           const nsTArray<const Image*>& aSources,
                           const Image* aTarget,
                           const nsIntRect& rect)
{
  
  
  
  

  nsIntRect tile;
  bool res = gfxUtils::GfxRectToIntRect(aSources[0]->mFilterPrimitiveSubregion,
                                        &tile);

  NS_ENSURE_TRUE(res, NS_ERROR_FAILURE); 
  if (tile.IsEmpty())
    return NS_OK;

  const nsIntRect &surfaceRect = instance->GetSurfaceRect();
  if (!tile.Intersects(surfaceRect)) {
    
    return NS_OK;
  }

  
  tile = tile.Intersect(surfaceRect);

  
  tile -= surfaceRect.TopLeft();

  uint8_t* sourceData = aSources[0]->mImage->Data();
  uint8_t* targetData = aTarget->mImage->Data();
  uint32_t stride = aTarget->mImage->Stride();

  












  int32_t leftPartialTileWidth;
  int32_t rightPartialTileWidth;
  int32_t centreWidth;
  ComputePartialTileExtents(&leftPartialTileWidth,
                            &rightPartialTileWidth,
                            &centreWidth,
                            rect.x,
                            rect.width,
                            tile.x,
                            tile.width);

  int32_t topPartialTileHeight;
  int32_t bottomPartialTileHeight;
  int32_t centreHeight;
  ComputePartialTileExtents(&topPartialTileHeight,
                            &bottomPartialTileHeight,
                            &centreHeight,
                            rect.y,
                            rect.height,
                            tile.y,
                            tile.height);

  




















  nsIntRect targetRects[] = {
    
    nsIntRect(rect.x, rect.y, leftPartialTileWidth, topPartialTileHeight),
    
    nsIntRect(rect.x + leftPartialTileWidth,
              rect.y,
              centreWidth,
              topPartialTileHeight),
    
    nsIntRect(rect.XMost() - rightPartialTileWidth,
              rect.y,
              rightPartialTileWidth,
              topPartialTileHeight),
    
    nsIntRect(rect.x,
              rect.y + topPartialTileHeight,
              leftPartialTileWidth,
              centreHeight),
    
    nsIntRect(rect.x + leftPartialTileWidth,
              rect.y + topPartialTileHeight,
              centreWidth,
              centreHeight),
    
    nsIntRect(rect.XMost() - rightPartialTileWidth,
              rect.y + topPartialTileHeight,
              rightPartialTileWidth,
              centreHeight),
    
    nsIntRect(rect.x,
              rect.YMost() - bottomPartialTileHeight,
              leftPartialTileWidth,
              bottomPartialTileHeight),
    
    nsIntRect(rect.x + leftPartialTileWidth,
              rect.YMost() - bottomPartialTileHeight,
              centreWidth,
              bottomPartialTileHeight),
    
    nsIntRect(rect.XMost() - rightPartialTileWidth,
              rect.YMost() - bottomPartialTileHeight,
              rightPartialTileWidth,
              bottomPartialTileHeight)
  };

  nsIntRect tileRects[] = {
    
    nsIntRect(tile.XMost() - leftPartialTileWidth,
              tile.YMost() - topPartialTileHeight,
              leftPartialTileWidth,
              topPartialTileHeight),
    
    nsIntRect(tile.x,
              tile.YMost() - topPartialTileHeight,
              tile.width,
              topPartialTileHeight),
    
    nsIntRect(tile.x,
              tile.YMost() - topPartialTileHeight,
              rightPartialTileWidth,
              topPartialTileHeight),
    
    nsIntRect(tile.XMost() - leftPartialTileWidth,
              tile.y,
              leftPartialTileWidth,
              tile.height),
    
    nsIntRect(tile.x,
              tile.y,
              tile.width,
              tile.height),
    
    nsIntRect(tile.x,
              tile.y,
              rightPartialTileWidth,
              tile.height),
    
    nsIntRect(tile.XMost() - leftPartialTileWidth,
              tile.y,
              leftPartialTileWidth,
              bottomPartialTileHeight),
    
    nsIntRect(tile.x,
              tile.y,
              tile.width,
              bottomPartialTileHeight),
    
    nsIntRect(tile.x,
              tile.y,
              rightPartialTileWidth,
              bottomPartialTileHeight)
  };

  for (uint32_t i = 0; i < ArrayLength(targetRects); ++i) {
    TilePixels(targetData,
               sourceData,
               targetRects[i],
               tileRects[i],
               stride);
  }

  return NS_OK;
}

bool
nsSVGFETileElement::AttributeAffectsRendering(int32_t aNameSpaceID,
                                              nsIAtom* aAttribute) const
{
  return nsSVGFETileElementBase::AttributeAffectsRendering(aNameSpaceID, 
                                                           aAttribute) ||
           (aNameSpaceID == kNameSpaceID_None && aAttribute == nsGkAtoms::in);
}




nsSVGElement::StringAttributesInfo
nsSVGFETileElement::GetStringInfo()
{
  return StringAttributesInfo(mStringAttributes, sStringInfo,
                              ArrayLength(sStringInfo));
}

} 
} 
