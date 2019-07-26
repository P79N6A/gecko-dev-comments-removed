




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
class nsSVGFilterFrame;
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
                      nsSVGFilterFrame *aFilterFrame,
                      nsSVGFilterPaintCallback *aPaintCallback,
                      const nsRect *aPostFilterDirtyRect = nullptr,
                      const nsRect *aPreFilterDirtyRect = nullptr,
                      const nsRect *aOverridePreFilterVisualOverflowRect = nullptr,
                      const gfxRect *aOverrideBBox = nullptr,
                      nsIFrame* aTransformRoot = nullptr);

  


  bool IsInitialized() const { return mInitialized; }

  





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

  int32_t AppUnitsPerCSSPixel() const { return mAppUnitsPerCSSPx; }

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

  



  void GetInputsAreTainted(const nsTArray<int32_t>& aInputIndices,
                           nsTArray<bool>& aOutInputsAreTainted);

  



  float GetPrimitiveNumber(uint8_t aCtxType, float aValue) const;

  gfxRect UserSpaceToFilterSpace(const gfxRect& aUserSpace) const;

  





  nsIntRect FrameSpaceToFilterSpace(const nsRect* aRect) const;

  




  gfxMatrix GetUserSpaceToFrameSpaceInCSSPxTransform() const;

  


  nsIFrame*               mTargetFrame;

  nsSVGFilterPaintCallback* mPaintCallback;

  


  const mozilla::dom::SVGFilterElement* mFilterElement;

  


  gfxRect                 mTargetBBox;

  


  gfxMatrix               mFilterSpaceToDeviceSpaceTransform;

  


  gfxMatrix               mFilterSpaceToFrameSpaceInCSSPxTransform;
  gfxMatrix               mFrameSpaceInCSSPxToFilterSpaceTransform;

  


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
  nsTArray<mozilla::RefPtr<SourceSurface>> mInputImages;
  nsTArray<FilterPrimitiveDescription> mPrimitiveDescriptions;
  int32_t                 mAppUnitsPerCSSPx;
  bool                    mInitialized;
};

#endif
