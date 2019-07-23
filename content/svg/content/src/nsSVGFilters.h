



































#ifndef __NS_SVGFILTERSELEMENT_H__
#define __NS_SVGFILTERSELEMENT_H__

#include "nsSVGStylableElement.h"
#include "nsSVGLength2.h"
#include "nsSVGString.h"
#include "nsIFrame.h"

class nsSVGFilterResource;

typedef nsSVGStylableElement nsSVGFEBase;

#define NS_SVG_FE_CID \
{ 0x60483958, 0xd229, 0x4a77, \
  { 0x96, 0xb2, 0x62, 0x3e, 0x69, 0x95, 0x1e, 0x0e } }

class nsSVGFE : public nsSVGFEBase

{
  friend class nsSVGFilterInstance;

protected:
  nsSVGFE(nsINodeInfo *aNodeInfo) : nsSVGFEBase(aNodeInfo) {}

  struct ScaleInfo {
    nsRefPtr<gfxImageSurface> mRealSource;
    nsRefPtr<gfxImageSurface> mRealTarget;
    nsRefPtr<gfxImageSurface> mSource;
    nsRefPtr<gfxImageSurface> mTarget;
    nsRect mRect; 
    PRPackedBool mRescaling;
  };

  nsresult SetupScalingFilter(nsSVGFilterInstance *aInstance,
                              nsSVGFilterResource *aResource,
                              nsSVGString *aIn,
                              nsSVGNumber2 *aUnitX, nsSVGNumber2 *aUnitY,
                              ScaleInfo *aScaleInfo);

  void FinishScalingFilter(nsSVGFilterResource *aResource,
                           ScaleInfo *aScaleInfo);


public:
  nsSVGFilterInstance::ColorModel
  GetColorModel(nsSVGFilterInstance* aInstance, nsSVGString* aIn) {
    return nsSVGFilterInstance::ColorModel (
          (OperatesOnSRGB(aInstance, aIn) ?
             nsSVGFilterInstance::ColorModel::SRGB :
             nsSVGFilterInstance::ColorModel::LINEAR_RGB),
          (OperatesOnPremultipledAlpha() ?
             nsSVGFilterInstance::ColorModel::PREMULTIPLIED :
             nsSVGFilterInstance::ColorModel::UNPREMULTIPLIED));
  }

  
  virtual PRBool SubregionIsUnionOfRegions() { return PR_TRUE; }

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_SVG_FE_CID)
  
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGFILTERPRIMITIVESTANDARDATTRIBUTES

  virtual nsSVGString* GetResultImageName()=0;
  
  
  virtual void GetSourceImageNames(nsTArray<nsSVGString*>* aSources);
  
  
  
  
  
  
  virtual nsRect ComputeTargetBBox(const nsTArray<nsRect>& aSourceBBoxes,
          const nsSVGFilterInstance& aInstance);
  
  
  
  
  
  
  
  virtual void ComputeNeededSourceBBoxes(const nsRect& aTargetBBox,
          nsTArray<nsRect>& aSourceBBoxes, const nsSVGFilterInstance& aInstance);
  
  virtual nsresult Filter(nsSVGFilterInstance* aInstance) = 0;

  static nsRect GetMaxRect() {
    
    
    return nsRect(PR_INT32_MIN/2, PR_INT32_MIN/2, PR_INT32_MAX, PR_INT32_MAX);
  }

  operator nsISupports*() { return static_cast<nsIContent*>(this); }
  
protected:
  virtual PRBool OperatesOnPremultipledAlpha() { return PR_TRUE; }

  virtual PRBool OperatesOnSRGB(nsSVGFilterInstance*,
                                nsSVGString*) {
    nsIFrame* frame = GetPrimaryFrame();
    if (!frame) return PR_FALSE;

    nsStyleContext* style = frame->GetStyleContext();
    return style->GetStyleSVG()->mColorInterpolationFilters ==
             NS_STYLE_COLOR_INTERPOLATION_SRGB;
  }

  
  virtual LengthAttributesInfo GetLengthInfo();

  
  enum { X, Y, WIDTH, HEIGHT };
  nsSVGLength2 mLengthAttributes[4];
  static LengthInfo sLengthInfo[4];
};

#endif
