




#ifndef __NS_SVGFILTERINSTANCE_H__
#define __NS_SVGFILTERINSTANCE_H__

#include "gfxMatrix.h"
#include "gfxRect.h"
#include "nsSVGFilters.h"
#include "nsSVGNumber2.h"
#include "nsSVGNumberPair.h"
#include "nsTArray.h"

class nsSVGFilterFrame;
struct nsStyleFilter;

namespace mozilla {
namespace dom {
class SVGFilterElement;
}
}










































class nsSVGFilterInstance
{
  typedef mozilla::gfx::Point3D Point3D;
  typedef mozilla::gfx::IntRect IntRect;
  typedef mozilla::gfx::SourceSurface SourceSurface;
  typedef mozilla::gfx::FilterPrimitiveDescription FilterPrimitiveDescription;
  typedef mozilla::dom::UserSpaceMetrics UserSpaceMetrics;

public:
  







  nsSVGFilterInstance(const nsStyleFilter& aFilter,
                      nsIContent* aTargetContent,
                      const UserSpaceMetrics& aMetrics,
                      const gfxRect& aTargetBBox,
                      const gfxSize& aUserSpaceToFilterSpaceScale,
                      const gfxSize& aFilterSpaceToUserSpaceScale);

  


  bool IsInitialized() const { return mInitialized; }

  





  nsresult BuildPrimitives(nsTArray<FilterPrimitiveDescription>& aPrimitiveDescrs,
                           nsTArray<mozilla::RefPtr<SourceSurface>>& aInputImages);

  





  gfxRect GetFilterRegion() const { return mUserSpaceBounds; }

  


  nsIntRect GetFilterSpaceBounds() const { return mFilterSpaceBounds; }

  float GetPrimitiveNumber(uint8_t aCtxType, const nsSVGNumber2 *aNumber) const
  {
    return GetPrimitiveNumber(aCtxType, aNumber->GetAnimValue());
  }
  float GetPrimitiveNumber(uint8_t aCtxType, const nsSVGNumberPair *aNumberPair,
                           nsSVGNumberPair::PairIndex aIndex) const
  {
    return GetPrimitiveNumber(aCtxType, aNumberPair->GetAnimValue(aIndex));
  }

  




  Point3D ConvertLocation(const Point3D& aPoint) const;

  


  gfxRect UserSpaceToFilterSpace(const gfxRect& aUserSpaceRect) const;

private:
  


  nsSVGFilterFrame* GetFilterFrame();

  


  IntRect ComputeFilterPrimitiveSubregion(nsSVGFE* aFilterElement,
                                          const nsTArray<FilterPrimitiveDescription>& aPrimitiveDescrs,
                                          const nsTArray<int32_t>& aInputIndices);

  



  void GetInputsAreTainted(const nsTArray<FilterPrimitiveDescription>& aPrimitiveDescrs,
                           const nsTArray<int32_t>& aInputIndices,
                           nsTArray<bool>& aOutInputsAreTainted);

  



  float GetPrimitiveNumber(uint8_t aCtxType, float aValue) const;

  


  gfxRect FilterSpaceToUserSpace(const gfxRect& aFilterSpaceRect) const;

  




  gfxMatrix GetUserSpaceToFrameSpaceInCSSPxTransform() const;

  







  int32_t GetOrCreateSourceAlphaIndex(nsTArray<FilterPrimitiveDescription>& aPrimitiveDescrs);

  






  nsresult GetSourceIndices(nsSVGFE* aPrimitiveElement,
                            nsTArray<FilterPrimitiveDescription>& aPrimitiveDescrs,
                            const nsDataHashtable<nsStringHashKey, int32_t>& aImageTable,
                            nsTArray<int32_t>& aSourceIndices);

  



  nsresult ComputeBounds();

  


  const nsStyleFilter& mFilter;

  


  nsIContent* mTargetContent;

  


  const UserSpaceMetrics& mMetrics;

  


  const mozilla::dom::SVGFilterElement* mFilterElement;

  


  nsSVGFilterFrame* mFilterFrame;

  


  gfxRect mTargetBBox;

  


  gfxRect mUserSpaceBounds;
  nsIntRect mFilterSpaceBounds;

  


  gfxSize mUserSpaceToFilterSpaceScale;
  gfxSize mFilterSpaceToUserSpaceScale;

  


  uint16_t mPrimitiveUnits;

  




  int32_t mSourceGraphicIndex;

  




  int32_t mSourceAlphaIndex;

  


  int32_t mSourceAlphaAvailable;

  bool mInitialized;
};

#endif
