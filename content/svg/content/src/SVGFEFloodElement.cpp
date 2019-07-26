




#include "mozilla/dom/SVGFEFloodElement.h"
#include "mozilla/dom/SVGFEFloodElementBinding.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(FEFlood)

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

nsresult
SVGFEFloodElement::Filter(nsSVGFilterInstance *instance,
                          const nsTArray<const Image*>& aSources,
                          const Image* aTarget,
                          const nsIntRect& aDataRect)
{
  nsIFrame* frame = GetPrimaryFrame();
  if (!frame) return NS_ERROR_FAILURE;
  nsStyleContext* style = frame->StyleContext();

  nscolor floodColor = style->StyleSVGReset()->mFloodColor;
  float floodOpacity = style->StyleSVGReset()->mFloodOpacity;

  gfxContext ctx(aTarget->mImage);
  ctx.SetColor(gfxRGBA(NS_GET_R(floodColor) / 255.0,
                       NS_GET_G(floodColor) / 255.0,
                       NS_GET_B(floodColor) / 255.0,
                       NS_GET_A(floodColor) / 255.0 * floodOpacity));
  ctx.Rectangle(aTarget->mFilterPrimitiveSubregion);
  ctx.Fill();
  return NS_OK;
}

nsIntRect
SVGFEFloodElement::ComputeTargetBBox(const nsTArray<nsIntRect>& aSourceBBoxes,
                                     const nsSVGFilterInstance& aInstance)
{
  return GetMaxRect();
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
