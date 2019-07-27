




#ifndef __NS_CSSFILTERINSTANCE_H__
#define __NS_CSSFILTERINSTANCE_H__

#include "FilterSupport.h"
#include "gfxMatrix.h"
#include "gfxRect.h"
#include "mozilla/gfx/Point.h"
#include "mozilla/gfx/Types.h"
#include "nsColor.h"

class nsIFrame;
struct nsStyleFilter;
template<class T> class nsTArray;






class nsCSSFilterInstance
{
  typedef mozilla::gfx::Color Color;
  typedef mozilla::gfx::FilterPrimitiveDescription FilterPrimitiveDescription;
  typedef mozilla::gfx::IntPoint IntPoint;
  typedef mozilla::gfx::PrimitiveType PrimitiveType;
  typedef mozilla::gfx::Size Size;

public:
  








  nsCSSFilterInstance(const nsStyleFilter& aFilter,
                      nsIFrame *aTargetFrame,
                      const nsIntRect& mTargetBBoxInFilterSpace,
                      const gfxMatrix& aFrameSpaceInCSSPxToFilterSpaceTransform);

  




  nsresult BuildPrimitives(nsTArray<FilterPrimitiveDescription>& aPrimitiveDescrs);

private:
  


  FilterPrimitiveDescription CreatePrimitiveDescription(PrimitiveType aType,
                                                        const nsTArray<FilterPrimitiveDescription>& aPrimitiveDescrs);

  


  nsresult SetAttributesForBlur(FilterPrimitiveDescription& aDescr);
  nsresult SetAttributesForDropShadow(FilterPrimitiveDescription& aDescr);
  nsresult SetAttributesForGrayscale(FilterPrimitiveDescription& aDescr);
  nsresult SetAttributesForHueRotate(FilterPrimitiveDescription& aDescr);
  nsresult SetAttributesForSaturate(FilterPrimitiveDescription& aDescr);
  nsresult SetAttributesForSepia(FilterPrimitiveDescription& aDescr);

  



  int32_t GetLastResultIndex(const nsTArray<FilterPrimitiveDescription>& aPrimitiveDescrs);

  





  void SetBounds(FilterPrimitiveDescription& aDescr,
                 const nsTArray<FilterPrimitiveDescription>& aPrimitiveDescrs);

  



  Color ToAttributeColor(nscolor aColor);

  


  Size BlurRadiusToFilterSpace(nscoord aRadiusInFrameSpace);

  



  IntPoint OffsetToFilterSpace(nscoord aXOffsetInFrameSpace,
                               nscoord aYOffsetInFrameSpace);

  


  const nsStyleFilter& mFilter;

  


  nsIFrame*               mTargetFrame;

  



  nsIntRect mTargetBBoxInFilterSpace;

  



  gfxMatrix mFrameSpaceInCSSPxToFilterSpaceTransform;
};

#endif
