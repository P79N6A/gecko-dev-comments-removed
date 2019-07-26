




#include "mozilla/dom/SVGSetElement.h"
#include "nsSMILSetAnimationFunction.h"

DOMCI_NODE_DATA(SVGSetElement, mozilla::dom::SVGSetElement)

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Set)

namespace mozilla {
namespace dom {




NS_IMPL_ADDREF_INHERITED(SVGSetElement, SVGAnimationElement)
NS_IMPL_RELEASE_INHERITED(SVGSetElement, SVGAnimationElement)

NS_INTERFACE_TABLE_HEAD(SVGSetElement)
  NS_NODE_INTERFACE_TABLE5(SVGSetElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGAnimationElement,
                           nsIDOMSVGSetElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGSetElement)
NS_INTERFACE_MAP_END_INHERITING(SVGAnimationElement)




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

