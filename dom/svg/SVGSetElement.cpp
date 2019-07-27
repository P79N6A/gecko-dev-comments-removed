




#include "mozilla/dom/SVGSetElement.h"
#include "mozilla/dom/SVGSetElementBinding.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Set)

namespace mozilla {
namespace dom {

JSObject*
SVGSetElement::WrapNode(JSContext *aCx)
{
  return SVGSetElementBinding::Wrap(aCx, this);
}




SVGSetElement::SVGSetElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
  : SVGAnimationElement(aNodeInfo)
{
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGSetElement)



nsSMILAnimationFunction&
SVGSetElement::AnimationFunction()
{
  return mAnimationFunction;
}

} 
} 

