




#ifndef __NS_CSSFILTERINSTANCE_H__
#define __NS_CSSFILTERINSTANCE_H__

#include "FilterSupport.h"
#include "gfxMatrix.h"
#include "gfxRect.h"

struct nsStyleFilter;
template<class T> class nsTArray;






class nsCSSFilterInstance
{
  typedef mozilla::gfx::FilterPrimitiveDescription FilterPrimitiveDescription;
  typedef mozilla::gfx::PrimitiveType PrimitiveType;

public:
  








  nsCSSFilterInstance(const nsStyleFilter& aFilter,
                      const nsIntRect& mTargetBBoxInFilterSpace,
                      const gfxMatrix& aFrameSpaceInCSSPxToFilterSpaceTransform);

  




  nsresult BuildPrimitives(nsTArray<FilterPrimitiveDescription>& aPrimitiveDescrs);

private:
  


  FilterPrimitiveDescription CreatePrimitiveDescription(PrimitiveType aType,
                                                        const nsTArray<FilterPrimitiveDescription>& aPrimitiveDescrs);

  


  nsresult SetAttributesForBlur(FilterPrimitiveDescription& aDescr);

  



  int32_t GetLastResultIndex(const nsTArray<FilterPrimitiveDescription>& aPrimitiveDescrs);

  





  void SetBounds(FilterPrimitiveDescription& aDescr,
                 const nsTArray<FilterPrimitiveDescription>& aPrimitiveDescrs);

  


  const nsStyleFilter& mFilter;

  



  nsIntRect mTargetBBoxInFilterSpace;

  



  gfxMatrix mFrameSpaceInCSSPxToFilterSpaceTransform;
};

#endif
