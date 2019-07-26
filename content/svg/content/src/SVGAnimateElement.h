




#ifndef mozilla_dom_SVGAnimateElement_h
#define mozilla_dom_SVGAnimateElement_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/SVGAnimationElement.h"
#include "nsSMILAnimationFunction.h"

nsresult NS_NewSVGAnimateElement(nsIContent **aResult,
                                 already_AddRefed<nsINodeInfo>&& aNodeInfo);

namespace mozilla {
namespace dom {

class SVGAnimateElement MOZ_FINAL : public SVGAnimationElement
{
protected:
  SVGAnimateElement(already_AddRefed<nsINodeInfo>& aNodeInfo);

  nsSMILAnimationFunction mAnimationFunction;
  friend nsresult
    (::NS_NewSVGAnimateElement(nsIContent **aResult,
                               already_AddRefed<nsINodeInfo>&& aNodeInfo));

  virtual JSObject* WrapNode(JSContext *aCx) MOZ_OVERRIDE;

public:
  
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  
  virtual nsSMILAnimationFunction& AnimationFunction();
};

} 
} 

#endif 
