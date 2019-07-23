



































#ifndef __NS_SVGFILTERSELEMENT_H__
#define __NS_SVGFILTERSELEMENT_H__

#include "nsSVGStylableElement.h"
#include "nsSVGLength2.h"
#include "nsIFrame.h"

class nsSVGFilterResource;
class nsIDOMSVGAnimatedString;

typedef nsSVGStylableElement nsSVGFEBase;

class nsSVGFE : public nsSVGFEBase

{
  friend class nsSVGFilterInstance;

protected:
  nsSVGFE(nsINodeInfo *aNodeInfo) : nsSVGFEBase(aNodeInfo) {}
  nsresult Init();

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
                              nsIDOMSVGAnimatedString *aIn,
                              nsSVGNumber2 *aUnitX, nsSVGNumber2 *aUnitY,
                              ScaleInfo *aScaleInfo);

  void FinishScalingFilter(nsSVGFilterResource *aResource,
                           ScaleInfo *aScaleInfo);


public:
  nsSVGFilterInstance::ColorModel
  GetColorModel(nsSVGFilterInstance* aInstance, nsIDOMSVGAnimatedString* aIn) {
    return nsSVGFilterInstance::ColorModel (
          (OperatesOnSRGB(aInstance, aIn) ?
             nsSVGFilterInstance::ColorModel::SRGB :
             nsSVGFilterInstance::ColorModel::LINEAR_RGB),
          (OperatesOnPremultipledAlpha() ?
             nsSVGFilterInstance::ColorModel::PREMULTIPLIED :
             nsSVGFilterInstance::ColorModel::UNPREMULTIPLIED));
  }

  
  virtual PRBool SubregionIsUnionOfRegions() { return PR_TRUE; }

  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGFILTERPRIMITIVESTANDARDATTRIBUTES

protected:
  virtual PRBool OperatesOnPremultipledAlpha() { return PR_TRUE; }

  virtual PRBool OperatesOnSRGB(nsSVGFilterInstance*,
                                nsIDOMSVGAnimatedString*) {
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

  nsCOMPtr<nsIDOMSVGAnimatedString> mResult;
};

#endif
