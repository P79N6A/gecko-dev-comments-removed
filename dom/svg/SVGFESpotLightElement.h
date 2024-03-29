





#ifndef mozilla_dom_SVGFESpotLightElement_h
#define mozilla_dom_SVGFESpotLightElement_h

#include "nsSVGFilters.h"
#include "nsSVGNumber2.h"

nsresult NS_NewSVGFESpotLightElement(nsIContent **aResult,
                                     already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo);

namespace mozilla {
namespace dom {

typedef SVGFELightElement SVGFESpotLightElementBase;

class SVGFESpotLightElement : public SVGFESpotLightElementBase
{
  friend nsresult (::NS_NewSVGFESpotLightElement(nsIContent **aResult,
                                                 already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo));
  friend class ::nsSVGFELightingElement;
protected:
  explicit SVGFESpotLightElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
    : SVGFESpotLightElementBase(aNodeInfo)
  {
  }
  virtual JSObject* WrapNode(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

public:
  virtual AttributeMap ComputeLightAttributes(nsSVGFilterInstance* aInstance) override;
  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const override;

  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const override;

  
  already_AddRefed<SVGAnimatedNumber> X();
  already_AddRefed<SVGAnimatedNumber> Y();
  already_AddRefed<SVGAnimatedNumber> Z();
  already_AddRefed<SVGAnimatedNumber> PointsAtX();
  already_AddRefed<SVGAnimatedNumber> PointsAtY();
  already_AddRefed<SVGAnimatedNumber> PointsAtZ();
  already_AddRefed<SVGAnimatedNumber> SpecularExponent();
  already_AddRefed<SVGAnimatedNumber> LimitingConeAngle();

protected:
  virtual NumberAttributesInfo GetNumberInfo() override;

  enum { ATTR_X, ATTR_Y, ATTR_Z, POINTS_AT_X, POINTS_AT_Y, POINTS_AT_Z,
         SPECULAR_EXPONENT, LIMITING_CONE_ANGLE };
  nsSVGNumber2 mNumberAttributes[8];
  static NumberInfo sNumberInfo[8];
};

} 
} 

#endif 
