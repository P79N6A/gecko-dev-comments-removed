



































#include "nsSVGClipPathElement.h"
#include "nsGkAtoms.h"

nsSVGElement::EnumInfo nsSVGClipPathElement::sEnumInfo[1] =
{
  { &nsGkAtoms::clipPathUnits,
    sSVGUnitTypesMap,
    nsIDOMSVGUnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE
  }
};

NS_IMPL_NS_NEW_SVG_ELEMENT(ClipPath)




NS_IMPL_ADDREF_INHERITED(nsSVGClipPathElement,nsSVGClipPathElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGClipPathElement,nsSVGClipPathElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGClipPathElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGClipPathElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGUnitTypes)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGClipPathElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGClipPathElementBase)




nsSVGClipPathElement::nsSVGClipPathElement(nsINodeInfo *aNodeInfo)
  : nsSVGClipPathElementBase(aNodeInfo)
{
}


NS_IMETHODIMP nsSVGClipPathElement::GetClipPathUnits(nsIDOMSVGAnimatedEnumeration * *aClipPathUnits)
{
  return mEnumAttributes[CLIPPATHUNITS].ToDOMAnimatedEnum(aClipPathUnits, this);
}

nsSVGElement::EnumAttributesInfo
nsSVGClipPathElement::GetEnumInfo()
{
  return EnumAttributesInfo(mEnumAttributes, sEnumInfo,
                            NS_ARRAY_LENGTH(sEnumInfo));
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGClipPathElement)

