





#include "mozilla/dom/SVGAnimateMotionElement.h"
#include "mozilla/dom/SVGAnimateMotionElementBinding.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(AnimateMotion)

namespace mozilla {
namespace dom {

JSObject*
SVGAnimateMotionElement::WrapNode(JSContext *aCx, JS::Handle<JSObject*> aGivenProto)
{
  return SVGAnimateMotionElementBinding::Wrap(aCx, this, aGivenProto);
}




SVGAnimateMotionElement::SVGAnimateMotionElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
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

