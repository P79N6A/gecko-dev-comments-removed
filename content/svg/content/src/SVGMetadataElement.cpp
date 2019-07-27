




#include "mozilla/dom/SVGMetadataElement.h"
#include "mozilla/dom/SVGMetadataElementBinding.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Metadata)

namespace mozilla {
namespace dom {

JSObject*
SVGMetadataElement::WrapNode(JSContext *aCx)
{
  return SVGMetadataElementBinding::Wrap(aCx, this);
}




SVGMetadataElement::SVGMetadataElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
  : SVGMetadataElementBase(aNodeInfo)
{
}


nsresult
SVGMetadataElement::Init()
{
  return NS_OK;
}





NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGMetadataElement)

} 
} 

