




#include "mozilla/dom/SVGAnimateElement.h"
#include "mozilla/dom/SVGAnimateElementBinding.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Animate)

namespace mozilla {
namespace dom {

JSObject*
SVGAnimateElement::WrapNode(JSContext *aCx, JSObject *aScope)
{
  return SVGAnimateElementBinding::Wrap(aCx, aScope, this);
}




SVGAnimateElement::SVGAnimateElement(already_AddRefed<nsINodeInfo> aNodeInfo)
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

