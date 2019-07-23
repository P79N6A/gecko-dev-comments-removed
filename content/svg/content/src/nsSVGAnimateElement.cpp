




































#include "nsSVGAnimationElement.h"
#include "nsIDOMSVGAnimateElement.h"
#include "nsSMILAnimationFunction.h"

typedef nsSVGAnimationElement nsSVGAnimateElementBase;

class nsSVGAnimateElement : public nsSVGAnimateElementBase,
                            public nsIDOMSVGAnimateElement
{
protected:
  friend nsresult NS_NewSVGAnimateElement(nsIContent **aResult,
                                          nsINodeInfo *aNodeInfo);
  nsSVGAnimateElement(nsINodeInfo* aNodeInfo);

  nsSMILAnimationFunction mAnimationFunction;

public:
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGANIMATEELEMENT

  NS_FORWARD_NSIDOMNODE(nsSVGAnimateElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGAnimateElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGAnimateElementBase::)
  NS_FORWARD_NSIDOMSVGANIMATIONELEMENT(nsSVGAnimateElementBase::)
  
  
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  
  virtual nsSMILAnimationFunction& AnimationFunction();
};

NS_IMPL_NS_NEW_SVG_ELEMENT(Animate)




NS_IMPL_ADDREF_INHERITED(nsSVGAnimateElement,nsSVGAnimateElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGAnimateElement,nsSVGAnimateElementBase)

NS_INTERFACE_TABLE_HEAD(nsSVGAnimateElement)
  NS_NODE_INTERFACE_TABLE5(nsSVGAnimateElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGAnimationElement,
                           nsIDOMSVGAnimateElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGAnimateElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGAnimateElementBase)




nsSVGAnimateElement::nsSVGAnimateElement(nsINodeInfo *aNodeInfo)
  : nsSVGAnimateElementBase(aNodeInfo)
{
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGAnimateElement)




nsSMILAnimationFunction&
nsSVGAnimateElement::AnimationFunction()
{
  return mAnimationFunction;
}
