





































#include "nsSVGAnimationElement.h"
#include "nsIDOMSVGSetElement.h"
#include "nsSMILSetAnimationFunction.h"

typedef nsSVGAnimationElement nsSVGSetElementBase;

class nsSVGSetElement : public nsSVGSetElementBase,
                        public nsIDOMSVGSetElement
{
protected:
  friend nsresult NS_NewSVGSetElement(nsIContent **aResult,
                                      nsINodeInfo *aNodeInfo);
  nsSVGSetElement(nsINodeInfo* aNodeInfo);

  nsSMILSetAnimationFunction mAnimationFunction;

public:
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGSETELEMENT

  NS_FORWARD_NSIDOMNODE(nsSVGSetElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGSetElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGSetElementBase::)
  NS_FORWARD_NSIDOMSVGANIMATIONELEMENT(nsSVGSetElementBase::)
  
  
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  
  virtual nsSMILAnimationFunction& AnimationFunction();
};

NS_IMPL_NS_NEW_SVG_ELEMENT(Set)




NS_IMPL_ADDREF_INHERITED(nsSVGSetElement,nsSVGSetElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGSetElement,nsSVGSetElementBase)

NS_INTERFACE_TABLE_HEAD(nsSVGSetElement)
  NS_NODE_INTERFACE_TABLE5(nsSVGSetElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGAnimationElement,
                           nsIDOMSVGSetElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGSetElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGSetElementBase)




nsSVGSetElement::nsSVGSetElement(nsINodeInfo *aNodeInfo)
  : nsSVGSetElementBase(aNodeInfo)
{
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGSetElement)




nsSMILAnimationFunction&
nsSVGSetElement::AnimationFunction()
{
  return mAnimationFunction;
}
