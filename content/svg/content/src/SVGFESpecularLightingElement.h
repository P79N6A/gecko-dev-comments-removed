




#ifndef mozilla_dom_SVGFESpecularLightingElement_h
#define mozilla_dom_SVGFESpecularLightingElement_h

#include "nsSVGFilters.h"

nsresult NS_NewSVGFESpecularLightingElement(nsIContent **aResult,
                                            already_AddRefed<nsINodeInfo> aNodeInfo);

namespace mozilla {
namespace dom {



typedef nsSVGFELightingElement SVGFESpecularLightingElementBase;

class SVGFESpecularLightingElement : public SVGFESpecularLightingElementBase
{
  friend nsresult (::NS_NewSVGFESpecularLightingElement(nsIContent **aResult,
                                                        already_AddRefed<nsINodeInfo> aNodeInfo));
protected:
  SVGFESpecularLightingElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : SVGFESpecularLightingElementBase(aNodeInfo)
  {
  }
  virtual JSObject* WrapNode(JSContext* aCx,
                             JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

public:
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsresult Filter(nsSVGFilterInstance* aInstance,
                          const nsTArray<const Image*>& aSources,
                          const Image* aTarget,
                          const nsIntRect& aDataRect);
  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const;

  
  already_AddRefed<SVGAnimatedString> In1();
  already_AddRefed<SVGAnimatedNumber> SurfaceScale();
  already_AddRefed<SVGAnimatedNumber> SpecularConstant();
  already_AddRefed<SVGAnimatedNumber> SpecularExponent();
  already_AddRefed<SVGAnimatedNumber> KernelUnitLengthX();
  already_AddRefed<SVGAnimatedNumber> KernelUnitLengthY();

protected:
  virtual void LightPixel(const float *N, const float *L,
                          nscolor color, uint8_t *targetData);

};

} 
} 

#endif 
