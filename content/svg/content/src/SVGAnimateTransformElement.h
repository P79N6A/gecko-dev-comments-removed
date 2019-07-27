




#ifndef mozilla_dom_SVGAnimateTransformElement_h
#define mozilla_dom_SVGAnimateTransformElement_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/SVGAnimationElement.h"
#include "nsSMILAnimationFunction.h"

nsresult NS_NewSVGAnimateTransformElement(nsIContent **aResult,
                                          already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo);

namespace mozilla {
namespace dom {

class SVGAnimateTransformElement MOZ_FINAL : public SVGAnimationElement
{
protected:
  explicit SVGAnimateTransformElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);

  nsSMILAnimationFunction mAnimationFunction;
  friend nsresult
    (::NS_NewSVGAnimateTransformElement(nsIContent **aResult,
                                        already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo));

  virtual JSObject* WrapNode(JSContext *aCx) MOZ_OVERRIDE;

public:
  
  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  
  bool ParseAttribute(int32_t aNamespaceID,
                        nsIAtom* aAttribute,
                        const nsAString& aValue,
                        nsAttrValue& aResult) MOZ_OVERRIDE;

  
  virtual nsSMILAnimationFunction& AnimationFunction();
};

} 
} 

#endif 
