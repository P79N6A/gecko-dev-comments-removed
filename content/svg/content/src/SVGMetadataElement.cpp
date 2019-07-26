




#include "mozilla/dom/SVGMetadataElement.h"
#include "mozilla/dom/SVGMetadataElementBinding.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Metadata)

namespace mozilla {
namespace dom {

JSObject*
SVGMetadataElement::WrapNode(JSContext *aCx, JSObject *aScope)
{
  return SVGMetadataElementBinding::Wrap(aCx, aScope, this);
}




NS_IMPL_ISUPPORTS_INHERITED3(SVGMetadataElement, SVGMetadataElementBase,
                             nsIDOMNode, nsIDOMElement,
                             nsIDOMSVGElement)





SVGMetadataElement::SVGMetadataElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : SVGMetadataElementBase(aNodeInfo)
{
  SetIsDOMBinding();
}


nsresult
SVGMetadataElement::Init()
{
  return NS_OK;
}





NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGMetadataElement)

} 
} 

