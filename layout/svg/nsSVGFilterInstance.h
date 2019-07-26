




#ifndef __NS_SVGFILTERINSTANCE_H__
#define __NS_SVGFILTERINSTANCE_H__

#include "gfxMatrix.h"
#include "gfxPoint.h"
#include "gfxRect.h"
#include "nsCOMPtr.h"
#include "nsHashKeys.h"
#include "nsPoint.h"
#include "nsRect.h"
#include "nsSize.h"
#include "nsSVGFilters.h"
#include "nsSVGNumber2.h"
#include "nsSVGNumberPair.h"
#include "nsTArray.h"
#include "nsIFrame.h"
#include "mozilla/gfx/2D.h"

class gfxASurface;
class gfxImageSurface;
class nsIFrame;
class nsSVGFilterPaintCallback;

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
  typedef mozilla::gfx::DrawTarget DrawTarget;
  typedef mozilla::gfx::FilterPrimitiveDescription FilterPrimitiveDescription;

public:
  

























  nsSVGFilterInstance(nsIFrame *aTargetFrame,
                      nsSVGFilterPaintCallback *aPaintCallback,
                      const mozilla::dom::SVGFilterElement *aFilterElement,
                      const gfxRect &aTargetBBox,
                      const gfxRect& aFilterRegion,
                      const nsIntSize& aFilterSpaceSize,
                      const gfxMatrix &aFilterSpaceToDeviceSpaceTransform,
                      const gfxMatrix &aFilterSpaceToFrameSpaceInCSSPxTransform,
                      const nsIntRect& aTargetBounds,
                      const nsIntRect& aPostFilterDirtyRect,
                      const nsIntRect& aPreFilterDirtyRect,
                      uint16_t aPrimitiveUnits,
                      nsIFrame* aTransformRoot) :
    mTargetFrame(aTargetFrame),
    mPaintCallback(aPaintCallback),
    mFilterElement(aFilterElement),
    mTargetBBox(aTargetBBox),
    mFilterSpaceToDeviceSpaceTransform(aFilterSpaceToDeviceSpaceTransform),
    mFilterSpaceToFrameSpaceInCSSPxTransform(aFilterSpaceToFrameSpaceInCSSPxTransform),
    mFilterRegion(aFilterRegion),
    mFilterSpaceBounds(nsIntPoint(0, 0), aFilterSpaceSize),
    mTargetBounds(aTargetBounds),
    mPostFilterDirtyRect(aPostFilterDirtyRect),
    mPreFilterDirtyRect(aPreFilterDirtyRect),
    mPrimitiveUnits(aPrimitiveUnits),
    mTransformRoot(aTransformRoot) {
  }

  





  gfxRect GetFilterRegion() const { return mFilterRegion; }

  







  uint32_t GetFilterResX() const { return mFilterSpaceBounds.width; }
  uint32_t GetFilterResY() const { return mFilterSpaceBounds.height; }

  





  nsresult Render(gfxContext* aContext);

  






  nsresult ComputePostFilterDirtyRect(nsIntRect* aPostFilterDirtyRect);

  






  nsresult ComputePostFilterExtents(nsIntRect* aPostFilterExtents);

  






  nsresult ComputeSourceNeededRect(nsIntRect* aDirty);

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

  



  gfxMatrix GetUserSpaceToFilterSpaceTransform() const;

  


  gfxMatrix GetFilterSpaceToDeviceSpaceTransform() const {
    return mFilterSpaceToDeviceSpaceTransform;
  }

  gfxPoint FilterSpaceToUserSpace(const gfxPoint& aPt) const;

  






  gfxMatrix GetFilterSpaceToFrameSpaceInCSSPxTransform() const {
    return mFilterSpaceToFrameSpaceInCSSPxTransform;
  }

  int32_t AppUnitsPerCSSPixel() const {
    return mTargetFrame->PresContext()->AppUnitsPerCSSPixel();
  }

private:
  struct SourceInfo {
    
    
    nsIntRect mNeededBounds;

    
    
    mozilla::RefPtr<SourceSurface> mSourceSurface;

    
    
    IntRect mSurfaceRect;
  };

  



  nsresult BuildSourcePaint(SourceInfo *aPrimitive,
                            gfxASurface* aTargetSurface,
                            DrawTarget* aTargetDT);

  




  nsresult BuildSourcePaints(gfxASurface* aTargetSurface,
                             DrawTarget* aTargetDT);

  



  nsresult BuildSourceImage(gfxASurface* aTargetSurface,
                            DrawTarget* aTargetDT);

  




  nsresult BuildPrimitives();

  




   void ComputeNeededBoxes();

  


  IntRect ComputeFilterPrimitiveSubregion(nsSVGFE* aFilterElement,
                                          const nsTArray<int32_t>& aInputIndices);

  



  float GetPrimitiveNumber(uint8_t aCtxType, float aValue) const;

  gfxRect UserSpaceToFilterSpace(const gfxRect& aUserSpace) const;

  


  nsIFrame*               mTargetFrame;

  nsSVGFilterPaintCallback* mPaintCallback;
  const mozilla::dom::SVGFilterElement* mFilterElement;

  


  gfxRect                 mTargetBBox;

  gfxMatrix               mFilterSpaceToDeviceSpaceTransform;
  gfxMatrix               mFilterSpaceToFrameSpaceInCSSPxTransform;
  gfxRect                 mFilterRegion;
  nsIntRect               mFilterSpaceBounds;

  



  nsIntRect               mTargetBounds;

  




  nsIntRect               mPostFilterDirtyRect;

  





  nsIntRect               mPreFilterDirtyRect;

  


  uint16_t                mPrimitiveUnits;

  SourceInfo              mSourceGraphic;
  SourceInfo              mFillPaint;
  SourceInfo              mStrokePaint;
  nsIFrame*               mTransformRoot;
  nsTArray<nsRefPtr<gfxASurface> > mInputImages;
  nsTArray<FilterPrimitiveDescription> mPrimitiveDescriptions;
};

#endif
