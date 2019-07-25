




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

class gfxASurface;
class gfxImageSurface;
class nsIFrame;
class nsSVGFilterElement;
class nsSVGFilterPaintCallback;


















class NS_STACK_CLASS nsSVGFilterInstance
{
public:
  

























  nsSVGFilterInstance(nsIFrame *aTargetFrame,
                      nsSVGFilterPaintCallback *aPaintCallback,
                      const nsSVGFilterElement *aFilterElement,
                      const gfxRect &aTargetBBox,
                      const gfxRect& aFilterRegion,
                      const nsIntSize& aFilterSpaceSize,
                      const gfxMatrix &aFilterSpaceToDeviceSpaceTransform,
                      const gfxMatrix &aFilterSpaceToFrameSpaceInCSSPxTransform,
                      const nsIntRect& aTargetBounds,
                      const nsIntRect& aPostFilterDirtyRect,
                      const nsIntRect& aPreFilterDirtyRect,
                      PRUint16 aPrimitiveUnits) :
    mTargetFrame(aTargetFrame),
    mPaintCallback(aPaintCallback),
    mFilterElement(aFilterElement),
    mTargetBBox(aTargetBBox),
    mFilterSpaceToDeviceSpaceTransform(aFilterSpaceToDeviceSpaceTransform),
    mFilterSpaceToFrameSpaceInCSSPxTransform(aFilterSpaceToFrameSpaceInCSSPxTransform),
    mFilterRegion(aFilterRegion),
    mFilterSpaceSize(aFilterSpaceSize),
    mSurfaceRect(nsIntPoint(0, 0), aFilterSpaceSize),
    mTargetBounds(aTargetBounds),
    mPostFilterDirtyRect(aPostFilterDirtyRect),
    mPreFilterDirtyRect(aPreFilterDirtyRect),
    mPrimitiveUnits(aPrimitiveUnits) {
  }

  





  gfxRect GetFilterRegion() const { return mFilterRegion; }

  







  const nsIntSize& GetFilterSpaceSize() { return mFilterSpaceSize; }
  PRUint32 GetFilterResX() const { return mFilterSpaceSize.width; }
  PRUint32 GetFilterResY() const { return mFilterSpaceSize.height; }

  





  const nsIntRect& GetSurfaceRect() const { return mSurfaceRect; }
  PRInt32 GetSurfaceWidth() const { return mSurfaceRect.width; }
  PRInt32 GetSurfaceHeight() const { return mSurfaceRect.height; }

  






  nsresult Render(gfxASurface** aOutput);

  






  nsresult ComputePostFilterDirtyRect(nsIntRect* aPostFilterDirtyRect);

  






  nsresult ComputeSourceNeededRect(nsIntRect* aDirty);

  




  nsresult ComputeOutputBBox(nsIntRect* aBBox);

  float GetPrimitiveNumber(PRUint8 aCtxType, const nsSVGNumber2 *aNumber) const
  {
    return GetPrimitiveNumber(aCtxType, aNumber->GetAnimValue());
  }
  float GetPrimitiveNumber(PRUint8 aCtxType, const nsSVGNumberPair *aNumberPair,
                           nsSVGNumberPair::PairIndex aIndex) const
  {
    return GetPrimitiveNumber(aCtxType, aNumberPair->GetAnimValue(aIndex));
  }

  




  void ConvertLocation(float aValues[3]) const;

  



  gfxMatrix GetUserSpaceToFilterSpaceTransform() const;

  


  gfxMatrix GetFilterSpaceToDeviceSpaceTransform() const {
    return mFilterSpaceToDeviceSpaceTransform;
  }

  gfxPoint FilterSpaceToUserSpace(const gfxPoint& aPt) const;

  






  gfxMatrix GetFilterSpaceToFrameSpaceInCSSPxTransform() const {
    return mFilterSpaceToFrameSpaceInCSSPxTransform;
  }

  PRInt32 AppUnitsPerCSSPixel() const {
    return mTargetFrame->PresContext()->AppUnitsPerCSSPixel();
  }

private:
  typedef nsSVGFE::Image Image;
  typedef nsSVGFE::ColorModel ColorModel;

  struct PrimitiveInfo {
    
    nsSVGFE*  mFE;

    






    nsIntRect mResultBoundingBox;

    










    nsIntRect mResultNeededBox;

    







    nsIntRect mResultChangeBox;

    Image     mImage;

    






    PRInt32   mImageUsers;
  
    
    
    
    nsTArray<PrimitiveInfo*> mInputs;

    PrimitiveInfo() : mFE(nullptr), mImageUsers(0) {}
  };

  class ImageAnalysisEntry : public nsStringHashKey {
  public:
    ImageAnalysisEntry(KeyTypePointer aStr) : nsStringHashKey(aStr) { }
    ImageAnalysisEntry(const ImageAnalysisEntry& toCopy) : nsStringHashKey(toCopy),
      mInfo(toCopy.mInfo) { }

    PrimitiveInfo* mInfo;
  };

  




  nsresult BuildSources();

  



  nsresult BuildSourcePaint(PrimitiveInfo *aPrimitive);

  




  nsresult BuildSourcePaints();

  





  nsresult BuildSourceImages();

  





  nsresult BuildPrimitives();

  






  void ComputeResultBoundingBoxes();

  




   void ComputeNeededBoxes();

  




  void ComputeResultChangeBoxes();

  




  nsIntRect ComputeUnionOfAllNeededBoxes();

  




  already_AddRefed<gfxImageSurface> CreateImage();

  


  void ComputeFilterPrimitiveSubregion(PrimitiveInfo* aInfo);

  




  void EnsureColorModel(PrimitiveInfo* aPrimitive,
                        ColorModel aColorModel);

  



  float GetPrimitiveNumber(PRUint8 aCtxType, float aValue) const;

  gfxRect UserSpaceToFilterSpace(const gfxRect& aUserSpace) const;

  


  void ClipToFilterSpace(nsIntRect* aRect) const
  {
    nsIntRect filterSpace(nsIntPoint(0, 0), mFilterSpaceSize);
    aRect->IntersectRect(*aRect, filterSpace);
  }

  


  nsIFrame*               mTargetFrame;

  nsSVGFilterPaintCallback* mPaintCallback;
  const nsSVGFilterElement* mFilterElement;

  


  gfxRect                 mTargetBBox;

  gfxMatrix               mFilterSpaceToDeviceSpaceTransform;
  gfxMatrix               mFilterSpaceToFrameSpaceInCSSPxTransform;
  gfxRect                 mFilterRegion;
  nsIntSize               mFilterSpaceSize;
  nsIntRect               mSurfaceRect;

  



  nsIntRect               mTargetBounds;

  




  nsIntRect               mPostFilterDirtyRect;

  





  nsIntRect               mPreFilterDirtyRect;

  


  PRUint16                mPrimitiveUnits;

  PrimitiveInfo           mSourceColorAlpha;
  PrimitiveInfo           mSourceAlpha;
  PrimitiveInfo           mFillPaint;
  PrimitiveInfo           mStrokePaint;
  nsTArray<PrimitiveInfo> mPrimitives;
};

#endif
