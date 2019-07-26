




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

public:
  





  static nsresult PaintFilteredFrame(nsRenderingContext *aContext,
                                     nsIFrame *aFilteredFrame,
                                     nsSVGFilterPaintCallback *aPaintCallback,
                                     const nsRect* aDirtyArea,
                                     nsIFrame* aTransformRoot = nullptr);

  





  static nsRect GetPostFilterDirtyArea(nsIFrame *aFilteredFrame,
                                       const nsRect& aPreFilterDirtyRect);

  





  static nsRect GetPreFilterNeededArea(nsIFrame *aFilteredFrame,
                                       const nsRect& aPostFilterDirtyRect);

  







  static nsRect GetPostFilterBounds(nsIFrame *aFilteredFrame,
                                    const gfxRect *aOverrideBBox = nullptr,
                                    const nsRect *aPreFilterBounds = nullptr);

  















  nsFilterInstance(nsIFrame *aTargetFrame,
                   nsSVGFilterPaintCallback *aPaintCallback,
                   const nsRect *aPostFilterDirtyRect = nullptr,
                   const nsRect *aPreFilterDirtyRect = nullptr,
                   const nsRect *aOverridePreFilterVisualOverflowRect = nullptr,
                   const gfxRect *aOverrideBBox = nullptr,
                   nsIFrame* aTransformRoot = nullptr);

  


  bool IsInitialized() const { return mInitialized; }

  





  nsresult Render(gfxContext* aContext);

  






  nsresult ComputePostFilterDirtyRect(nsRect* aPostFilterDirtyRect);

  






  nsresult ComputePostFilterExtents(nsRect* aPostFilterExtents);

  






  nsresult ComputeSourceNeededRect(nsRect* aDirty);


  


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
                            gfxASurface* aTargetSurface,
                            DrawTarget* aTargetDT);

  




  nsresult BuildSourcePaints(gfxASurface* aTargetSurface,
                             DrawTarget* aTargetDT);

  



  nsresult BuildSourceImage(gfxASurface* aTargetSurface,
                            DrawTarget* aTargetDT);

  




  nsresult BuildPrimitives();

  




  nsresult BuildPrimitivesForFilter(const nsStyleFilter& aFilter);

  




  void ComputeNeededBoxes();

  


  nsresult ComputeUserSpaceToFilterSpaceScale();

  


  gfxRect UserSpaceToFilterSpace(const gfxRect& aUserSpace) const;
  gfxRect FilterSpaceToUserSpace(const gfxRect& aFilterSpaceRect) const;

  





  nsIntRect FrameSpaceToFilterSpace(const nsRect* aRect) const;
  nsRect FilterSpaceToFrameSpace(const nsIntRect& aRect) const;

  




  gfxMatrix GetUserSpaceToFrameSpaceInCSSPxTransform() const;

  


  nsIFrame*               mTargetFrame;

  nsSVGFilterPaintCallback* mPaintCallback;

  


  gfxRect                 mTargetBBox;

  


  gfxMatrix               mFilterSpaceToDeviceSpaceTransform;

  


  gfxMatrix               mFilterSpaceToFrameSpaceInCSSPxTransform;
  gfxMatrix               mFrameSpaceInCSSPxToFilterSpaceTransform;

  


  gfxRect                 mUserSpaceBounds;
  nsIntRect               mFilterSpaceBounds;

  


  gfxSize                 mUserSpaceToFilterSpaceScale;
  gfxSize                 mFilterSpaceToUserSpaceScale;

  



  nsIntRect               mTargetBounds;

  




  nsIntRect               mPostFilterDirtyRect;

  





  nsIntRect               mPreFilterDirtyRect;

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
