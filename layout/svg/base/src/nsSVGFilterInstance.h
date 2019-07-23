



































#ifndef __NS_SVGFILTERINSTANCE_H__
#define __NS_SVGFILTERINSTANCE_H__

#include "nsIDOMSVGLength.h"
#include "nsIDOMSVGRect.h"
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








class NS_STACK_CLASS nsSVGFilterInstance
{
public:
  float GetPrimitiveLength(nsSVGLength2 *aLength) const;

  nsSVGFilterInstance(nsISVGChildFrame *aTargetFrame,
                      nsIContent* aFilterElement,
                      nsIDOMSVGRect *aTargetBBox,
                      const gfxRect& aFilterRect,
                      const nsIntSize& aFilterSpaceSize,
                      const nsIntRect& aDirtyRect,
                      PRUint16 aPrimitiveUnits) :
    mTargetFrame(aTargetFrame),
    mFilterElement(aFilterElement),
    mTargetBBox(aTargetBBox),
    mFilterRect(aFilterRect),
    mFilterSpaceSize(aFilterSpaceSize),
    mDirtyRect(aDirtyRect),
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

private:
  typedef nsSVGFE::Image Image;
  typedef nsSVGFE::ColorModel ColorModel;

  struct PrimitiveInfo {
    nsSVGFE*  mFE;
    nsIntRect mResultBoundingBox;
    nsIntRect mResultNeededBox;
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
  nsSVGElement* TargetElement() const
  {
    nsIFrame* f;
    CallQueryInterface(mTargetFrame, &f);
    return static_cast<nsSVGElement*>(f->GetContent());
  }

  nsISVGChildFrame*       mTargetFrame;
  nsIContent*             mFilterElement;
  nsCOMPtr<nsIDOMSVGRect> mTargetBBox;
  gfxRect                 mFilterRect;
  nsIntSize               mFilterSpaceSize;
  nsIntRect               mDirtyRect;
  nsIntRect               mSurfaceRect;
  PRUint16                mPrimitiveUnits;

  PrimitiveInfo           mSourceColorAlpha;
  PrimitiveInfo           mSourceAlpha;
  nsTArray<PrimitiveInfo> mPrimitives;
};

#endif
