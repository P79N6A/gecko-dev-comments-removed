




#include "mozilla/dom/SVGGraphicsElement.h"

namespace mozilla {
namespace dom {




NS_IMPL_ADDREF_INHERITED(SVGGraphicsElement, SVGGraphicsElementBase)
NS_IMPL_RELEASE_INHERITED(SVGGraphicsElement, SVGGraphicsElementBase)

NS_INTERFACE_MAP_BEGIN(SVGGraphicsElement)
  NS_INTERFACE_MAP_ENTRY(mozilla::dom::SVGTests)
NS_INTERFACE_MAP_END_INHERITING(SVGGraphicsElementBase)




SVGGraphicsElement::SVGGraphicsElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
  : SVGGraphicsElementBase(aNodeInfo)
{
}

SVGGraphicsElement::~SVGGraphicsElement()
{
}

} 
} 
