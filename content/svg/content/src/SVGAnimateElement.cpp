




#include "mozilla/dom/SVGAnimateElement.h"

DOMCI_NODE_DATA(SVGAnimateElement, mozilla::dom::SVGAnimateElement)

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Animate)

namespace mozilla {
namespace dom {





NS_IMPL_ADDREF_INHERITED(SVGAnimateElement, SVGAnimationElement)
NS_IMPL_RELEASE_INHERITED(SVGAnimateElement, SVGAnimationElement)

NS_INTERFACE_TABLE_HEAD(SVGAnimateElement)
  NS_NODE_INTERFACE_TABLE5(SVGAnimateElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGAnimationElement,
                           nsIDOMSVGAnimateElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGAnimateElement)
NS_INTERFACE_MAP_END_INHERITING(SVGAnimationElement)




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

