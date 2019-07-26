




#include "mozilla/Util.h"

#include "mozilla/dom/SVGClipPathElement.h"
#include "mozilla/dom/SVGClipPathElementBinding.h"
#include "nsGkAtoms.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(ClipPath)

DOMCI_NODE_DATA(SVGClipPathElement, mozilla::dom::SVGClipPathElement)

namespace mozilla {
namespace dom {

JSObject*
SVGClipPathElement::WrapNode(JSContext *aCx, JSObject *aScope, bool *aTriedToWrap)
{
  return SVGClipPathElementBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}

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
  SetIsDOMBinding();
}


NS_IMETHODIMP SVGClipPathElement::GetClipPathUnits(nsIDOMSVGAnimatedEnumeration * *aClipPathUnits)
{
  *aClipPathUnits = ClipPathUnits().get();
  return NS_OK;
}

already_AddRefed<nsIDOMSVGAnimatedEnumeration>
SVGClipPathElement::ClipPathUnits()
{
  nsCOMPtr<nsIDOMSVGAnimatedEnumeration> unit;
  mEnumAttributes[CLIPPATHUNITS].ToDOMAnimatedEnum(getter_AddRefs(unit), this);
  return unit.forget();
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
