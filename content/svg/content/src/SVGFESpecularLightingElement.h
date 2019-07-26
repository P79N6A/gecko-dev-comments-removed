




#ifndef mozilla_dom_SVGFESpecularLightingElement_h
#define mozilla_dom_SVGFESpecularLightingElement_h

#include "nsSVGFilters.h"

nsresult NS_NewSVGFESpecularLightingElement(nsIContent **aResult,
                                            already_AddRefed<nsINodeInfo>&& aNodeInfo);

namespace mozilla {
namespace dom {



typedef nsSVGFELightingElement SVGFESpecularLightingElementBase;

class SVGFESpecularLightingElement : public SVGFESpecularLightingElementBase
{
  friend nsresult (::NS_NewSVGFESpecularLightingElement(nsIContent **aResult,
                                                        already_AddRefed<nsINodeInfo>&& aNodeInfo));
protected:
  SVGFESpecularLightingElement(already_AddRefed<nsINodeInfo>& aNodeInfo)
    : SVGFESpecularLightingElementBase(aNodeInfo)
  {
  }
  virtual JSObject* WrapNode(JSContext* aCx) MOZ_OVERRIDE;

public:
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual FilterPrimitiveDescription
    GetPrimitiveDescription(nsSVGFilterInstance* aInstance,
                            const IntRect& aFilterSubregion,
                            const nsTArray<bool>& aInputsAreTainted,
                            nsTArray<mozilla::RefPtr<SourceSurface>>& aInputImages) MOZ_OVERRIDE;
  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const;

  
  already_AddRefed<SVGAnimatedString> In1();
  already_AddRefed<SVGAnimatedNumber> SurfaceScale();
  already_AddRefed<SVGAnimatedNumber> SpecularConstant();
  already_AddRefed<SVGAnimatedNumber> SpecularExponent();
  already_AddRefed<SVGAnimatedNumber> KernelUnitLengthX();
  already_AddRefed<SVGAnimatedNumber> KernelUnitLengthY();
};

} 
} 

#endif 
