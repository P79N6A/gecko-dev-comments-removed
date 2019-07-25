




































#include "nsSVGAnimationElement.h"
#include "nsIDOMSVGAnimateElement.h"
#include "nsSMILAnimationFunction.h"

typedef nsSVGAnimationElement nsSVGAnimateElementBase;

class nsSVGAnimateElement : public nsSVGAnimateElementBase,
                            public nsIDOMSVGAnimateElement
{
protected:
  friend nsresult NS_NewSVGAnimateElement(nsIContent **aResult,
                                          already_AddRefed<nsINodeInfo> aNodeInfo);
  nsSVGAnimateElement(already_AddRefed<nsINodeInfo> aNodeInfo);

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

  virtual nsXPCClassInfo* GetClassInfo();
};

NS_IMPL_NS_NEW_SVG_ELEMENT(Animate)




NS_IMPL_ADDREF_INHERITED(nsSVGAnimateElement,nsSVGAnimateElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGAnimateElement,nsSVGAnimateElementBase)

DOMCI_NODE_DATA(SVGAnimateElement, nsSVGAnimateElement)

NS_INTERFACE_TABLE_HEAD(nsSVGAnimateElement)
  NS_NODE_INTERFACE_TABLE6(nsSVGAnimateElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGAnimationElement,
                           nsIDOMSVGTests, nsIDOMSVGAnimateElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGAnimateElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGAnimateElementBase)




nsSVGAnimateElement::nsSVGAnimateElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsSVGAnimateElementBase(aNodeInfo)
{
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGAnimateElement)




nsSMILAnimationFunction&
nsSVGAnimateElement::AnimationFunction()
{
  return mAnimationFunction;
}
