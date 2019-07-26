




#ifndef __NS_SVGFILTERSELEMENT_H__
#define __NS_SVGFILTERSELEMENT_H__

#include "gfxImageSurface.h"
#include "gfxRect.h"
#include "nsIDOMSVGFilters.h"
#include "nsIFrame.h"
#include "nsImageLoadingContent.h"
#include "nsSVGLength2.h"
#include "nsSVGString.h"
#include "nsSVGElement.h"
#include "SVGAnimatedPreserveAspectRatio.h"

class nsSVGFilterInstance;
class nsSVGFilterResource;
class nsSVGNumberPair;

struct nsSVGStringInfo {
  nsSVGStringInfo(const nsSVGString* aString,
                  nsSVGElement *aElement) :
    mString(aString), mElement(aElement) {}

  const nsSVGString* mString;
  nsSVGElement* mElement;
};

typedef nsSVGElement nsSVGFEBase;

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
    bool operator==(const ColorModel& aOther) const {
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
    
    bool                      mConstantColorChannels;
    
    Image() : mConstantColorChannels(false) {}
  };

protected:
  nsSVGFE(already_AddRefed<nsINodeInfo> aNodeInfo) : nsSVGFEBase(aNodeInfo) {}

  struct ScaleInfo {
    nsRefPtr<gfxImageSurface> mRealTarget;
    nsRefPtr<gfxImageSurface> mSource;
    nsRefPtr<gfxImageSurface> mTarget;
    nsIntRect mDataRect; 
    bool mRescaling;
  };

  ScaleInfo SetupScalingFilter(nsSVGFilterInstance *aInstance,
                               const Image *aSource,
                               const Image *aTarget,
                               const nsIntRect& aDataRect,
                               nsSVGNumberPair *aUnit);

  void FinishScalingFilter(ScaleInfo *aScaleInfo);

public:
  ColorModel
  GetInputColorModel(nsSVGFilterInstance* aInstance, int32_t aInputIndex,
                     Image* aImage) {
    return ColorModel(
          (OperatesOnSRGB(aInstance, aInputIndex, aImage) ?
             ColorModel::SRGB : ColorModel::LINEAR_RGB),
          (OperatesOnPremultipledAlpha(aInputIndex) ?
             ColorModel::PREMULTIPLIED : ColorModel::UNPREMULTIPLIED));
  }

  ColorModel
  GetOutputColorModel(nsSVGFilterInstance* aInstance) {
    return ColorModel(
          (OperatesOnSRGB(aInstance, -1, nullptr) ?
             ColorModel::SRGB : ColorModel::LINEAR_RGB),
          (OperatesOnPremultipledAlpha(-1) ?
             ColorModel::PREMULTIPLIED : ColorModel::UNPREMULTIPLIED));
  }

  
  virtual bool SubregionIsUnionOfRegions() { return true; }

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_SVG_FE_CID)
  
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGFILTERPRIMITIVESTANDARDATTRIBUTES

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  
  virtual bool HasValidDimensions() const;

  bool IsNodeOfType(uint32_t aFlags) const
    { return !(aFlags & ~(eCONTENT | eFILTER)); }

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

  
  
  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const;

  static nsIntRect GetMaxRect() {
    
    
    return nsIntRect(INT32_MIN/2, INT32_MIN/2, INT32_MAX, INT32_MAX);
  }

  operator nsISupports*() { return static_cast<nsIContent*>(this); }

  
  already_AddRefed<mozilla::dom::SVGAnimatedLength> X();
  already_AddRefed<mozilla::dom::SVGAnimatedLength> Y();
  already_AddRefed<mozilla::dom::SVGAnimatedLength> Width();
  already_AddRefed<mozilla::dom::SVGAnimatedLength> Height();
  already_AddRefed<nsIDOMSVGAnimatedString> Result();

protected:
  virtual bool OperatesOnPremultipledAlpha(int32_t) { return true; }

  
  
  
  virtual bool OperatesOnSRGB(nsSVGFilterInstance* aInstance,
                                int32_t aInputIndex, Image* aImage) {
    nsIFrame* frame = GetPrimaryFrame();
    if (!frame) return false;

    nsStyleContext* style = frame->StyleContext();
    return style->StyleSVG()->mColorInterpolationFilters ==
             NS_STYLE_COLOR_INTERPOLATION_SRGB;
  }

  
  virtual LengthAttributesInfo GetLengthInfo();

  
  enum { ATTR_X, ATTR_Y, ATTR_WIDTH, ATTR_HEIGHT };
  nsSVGLength2 mLengthAttributes[4];
  static LengthInfo sLengthInfo[4];
};

typedef nsSVGElement SVGFEUnstyledElementBase;

class SVGFEUnstyledElement : public SVGFEUnstyledElementBase
{
protected:
  SVGFEUnstyledElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : SVGFEUnstyledElementBase(aNodeInfo) {}

public:
  
  
  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const = 0;
};

void
CopyDataRect(uint8_t *aDest, const uint8_t *aSrc, uint32_t aStride,
             const nsIntRect& aDataRect);

inline void
CopyRect(const nsSVGFE::Image* aDest, const nsSVGFE::Image* aSrc, const nsIntRect& aDataRect)
{
  NS_ASSERTION(aDest->mImage->Stride() == aSrc->mImage->Stride(), "stride mismatch");
  NS_ASSERTION(aDest->mImage->GetSize() == aSrc->mImage->GetSize(), "size mismatch");
  NS_ASSERTION(nsIntRect(0, 0, aDest->mImage->Width(), aDest->mImage->Height()).Contains(aDataRect),
               "aDataRect out of bounds");

  CopyDataRect(aDest->mImage->Data(), aSrc->mImage->Data(),
               aSrc->mImage->Stride(), aDataRect);
}

#endif
