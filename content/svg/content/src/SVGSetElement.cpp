




#include "mozilla/dom/SVGSetElement.h"
#include "mozilla/dom/SVGSetElementBinding.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Set)

namespace mozilla {
namespace dom {

JSObject*
SVGSetElement::WrapNode(JSContext *aCx, JSObject *aScope, bool *aTriedToWrap)
{
  return SVGSetElementBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}




NS_IMPL_ISUPPORTS_INHERITED3(SVGSetElement, SVGAnimationElement,
                             nsIDOMNode, nsIDOMElement,
                             nsIDOMSVGElement)




SVGSetElement::SVGSetElement(already_AddRefed<nsINodeInfo> aNodeInfo)
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

