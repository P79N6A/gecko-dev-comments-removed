




#ifndef __NS_FILTERINSTANCE_H__
#define __NS_FILTERINSTANCE_H__

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
class nsIFrame;
class nsSVGFilterPaintCallback;

















class nsFilterInstance
{
  typedef mozilla::gfx::IntRect IntRect;
  typedef mozilla::gfx::SourceSurface SourceSurface;
  typedef mozilla::gfx::DrawTarget DrawTarget;
  typedef mozilla::gfx::FilterPrimitiveDescription FilterPrimitiveDescription;
  typedef mozilla::gfx::FilterDescription FilterDescription;

public:
  





  static nsresult PaintFilteredFrame(nsIFrame *aFilteredFrame,
                                     nsRenderingContext *aContext,
                                     const gfxMatrix& aTransform,
                                     nsSVGFilterPaintCallback *aPaintCallback,
                                     const nsRegion* aDirtyArea);

  





  static nsRegion GetPostFilterDirtyArea(nsIFrame *aFilteredFrame,
                                         const nsRegion& aPreFilterDirtyRegion);

  





  static nsRegion GetPreFilterNeededArea(nsIFrame *aFilteredFrame,
                                         const nsRegion& aPostFilterDirtyRegion);

  







  static nsRect GetPostFilterBounds(nsIFrame *aFilteredFrame,
                                    const gfxRect *aOverrideBBox = nullptr,
                                    const nsRect *aPreFilterBounds = nullptr);

  
















  nsFilterInstance(nsIFrame *aTargetFrame,
                   nsSVGFilterPaintCallback *aPaintCallback,
                   const gfxMatrix& aPaintTransform,
                   const nsRegion *aPostFilterDirtyRegion = nullptr,
                   const nsRegion *aPreFilterDirtyRegion = nullptr,
                   const nsRect *aOverridePreFilterVisualOverflowRect = nullptr,
                   const gfxRect *aOverrideBBox = nullptr);

  


  bool IsInitialized() const { return mInitialized; }

  





  nsresult Render(gfxContext* aContext);

  






  nsRegion ComputePostFilterDirtyRegion();

  






  nsRect ComputePostFilterExtents();

  






  nsRect ComputeSourceNeededRect();


  


  gfxMatrix GetFilterSpaceToDeviceSpaceTransform() const {
    return mFilterSpaceToDeviceSpaceTransform;
  }

private:
  struct SourceInfo {
    
    
    nsIntRect mNeededBounds;

    
    
    mozilla::RefPtr<SourceSurface> mSourceSurface;

    
    
    IntRect mSurfaceRect;
  };

  



  nsresult BuildSourcePaint(SourceInfo *aPrimitive,
                            DrawTarget* aTargetDT);

  




  nsresult BuildSourcePaints(DrawTarget* aTargetDT);

  



  nsresult BuildSourceImage(DrawTarget* aTargetDT);

  




  nsresult BuildPrimitives();

  




  nsresult BuildPrimitivesForFilter(const nsStyleFilter& aFilter);

  




  void ComputeNeededBoxes();

  


  nsIntRect OutputFilterSpaceBounds() const;

  


  nsresult ComputeUserSpaceToFilterSpaceScale();

  


  gfxRect UserSpaceToFilterSpace(const gfxRect& aUserSpace) const;
  gfxRect FilterSpaceToUserSpace(const gfxRect& aFilterSpaceRect) const;

  






  nsIntRect FrameSpaceToFilterSpace(const nsRect* aRect) const;
  nsIntRegion FrameSpaceToFilterSpace(const nsRegion* aRegion) const;

  




  nsRect FilterSpaceToFrameSpace(const nsIntRect& aRect) const;
  nsRegion FilterSpaceToFrameSpace(const nsIntRegion& aRegion) const;

  




  gfxMatrix GetUserSpaceToFrameSpaceInCSSPxTransform() const;

  


  nsIFrame* mTargetFrame;

  nsSVGFilterPaintCallback* mPaintCallback;

  


  gfxRect mTargetBBox;

  


  nsIntRect mTargetBBoxInFilterSpace;

  


  gfxMatrix mFilterSpaceToDeviceSpaceTransform;

  


  gfxMatrix mFilterSpaceToFrameSpaceInCSSPxTransform;
  gfxMatrix mFrameSpaceInCSSPxToFilterSpaceTransform;

  


  gfxSize mUserSpaceToFilterSpaceScale;
  gfxSize mFilterSpaceToUserSpaceScale;

  



  nsIntRect mTargetBounds;

  


  nsIntRegion mPostFilterDirtyRegion;

  


  nsIntRegion mPreFilterDirtyRegion;

  SourceInfo mSourceGraphic;
  SourceInfo mFillPaint;
  SourceInfo mStrokePaint;

  


  gfxMatrix               mPaintTransform;

  nsTArray<mozilla::RefPtr<SourceSurface>> mInputImages;
  nsTArray<FilterPrimitiveDescription> mPrimitiveDescriptions;
  FilterDescription mFilterDescription;
  int32_t mAppUnitsPerCSSPx;
  bool mInitialized;
};

#endif
