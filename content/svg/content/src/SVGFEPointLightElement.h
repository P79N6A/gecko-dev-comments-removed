




#ifndef mozilla_dom_SVGFEPointLightElement_h
#define mozilla_dom_SVGFEPointLightElement_h

#include "nsSVGFilters.h"
#include "nsSVGNumber2.h"

typedef SVGFEUnstyledElement SVGFEPointLightElementBase;

nsresult NS_NewSVGFEPointLightElement(nsIContent **aResult,
                                      already_AddRefed<nsINodeInfo> aNodeInfo);

namespace mozilla {
namespace dom {

class SVGFEPointLightElement : public SVGFEPointLightElementBase
{
  friend nsresult (::NS_NewSVGFEPointLightElement(nsIContent **aResult,
                                                  already_AddRefed<nsINodeInfo> aNodeInfo));
protected:
  SVGFEPointLightElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : SVGFEPointLightElementBase(aNodeInfo)
  {
  }
  virtual JSObject* WrapNode(JSContext *cx, JSObject *scope) MOZ_OVERRIDE;

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  
  already_AddRefed<nsIDOMSVGAnimatedNumber> X();
  already_AddRefed<nsIDOMSVGAnimatedNumber> Y();
  already_AddRefed<nsIDOMSVGAnimatedNumber> Z();

protected:
  virtual NumberAttributesInfo GetNumberInfo();

  enum { ATTR_X, ATTR_Y, ATTR_Z };
  nsSVGNumber2 mNumberAttributes[3];
  static NumberInfo sNumberInfo[3];
};

} 
} 

#endif 
