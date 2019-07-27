




#include "mozilla/dom/SVGAnimateElement.h"
#include "mozilla/dom/SVGAnimateElementBinding.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Animate)

namespace mozilla {
namespace dom {

JSObject*
SVGAnimateElement::WrapNode(JSContext *aCx)
{
  return SVGAnimateElementBinding::Wrap(aCx, this);
}




SVGAnimateElement::SVGAnimateElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
  : SVGAnimationElement(aNodeInfo)
{
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGAnimateElement)



nsSMILAnimationFunction&
SVGAnimateElement::AnimationFunction()
{
  return mAnimationFunction;
}

} 
} 

