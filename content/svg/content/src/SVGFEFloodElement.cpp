




#include "mozilla/dom/SVGFEFloodElement.h"

#include "FilterSupport.h"
#include "mozilla/dom/SVGFEFloodElementBinding.h"
#include "nsColor.h"
#include "nsIFrame.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(FEFlood)

using namespace mozilla::gfx;

namespace mozilla {
namespace dom {

JSObject*
SVGFEFloodElement::WrapNode(JSContext *aCx, JS::Handle<JSObject*> aScope)
{
  return SVGFEFloodElementBinding::Wrap(aCx, aScope, this);
}

nsSVGElement::StringInfo SVGFEFloodElement::sStringInfo[1] =
{
  { &nsGkAtoms::result, kNameSpaceID_None, true }
};




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGFEFloodElement)

FilterPrimitiveDescription
SVGFEFloodElement::GetPrimitiveDescription(nsSVGFilterInstance* aInstance,
                                           const IntRect& aFilterSubregion,
                                           const nsTArray<bool>& aInputsAreTainted,
                                           nsTArray<RefPtr<SourceSurface>>& aInputImages)
{
  FilterPrimitiveDescription descr(FilterPrimitiveDescription::eFlood);
  nsIFrame* frame = GetPrimaryFrame();
  if (frame) {
    nsStyleContext* style = frame->StyleContext();
    nscolor floodColor = style->StyleSVGReset()->mFloodColor;
    float floodOpacity = style->StyleSVGReset()->mFloodOpacity;
    Color color(NS_GET_R(floodColor) / 255.0,
                NS_GET_G(floodColor) / 255.0,
                NS_GET_B(floodColor) / 255.0,
                NS_GET_A(floodColor) / 255.0 * floodOpacity);
    descr.Attributes().Set(eFloodColor, color);
  } else {
    descr.Attributes().Set(eFloodColor, Color());
  }
  return descr;
}




NS_IMETHODIMP_(bool)
SVGFEFloodElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sFEFloodMap
  };

  return FindAttributeDependence(name, map) ||
    SVGFEFloodElementBase::IsAttributeMapped(name);
}




nsSVGElement::StringAttributesInfo
SVGFEFloodElement::GetStringInfo()
{
  return StringAttributesInfo(mStringAttributes, sStringInfo,
                              ArrayLength(sStringInfo));
}

} 
} 
