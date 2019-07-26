




#ifndef mozilla_dom_SVGSetElement_h
#define mozilla_dom_SVGSetElement_h

#include "mozilla/dom/SVGAnimationElement.h"
#include "nsSMILSetAnimationFunction.h"

nsresult NS_NewSVGSetElement(nsIContent **aResult,
                             already_AddRefed<nsINodeInfo> aNodeInfo);

namespace mozilla {
namespace dom {

class SVGSetElement MOZ_FINAL : public SVGAnimationElement,
                                public nsIDOMSVGElement
{
protected:
  SVGSetElement(already_AddRefed<nsINodeInfo> aNodeInfo);

  nsSMILSetAnimationFunction mAnimationFunction;

  friend nsresult (::NS_NewSVGSetElement(nsIContent **aResult,
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
