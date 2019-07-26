




#ifndef mozilla_dom_SVGFESpotLightElement_h
#define mozilla_dom_SVGFESpotLightElement_h

#include "nsSVGFilters.h"
#include "nsSVGNumber2.h"

class nsSVGFELightingElement;

nsresult NS_NewSVGFESpotLightElement(nsIContent **aResult,
                                     already_AddRefed<nsINodeInfo> aNodeInfo);

namespace mozilla {
namespace dom {

typedef SVGFEUnstyledElement SVGFESpotLightElementBase;

class SVGFESpotLightElement : public SVGFESpotLightElementBase
{
  friend nsresult (::NS_NewSVGFESpotLightElement(nsIContent **aResult,
                                                 already_AddRefed<nsINodeInfo> aNodeInfo));
  friend class ::nsSVGFELightingElement;
protected:
  SVGFESpotLightElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : SVGFESpotLightElementBase(aNodeInfo)
  {
  }
  virtual JSObject* WrapNode(JSContext* aCx, JSObject* aScope) MOZ_OVERRIDE;

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  
  already_AddRefed<nsIDOMSVGAnimatedNumber> X();
  already_AddRefed<nsIDOMSVGAnimatedNumber> Y();
  already_AddRefed<nsIDOMSVGAnimatedNumber> Z();
  already_AddRefed<nsIDOMSVGAnimatedNumber> PointsAtX();
  already_AddRefed<nsIDOMSVGAnimatedNumber> PointsAtY();
  already_AddRefed<nsIDOMSVGAnimatedNumber> PointsAtZ();
  already_AddRefed<nsIDOMSVGAnimatedNumber> SpecularExponent();
  already_AddRefed<nsIDOMSVGAnimatedNumber> LimitingConeAngle();

protected:
  virtual NumberAttributesInfo GetNumberInfo();

  enum { ATTR_X, ATTR_Y, ATTR_Z, POINTS_AT_X, POINTS_AT_Y, POINTS_AT_Z,
         SPECULAR_EXPONENT, LIMITING_CONE_ANGLE };
  nsSVGNumber2 mNumberAttributes[8];
  static NumberInfo sNumberInfo[8];
};

} 
} 

#endif 
