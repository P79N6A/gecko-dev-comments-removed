




#include "mozilla/Util.h"

#include "mozilla/dom/SVGClipPathElement.h"
#include "nsGkAtoms.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(ClipPath)

DOMCI_NODE_DATA(SVGClipPathElement, mozilla::dom::SVGClipPathElement)

namespace mozilla {
namespace dom {

nsSVGElement::EnumInfo SVGClipPathElement::sEnumInfo[1] =
{
  { &nsGkAtoms::clipPathUnits,
    sSVGUnitTypesMap,
    nsIDOMSVGUnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE
  }
};




NS_IMPL_ADDREF_INHERITED(SVGClipPathElement,SVGClipPathElementBase)
NS_IMPL_RELEASE_INHERITED(SVGClipPathElement,SVGClipPathElementBase)

NS_INTERFACE_TABLE_HEAD(SVGClipPathElement)
  NS_NODE_INTERFACE_TABLE5(SVGClipPathElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement,
                           nsIDOMSVGClipPathElement,
                           nsIDOMSVGUnitTypes)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGClipPathElement)
NS_INTERFACE_MAP_END_INHERITING(SVGClipPathElementBase)




SVGClipPathElement::SVGClipPathElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : SVGClipPathElementBase(aNodeInfo)
{
}


NS_IMETHODIMP SVGClipPathElement::GetClipPathUnits(nsIDOMSVGAnimatedEnumeration * *aClipPathUnits)
{
  return mEnumAttributes[CLIPPATHUNITS].ToDOMAnimatedEnum(aClipPathUnits, this);
}

nsSVGElement::EnumAttributesInfo
SVGClipPathElement::GetEnumInfo()
{
  return EnumAttributesInfo(mEnumAttributes, sEnumInfo,
                            ArrayLength(sEnumInfo));
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGClipPathElement)

} 
} 
