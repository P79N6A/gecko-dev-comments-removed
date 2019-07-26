




#include "mozilla/Util.h"

#include "mozilla/dom/SVGClipPathElement.h"
#include "mozilla/dom/SVGClipPathElementBinding.h"
#include "nsGkAtoms.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(ClipPath)

namespace mozilla {
namespace dom {

JSObject*
SVGClipPathElement::WrapNode(JSContext *aCx, JSObject *aScope)
{
  return SVGClipPathElementBinding::Wrap(aCx, aScope, this);
}

nsSVGElement::EnumInfo SVGClipPathElement::sEnumInfo[1] =
{
  { &nsGkAtoms::clipPathUnits,
    sSVGUnitTypesMap,
    SVG_UNIT_TYPE_USERSPACEONUSE
  }
};




NS_IMPL_ISUPPORTS_INHERITED3(SVGClipPathElement, SVGClipPathElementBase,
                             nsIDOMNode, nsIDOMElement,
                             nsIDOMSVGElement)




SVGClipPathElement::SVGClipPathElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : SVGClipPathElementBase(aNodeInfo)
{
  SetIsDOMBinding();
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
