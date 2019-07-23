







































#include "nsSVGAnimateMotionElement.h"

NS_IMPL_NS_NEW_SVG_ELEMENT(AnimateMotion)




NS_IMPL_ADDREF_INHERITED(nsSVGAnimateMotionElement,nsSVGAnimateMotionElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGAnimateMotionElement,nsSVGAnimateMotionElementBase)

DOMCI_DATA(SVGAnimateMotionElement, nsSVGAnimateMotionElement)

NS_INTERFACE_TABLE_HEAD(nsSVGAnimateMotionElement)
  NS_NODE_INTERFACE_TABLE5(nsSVGAnimateMotionElement, nsIDOMNode,
                           nsIDOMElement, nsIDOMSVGElement,
                           nsIDOMSVGAnimationElement,
                           nsIDOMSVGAnimateMotionElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGAnimateMotionElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGAnimateMotionElementBase)




nsSVGAnimateMotionElement::nsSVGAnimateMotionElement(nsINodeInfo *aNodeInfo)
  : nsSVGAnimateMotionElementBase(aNodeInfo)
{
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGAnimateMotionElement)




nsSMILAnimationFunction&
nsSVGAnimateMotionElement::AnimationFunction()
{
  return mAnimationFunction;
}

nsIAtom*
nsSVGAnimateMotionElement::GetTargetAttributeName() const
{
  
  
  
  return nsGkAtoms::mozAnimateMotionDummyAttr;
}

nsSMILTargetAttrType
nsSVGAnimateMotionElement::GetTargetAttributeType() const
{
  
  
  
  return eSMILTargetAttrType_XML;
}
