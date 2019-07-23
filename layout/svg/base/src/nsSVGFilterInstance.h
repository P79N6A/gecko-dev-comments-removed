



































#ifndef __NS_SVGFILTERINSTANCE_H__
#define __NS_SVGFILTERINSTANCE_H__

#include "nsIDOMSVGLength.h"
#include "nsIDOMSVGFilters.h"
#include "nsRect.h"
#include "nsIContent.h"
#include "nsAutoPtr.h"
#include "nsSVGFilters.h"
#include "nsISVGChildFrame.h"
#include "nsSVGString.h"

#include "gfxImageSurface.h"

class nsSVGLength2;
class nsSVGElement;
class nsSVGFilterElement;
class nsSVGFilterPaintCallback;
struct gfxRect;








class NS_STACK_CLASS nsSVGFilterInstance
{
public:
  float GetPrimitiveLength(nsSVGLength2 *aLength) const;

  nsSVGFilterInstance(nsIFrame *aTargetFrame,
                      nsSVGFilterPaintCallback *aPaintCallback,
                      nsSVGFilterElement *aFilterElement,
                      const gfxRect &aTargetBBox,
                      const gfxRect& aFilterRect,
                      const nsIntSize& aFilterSpaceSize,
                      const gfxMatrix &aFilterSpaceToDeviceSpaceTransform,
                      const nsIntRect& aDirtyOutputRect,
                      const nsIntRect& aDirtyInputRect,
                      PRUint16 aPrimitiveUnits) :
    mTargetFrame(aTargetFrame),
    mPaintCallback(aPaintCallback),
    mFilterElement(aFilterElement),
    mTargetBBox(aTargetBBox),
    mFilterSpaceToDeviceSpaceTransform(aFilterSpaceToDeviceSpaceTransform),
    mFilterRect(aFilterRect),
    mFilterSpaceSize(aFilterSpaceSize),
    mDirtyOutputRect(aDirtyOutputRect),
    mDirtyInputRect(aDirtyInputRect),
    mSurfaceRect(nsIntPoint(0, 0), aFilterSpaceSize),
    mPrimitiveUnits(aPrimitiveUnits) {
  }
  
  
  void SetSurfaceRect(const nsIntRect& aRect) { mSurfaceRect = aRect; }

  gfxRect GetFilterRect() const { return mFilterRect; }

  const nsIntSize& GetFilterSpaceSize() { return mFilterSpaceSize; }
  PRUint32 GetFilterResX() const { return mFilterSpaceSize.width; }
  PRUint32 GetFilterResY() const { return mFilterSpaceSize.height; }
  
  const nsIntRect& GetSurfaceRect() const { return mSurfaceRect; }
  PRInt32 GetSurfaceWidth() const { return mSurfaceRect.width; }
  PRInt32 GetSurfaceHeight() const { return mSurfaceRect.height; }
  
  nsresult Render(gfxASurface** aOutput);
  nsresult ComputeOutputDirtyRect(nsIntRect* aDirty);
  nsresult ComputeSourceNeededRect(nsIntRect* aDirty);
  nsresult ComputeOutputBBox(nsIntRect* aBBox);

  gfxMatrix GetUserSpaceToFilterSpaceTransform() const;
  gfxMatrix GetFilterSpaceToDeviceSpaceTransform() const {
    return mFilterSpaceToDeviceSpaceTransform;
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

    PrimitiveInfo() : mFE(nsnull), mImageUsers(0) {}
  };

  class ImageAnalysisEntry : public nsStringHashKey {
  public:
    ImageAnalysisEntry(KeyTypePointer aStr) : nsStringHashKey(aStr) { }
    ImageAnalysisEntry(const ImageAnalysisEntry& toCopy) : nsStringHashKey(toCopy),
      mInfo(toCopy.mInfo) { }

    PrimitiveInfo* mInfo;
  };

  nsresult BuildSources();
  
  nsresult BuildPrimitives();
  
  void ComputeResultBoundingBoxes();
  
  
  void ComputeNeededBoxes();
  
  
  void ComputeResultChangeBoxes();
  nsIntRect ComputeUnionOfAllNeededBoxes();
  nsresult BuildSourceImages();

  
  
  
  
  already_AddRefed<gfxImageSurface> CreateImage();

  void ComputeFilterPrimitiveSubregion(PrimitiveInfo* aInfo);
  void EnsureColorModel(PrimitiveInfo* aPrimitive,
                        ColorModel aColorModel);

  gfxRect UserSpaceToFilterSpace(const gfxRect& aUserSpace) const;
  void ClipToFilterSpace(nsIntRect* aRect) const
  {
    nsIntRect filterSpace(nsIntPoint(0, 0), mFilterSpaceSize);
    aRect->IntersectRect(*aRect, filterSpace);
  }
  void ClipToGfxRect(nsIntRect* aRect, const gfxRect& aGfx) const;

  nsIFrame*               mTargetFrame;
  nsSVGFilterPaintCallback* mPaintCallback;
  nsSVGFilterElement*     mFilterElement;
  
  gfxRect                 mTargetBBox;
  gfxMatrix               mFilterSpaceToDeviceSpaceTransform;
  gfxRect                 mFilterRect;
  nsIntSize               mFilterSpaceSize;
  nsIntRect               mDirtyOutputRect;
  nsIntRect               mDirtyInputRect;
  nsIntRect               mSurfaceRect;
  PRUint16                mPrimitiveUnits;

  PrimitiveInfo           mSourceColorAlpha;
  PrimitiveInfo           mSourceAlpha;
  nsTArray<PrimitiveInfo> mPrimitives;
};

#endif
