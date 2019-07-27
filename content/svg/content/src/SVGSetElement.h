




#ifndef mozilla_dom_SVGSetElement_h
#define mozilla_dom_SVGSetElement_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/SVGAnimationElement.h"
#include "nsSMILSetAnimationFunction.h"

nsresult NS_NewSVGSetElement(nsIContent **aResult,
                             already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo);

namespace mozilla {
namespace dom {

class SVGSetElement MOZ_FINAL : public SVGAnimationElement
{
protected:
  explicit SVGSetElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);

  nsSMILSetAnimationFunction mAnimationFunction;

  friend nsresult (::NS_NewSVGSetElement(nsIContent **aResult,
                                         already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo));

  virtual JSObject* WrapNode(JSContext *aCx) MOZ_OVERRIDE;

public:
  
  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  
  virtual nsSMILAnimationFunction& AnimationFunction();
};

} 
} 

#endif 
