





#ifndef mozilla_dom_SVGFEPointLightElement_h
#define mozilla_dom_SVGFEPointLightElement_h

#include "nsSVGFilters.h"
#include "nsSVGNumber2.h"

nsresult NS_NewSVGFEPointLightElement(nsIContent **aResult,
                                      already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo);

namespace mozilla {
namespace dom {

typedef SVGFELightElement SVGFEPointLightElementBase;

class SVGFEPointLightElement : public SVGFEPointLightElementBase
{
  friend nsresult (::NS_NewSVGFEPointLightElement(nsIContent **aResult,
                                                  already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo));
protected:
  explicit SVGFEPointLightElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
    : SVGFEPointLightElementBase(aNodeInfo)
  {
  }
  virtual JSObject* WrapNode(JSContext *cx, JS::Handle<JSObject*> aGivenProto) override;

public:
  virtual AttributeMap ComputeLightAttributes(nsSVGFilterInstance* aInstance) override;
  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const override;

  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const override;

  
  already_AddRefed<SVGAnimatedNumber> X();
  already_AddRefed<SVGAnimatedNumber> Y();
  already_AddRefed<SVGAnimatedNumber> Z();

protected:
  virtual NumberAttributesInfo GetNumberInfo() override;

  enum { ATTR_X, ATTR_Y, ATTR_Z };
  nsSVGNumber2 mNumberAttributes[3];
  static NumberInfo sNumberInfo[3];
};

} 
} 

#endif 
