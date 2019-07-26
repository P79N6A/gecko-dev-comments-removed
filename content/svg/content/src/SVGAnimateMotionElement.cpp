




#include "mozilla/dom/SVGAnimateMotionElement.h"

DOMCI_NODE_DATA(SVGAnimateMotionElement, mozilla::dom::SVGAnimateMotionElement)

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(AnimateMotion)

namespace mozilla {
namespace dom {




NS_IMPL_ADDREF_INHERITED(SVGAnimateMotionElement, SVGAnimationElement)
NS_IMPL_RELEASE_INHERITED(SVGAnimateMotionElement, SVGAnimationElement)

NS_INTERFACE_TABLE_HEAD(SVGAnimateMotionElement)
  NS_NODE_INTERFACE_TABLE5(SVGAnimateMotionElement, nsIDOMNode,
                           nsIDOMElement, nsIDOMSVGElement,
                           nsIDOMSVGAnimationElement,
                           nsIDOMSVGAnimateMotionElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGAnimateMotionElement)
NS_INTERFACE_MAP_END_INHERITING(SVGAnimationElement)




SVGAnimateMotionElement::SVGAnimateMotionElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : SVGAnimationElement(aNodeInfo)
{
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGAnimateMotionElement)




nsSMILAnimationFunction&
SVGAnimateMotionElement::AnimationFunction()
{
  return mAnimationFunction;
}

bool
SVGAnimateMotionElement::GetTargetAttributeName(int32_t *aNamespaceID,
                                                nsIAtom **aLocalName) const
{
  
  
  
  *aNamespaceID = kNameSpaceID_None;
  *aLocalName = nsGkAtoms::mozAnimateMotionDummyAttr;
  return true;
}

nsSMILTargetAttrType
SVGAnimateMotionElement::GetTargetAttributeType() const
{
  
  
  
  return eSMILTargetAttrType_XML;
}

} 
} 

