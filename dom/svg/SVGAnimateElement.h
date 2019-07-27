




#ifndef mozilla_dom_SVGAnimateElement_h
#define mozilla_dom_SVGAnimateElement_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/SVGAnimationElement.h"
#include "nsSMILAnimationFunction.h"

nsresult NS_NewSVGAnimateElement(nsIContent **aResult,
                                 already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo);

namespace mozilla {
namespace dom {

class SVGAnimateElement MOZ_FINAL : public SVGAnimationElement
{
protected:
  explicit SVGAnimateElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);

  nsSMILAnimationFunction mAnimationFunction;
  friend nsresult
    (::NS_NewSVGAnimateElement(nsIContent **aResult,
                               already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo));

  virtual JSObject* WrapNode(JSContext *aCx) MOZ_OVERRIDE;

public:
  
  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  
  virtual nsSMILAnimationFunction& AnimationFunction();
};

} 
} 

#endif 
