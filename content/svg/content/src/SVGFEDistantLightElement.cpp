




#include "mozilla/dom/SVGFEDistantLightElement.h"
#include "mozilla/dom/SVGFEDistantLightElementBinding.h"
#include "nsSVGFilterInstance.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(FEDistantLight)

using namespace mozilla::gfx;

namespace mozilla {
namespace dom {

JSObject*
SVGFEDistantLightElement::WrapNode(JSContext* aCx)
{
  return SVGFEDistantLightElementBinding::Wrap(aCx, this);
}

nsSVGElement::NumberInfo SVGFEDistantLightElement::sNumberInfo[2] =
{
  { &nsGkAtoms::azimuth,   0, false },
  { &nsGkAtoms::elevation, 0, false }
};





NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGFEDistantLightElement)



bool
SVGFEDistantLightElement::AttributeAffectsRendering(int32_t aNameSpaceID,
                                                    nsIAtom* aAttribute) const
{
  return aNameSpaceID == kNameSpaceID_None &&
         (aAttribute == nsGkAtoms::azimuth ||
          aAttribute == nsGkAtoms::elevation);
}

AttributeMap
SVGFEDistantLightElement::ComputeLightAttributes(nsSVGFilterInstance* aInstance)
{
  float azimuth, elevation;
  GetAnimatedNumberValues(&azimuth, &elevation, nullptr);

  AttributeMap map;
  map.Set(eLightType, (uint32_t)eLightTypeDistant);
  map.Set(eDistantLightAzimuth, azimuth);
  map.Set(eDistantLightElevation, elevation);
  return map;
}

already_AddRefed<SVGAnimatedNumber>
SVGFEDistantLightElement::Azimuth()
{
  return mNumberAttributes[AZIMUTH].ToDOMAnimatedNumber(this);
}

already_AddRefed<SVGAnimatedNumber>
SVGFEDistantLightElement::Elevation()
{
  return mNumberAttributes[ELEVATION].ToDOMAnimatedNumber(this);
}




nsSVGElement::NumberAttributesInfo
SVGFEDistantLightElement::GetNumberInfo()
{
  return NumberAttributesInfo(mNumberAttributes, sNumberInfo,
                              ArrayLength(sNumberInfo));
}

} 
} 
