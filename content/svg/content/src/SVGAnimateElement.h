




#ifndef mozilla_dom_SVGAnimateElement_h
#define mozilla_dom_SVGAnimateElement_h

#include "mozilla/dom/SVGAnimationElement.h"
#include "nsSMILAnimationFunction.h"

nsresult NS_NewSVGAnimateElement(nsIContent **aResult,
                                 already_AddRefed<nsINodeInfo> aNodeInfo);

namespace mozilla {
namespace dom {

class SVGAnimateElement MOZ_FINAL : public SVGAnimationElement,
                                    public nsIDOMSVGElement
{
protected:
  SVGAnimateElement(already_AddRefed<nsINodeInfo> aNodeInfo);

  nsSMILAnimationFunction mAnimationFunction;
  friend nsresult
    (::NS_NewSVGAnimateElement(nsIContent **aResult,
                               already_AddRefed<nsINodeInfo> aNodeInfo));

  virtual JSObject* WrapNode(JSContext *aCx, JSObject *aScope) MOZ_OVERRIDE;

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC
  NS_FORWARD_NSIDOMSVGELEMENT(SVGAnimationElement::)

  
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  
  virtual nsSMILAnimationFunction& AnimationFunction();

  virtual nsIDOMNode* AsDOMNode() { return this; }
};

} 
} 

#endif 
