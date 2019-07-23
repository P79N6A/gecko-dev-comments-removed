



































#ifndef __NS_SVGFILTERSELEMENT_H__
#define __NS_SVGFILTERSELEMENT_H__

#include "nsSVGStylableElement.h"
#include "nsSVGLength2.h"
#include "nsIFrame.h"
#include "gfxRect.h"
#include "gfxImageSurface.h"
#include "nsIDOMSVGFilters.h"

class nsSVGFilterResource;
class nsSVGString;
class nsSVGFilterInstance;

struct nsSVGStringInfo {
  nsSVGStringInfo(const nsSVGString* aString,
                  nsSVGElement *aElement) :
    mString(aString), mElement(aElement) {}

  const nsSVGString* mString;
  nsSVGElement* mElement;
};

typedef nsSVGStylableElement nsSVGFEBase;

#define NS_SVG_FE_CID \
{ 0x60483958, 0xd229, 0x4a77, \
  { 0x96, 0xb2, 0x62, 0x3e, 0x69, 0x95, 0x1e, 0x0e } }

class nsSVGFE : public nsSVGFEBase

{
  friend class nsSVGFilterInstance;

public:
  class ColorModel {
  public:
    enum ColorSpace { SRGB, LINEAR_RGB };
    enum AlphaChannel { UNPREMULTIPLIED, PREMULTIPLIED };

    ColorModel(ColorSpace aColorSpace, AlphaChannel aAlphaChannel) :
      mColorSpace(aColorSpace), mAlphaChannel(aAlphaChannel) {}
    ColorModel() :
      mColorSpace(SRGB), mAlphaChannel(PREMULTIPLIED) {}
    PRBool operator==(const ColorModel& aOther) const {
      return mColorSpace == aOther.mColorSpace &&
             mAlphaChannel == aOther.mAlphaChannel;
    }
    ColorSpace   mColorSpace;
    AlphaChannel mAlphaChannel;
  };

  struct Image {
    
    nsRefPtr<gfxImageSurface> mImage;
    
    gfxRect                   mFilterPrimitiveSubregion;
    ColorModel                mColorModel;
    
    PRPackedBool              mConstantColorChannels;
    
    Image() : mConstantColorChannels(PR_FALSE) {}
  };

protected:
  nsSVGFE(nsINodeInfo *aNodeInfo) : nsSVGFEBase(aNodeInfo) {}

  struct ScaleInfo {
    nsRefPtr<gfxImageSurface> mRealTarget;
    nsRefPtr<gfxImageSurface> mSource;
    nsRefPtr<gfxImageSurface> mTarget;
    nsIntRect mDataRect; 
    PRPackedBool mRescaling;
  };

  ScaleInfo SetupScalingFilter(nsSVGFilterInstance *aInstance,
                               const Image *aSource,
                               const Image *aTarget,
                               const nsIntRect& aDataRect,
                               nsSVGNumber2 *aUnitX, nsSVGNumber2 *aUnitY,
                               nsSVGElement *aElement);

  void FinishScalingFilter(ScaleInfo *aScaleInfo);

public:
  ColorModel
  GetInputColorModel(nsSVGFilterInstance* aInstance, PRUint32 aInputIndex,
                     Image* aImage) {
    return ColorModel(
          (OperatesOnSRGB(aInstance, aInputIndex, aImage) ?
             ColorModel::SRGB : ColorModel::LINEAR_RGB),
          (OperatesOnPremultipledAlpha() ?
             ColorModel::PREMULTIPLIED : ColorModel::UNPREMULTIPLIED));
  }

  ColorModel
  GetOutputColorModel(nsSVGFilterInstance* aInstance) {
    return ColorModel(
          (OperatesOnSRGB(aInstance, 0, nsnull) ?
             ColorModel::SRGB : ColorModel::LINEAR_RGB),
          (OperatesOnPremultipledAlpha() ?
             ColorModel::PREMULTIPLIED : ColorModel::UNPREMULTIPLIED));
  }

  
  virtual PRBool SubregionIsUnionOfRegions() { return PR_TRUE; }

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_SVG_FE_CID)
  
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGFILTERPRIMITIVESTANDARDATTRIBUTES

  virtual nsSVGString& GetResultImageName() = 0;
  
  
  virtual void GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources);
  
  
  
  
  
  
  virtual nsIntRect ComputeTargetBBox(const nsTArray<nsIntRect>& aSourceBBoxes,
          const nsSVGFilterInstance& aInstance);
  
  
  
  
  
  
  
  virtual void ComputeNeededSourceBBoxes(const nsIntRect& aTargetBBox,
          nsTArray<nsIntRect>& aSourceBBoxes, const nsSVGFilterInstance& aInstance);
  
  
  
  
  
  virtual nsIntRect ComputeChangeBBox(const nsTArray<nsIntRect>& aSourceChangeBoxes,
          const nsSVGFilterInstance& aInstance);
  
  
  
  
  
  
  
  
  
  
  
  virtual nsresult Filter(nsSVGFilterInstance* aInstance,
                          const nsTArray<const Image*>& aSources,
                          const Image* aTarget,
                          const nsIntRect& aDataRect) = 0;

  static nsIntRect GetMaxRect() {
    
    
    return nsIntRect(PR_INT32_MIN/2, PR_INT32_MIN/2, PR_INT32_MAX, PR_INT32_MAX);
  }

  operator nsISupports*() { return static_cast<nsIContent*>(this); }
  
protected:
  virtual PRBool OperatesOnPremultipledAlpha() { return PR_TRUE; }

  
  
  
  virtual PRBool OperatesOnSRGB(nsSVGFilterInstance* aInstance,
                                PRUint32 aInputIndex, Image* aImage) {
    nsIFrame* frame = GetPrimaryFrame();
    if (!frame) return PR_FALSE;

    nsStyleContext* style = frame->GetStyleContext();
    return style->GetStyleSVG()->mColorInterpolationFilters ==
             NS_STYLE_COLOR_INTERPOLATION_SRGB;
  }

  
  virtual LengthAttributesInfo GetLengthInfo();
  virtual void DidAnimateLength(PRUint8 aAttrEnum);
  virtual void DidAnimateNumber(PRUint8 aAttrEnum);
  virtual void DidAnimateEnum(PRUint8 aAttrEnum);
  virtual void DidAnimateBoolean(PRUint8 aAttrEnum);

  
  enum { X, Y, WIDTH, HEIGHT };
  nsSVGLength2 mLengthAttributes[4];
  static LengthInfo sLengthInfo[4];
};

#endif
