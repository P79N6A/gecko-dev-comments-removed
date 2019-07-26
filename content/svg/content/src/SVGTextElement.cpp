




#include "mozilla/dom/SVGTextElement.h"
#include "mozilla/dom/SVGTextElementBinding.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Text)

namespace mozilla {
namespace dom {

JSObject*
SVGTextElement::WrapNode(JSContext *aCx, JSObject *aScope)
{
  return SVGTextElementBinding::Wrap(aCx, aScope, this);
}




NS_IMPL_ISUPPORTS_INHERITED3(SVGTextElement, SVGTextElementBase,
                             nsIDOMNode, nsIDOMElement,
                             nsIDOMSVGElement)




SVGTextElement::SVGTextElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : SVGTextElementBase(aNodeInfo)
{
  SetIsDOMBinding();
}





NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGTextElement)





NS_IMETHODIMP_(bool)
SVGTextElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sTextContentElementsMap,
    sFontSpecificationMap
  };

  return FindAttributeDependence(name, map) ||
    SVGTextElementBase::IsAttributeMapped(name);
}

} 
} 
