




#include "mozilla/Util.h"

#include "mozilla/dom/SVGClipPathElement.h"
#include "mozilla/dom/SVGClipPathElementBinding.h"
#include "nsGkAtoms.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(ClipPath)

namespace mozilla {
namespace dom {

JSObject*
SVGClipPathElement::WrapNode(JSContext *aCx, JS::Handle<JSObject*> aScope)
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




SVGClipPathElement::SVGClipPathElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : SVGClipPathElementBase(aNodeInfo)
{
}

already_AddRefed<SVGAnimatedEnumeration>
SVGClipPathElement::ClipPathUnits()
{
  return mEnumAttributes[CLIPPATHUNITS].ToDOMAnimatedEnum(this);
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
