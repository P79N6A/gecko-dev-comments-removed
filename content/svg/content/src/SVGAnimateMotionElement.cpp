




#include "mozilla/dom/SVGAnimateMotionElement.h"
#include "mozilla/dom/SVGAnimateMotionElementBinding.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(AnimateMotion)

namespace mozilla {
namespace dom {

JSObject*
SVGAnimateMotionElement::WrapNode(JSContext *aCx, JSObject *aScope, bool *aTriedToWrap)
{
  return SVGAnimateMotionElementBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}



NS_IMPL_ISUPPORTS_INHERITED4(SVGAnimateMotionElement, SVGAnimationElement,
                             nsIDOMNode,
                             nsIDOMElement, nsIDOMSVGElement,
                             nsIDOMSVGAnimationElement)




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

