




#ifndef mozilla_dom_SVGFEDisplacementMapElement_h
#define mozilla_dom_SVGFEDisplacementMapElement_h

#include "nsSVGEnum.h"
#include "nsSVGFilters.h"

nsresult NS_NewSVGFEDisplacementMapElement(nsIContent **aResult,
                                           already_AddRefed<nsINodeInfo> aNodeInfo);

namespace mozilla {
namespace dom {

typedef nsSVGFE SVGFEDisplacementMapElementBase;

class SVGFEDisplacementMapElement : public SVGFEDisplacementMapElementBase,
                                    public nsIDOMSVGElement
{
protected:
  friend nsresult (::NS_NewSVGFEDisplacementMapElement(nsIContent **aResult,
                                                       already_AddRefed<nsINodeInfo> aNodeInfo));
  SVGFEDisplacementMapElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : SVGFEDisplacementMapElementBase(aNodeInfo)
  {
    SetIsDOMBinding();
  }
  virtual JSObject* WrapNode(JSContext* aCx, JSObject* aScope) MOZ_OVERRIDE;

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  virtual nsresult Filter(nsSVGFilterInstance* aInstance,
                          const nsTArray<const Image*>& aSources,
                          const Image* aTarget,
                          const nsIntRect& aDataRect);
  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const;
  virtual nsSVGString& GetResultImageName() { return mStringAttributes[RESULT]; }
  virtual void GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources);
  virtual nsIntRect ComputeTargetBBox(const nsTArray<nsIntRect>& aSourceBBoxes,
          const nsSVGFilterInstance& aInstance);
  virtual void ComputeNeededSourceBBoxes(const nsIntRect& aTargetBBox,
          nsTArray<nsIntRect>& aSourceBBoxes, const nsSVGFilterInstance& aInstance);
  virtual nsIntRect ComputeChangeBBox(const nsTArray<nsIntRect>& aSourceChangeBoxes,
          const nsSVGFilterInstance& aInstance);

  NS_FORWARD_NSIDOMSVGELEMENT(SVGFEDisplacementMapElementBase::)

  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsIDOMNode* AsDOMNode() { return this; }

  
  already_AddRefed<nsIDOMSVGAnimatedString> In1();
  already_AddRefed<nsIDOMSVGAnimatedString> In2();
  already_AddRefed<nsIDOMSVGAnimatedNumber> Scale();
  already_AddRefed<nsIDOMSVGAnimatedEnumeration> XChannelSelector();
  already_AddRefed<nsIDOMSVGAnimatedEnumeration> YChannelSelector();

protected:
  virtual bool OperatesOnSRGB(nsSVGFilterInstance* aInstance,
                              int32_t aInput, Image* aImage) {
    switch (aInput) {
    case 0:
      return aImage->mColorModel.mColorSpace == ColorModel::SRGB;
    case 1:
      return SVGFEDisplacementMapElementBase::OperatesOnSRGB(aInstance,
                                                             aInput, aImage);
    default:
      NS_ERROR("Will not give correct output color model");
      return false;
    }
  }
  virtual bool OperatesOnPremultipledAlpha(int32_t aInput) {
    return !(aInput == 1);
  }

  virtual NumberAttributesInfo GetNumberInfo();
  virtual EnumAttributesInfo GetEnumInfo();
  virtual StringAttributesInfo GetStringInfo();

  enum { SCALE };
  nsSVGNumber2 mNumberAttributes[1];
  static NumberInfo sNumberInfo[1];

  enum { CHANNEL_X, CHANNEL_Y };
  nsSVGEnum mEnumAttributes[2];
  static nsSVGEnumMapping sChannelMap[];
  static EnumInfo sEnumInfo[2];

  enum { RESULT, IN1, IN2 };
  nsSVGString mStringAttributes[3];
  static StringInfo sStringInfo[3];
};

} 
} 

#endif 
