





#ifndef mozilla_dom_SVGFEMorphologyElement_h
#define mozilla_dom_SVGFEMorphologyElement_h

#include "nsSVGEnum.h"
#include "nsSVGFilters.h"
#include "nsSVGNumberPair.h"
#include "nsSVGString.h"

nsresult NS_NewSVGFEMorphologyElement(nsIContent **aResult,
                                      already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo);

namespace mozilla {
namespace dom {

typedef nsSVGFE SVGFEMorphologyElementBase;

class SVGFEMorphologyElement : public SVGFEMorphologyElementBase
{
  friend nsresult (::NS_NewSVGFEMorphologyElement(nsIContent **aResult,
                                                  already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo));
protected:
  explicit SVGFEMorphologyElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
    : SVGFEMorphologyElementBase(aNodeInfo)
  {
  }
  virtual JSObject* WrapNode(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

public:
  virtual FilterPrimitiveDescription
    GetPrimitiveDescription(nsSVGFilterInstance* aInstance,
                            const IntRect& aFilterSubregion,
                            const nsTArray<bool>& aInputsAreTainted,
                            nsTArray<mozilla::RefPtr<SourceSurface>>& aInputImages) override;
  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const override;
  virtual nsSVGString& GetResultImageName() override { return mStringAttributes[RESULT]; }
  virtual void GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources) override;

  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const override;

  
  already_AddRefed<SVGAnimatedString> In1();
  already_AddRefed<SVGAnimatedEnumeration> Operator();
  already_AddRefed<SVGAnimatedNumber> RadiusX();
  already_AddRefed<SVGAnimatedNumber> RadiusY();
  void SetRadius(float rx, float ry);

protected:
  void GetRXY(int32_t *aRX, int32_t *aRY, const nsSVGFilterInstance& aInstance);

  virtual NumberPairAttributesInfo GetNumberPairInfo() override;
  virtual EnumAttributesInfo GetEnumInfo() override;
  virtual StringAttributesInfo GetStringInfo() override;

  enum { RADIUS };
  nsSVGNumberPair mNumberPairAttributes[1];
  static NumberPairInfo sNumberPairInfo[1];

  enum { OPERATOR };
  nsSVGEnum mEnumAttributes[1];
  static nsSVGEnumMapping sOperatorMap[];
  static EnumInfo sEnumInfo[1];

  enum { RESULT, IN1 };
  nsSVGString mStringAttributes[2];
  static StringInfo sStringInfo[2];
};

} 
} 

#endif 
