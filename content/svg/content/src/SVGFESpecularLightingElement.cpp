




#include "mozilla/dom/SVGFESpecularLightingElement.h"
#include "mozilla/dom/SVGFESpecularLightingElementBinding.h"
#include "nsSVGUtils.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(FESpecularLighting)

namespace mozilla {
namespace dom {

JSObject*
SVGFESpecularLightingElement::WrapNode(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return SVGFESpecularLightingElementBinding::Wrap(aCx, aScope, this);
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGFESpecularLightingElement)

already_AddRefed<SVGAnimatedString>
SVGFESpecularLightingElement::In1()
{
  return mStringAttributes[IN1].ToDOMAnimatedString(this);
}

already_AddRefed<SVGAnimatedNumber>
SVGFESpecularLightingElement::SurfaceScale()
{
  return mNumberAttributes[SURFACE_SCALE].ToDOMAnimatedNumber(this);
}

already_AddRefed<SVGAnimatedNumber>
SVGFESpecularLightingElement::SpecularConstant()
{
  return mNumberAttributes[SPECULAR_CONSTANT].ToDOMAnimatedNumber(this);
}

already_AddRefed<SVGAnimatedNumber>
SVGFESpecularLightingElement::SpecularExponent()
{
  return mNumberAttributes[SPECULAR_EXPONENT].ToDOMAnimatedNumber(this);
}

already_AddRefed<SVGAnimatedNumber>
SVGFESpecularLightingElement::KernelUnitLengthX()
{
  return mNumberPairAttributes[KERNEL_UNIT_LENGTH].ToDOMAnimatedNumber(
    nsSVGNumberPair::eFirst, this);
}

already_AddRefed<SVGAnimatedNumber>
SVGFESpecularLightingElement::KernelUnitLengthY()
{
  return mNumberPairAttributes[KERNEL_UNIT_LENGTH].ToDOMAnimatedNumber(
    nsSVGNumberPair::eSecond, this);
}




nsresult
SVGFESpecularLightingElement::Filter(nsSVGFilterInstance* instance,
                                     const nsTArray<const Image*>& aSources,
                                     const Image* aTarget,
                                     const nsIntRect& rect)
{
  float specularExponent = mNumberAttributes[SPECULAR_EXPONENT].GetAnimValue();

  
  if (specularExponent < 1 || specularExponent > 128)
    return NS_ERROR_FAILURE;

  return SVGFESpecularLightingElementBase::Filter(instance, aSources, aTarget, rect);
}

bool
SVGFESpecularLightingElement::AttributeAffectsRendering(int32_t aNameSpaceID,
                                                        nsIAtom* aAttribute) const
{
  return SVGFESpecularLightingElementBase::AttributeAffectsRendering(aNameSpaceID, aAttribute) ||
         (aNameSpaceID == kNameSpaceID_None &&
          (aAttribute == nsGkAtoms::specularConstant ||
           aAttribute == nsGkAtoms::specularExponent));
}

void
SVGFESpecularLightingElement::LightPixel(const float *N, const float *L,
                                         nscolor color, uint8_t *targetData)
{
  float H[3];
  H[0] = L[0];
  H[1] = L[1];
  H[2] = L[2] + 1;
  NORMALIZE(H);

  float kS = mNumberAttributes[SPECULAR_CONSTANT].GetAnimValue();
  float dotNH = DOT(N, H);

  bool invalid = dotNH <= 0 || kS <= 0;
  kS *= invalid ? 0 : 1;
  uint8_t minAlpha = invalid ? 255 : 0;

  float specularNH =
    kS * pow(dotNH, mNumberAttributes[SPECULAR_EXPONENT].GetAnimValue());

  targetData[GFX_ARGB32_OFFSET_B] =
    std::min(uint32_t(specularNH * NS_GET_B(color)), 255U);
  targetData[GFX_ARGB32_OFFSET_G] =
    std::min(uint32_t(specularNH * NS_GET_G(color)), 255U);
  targetData[GFX_ARGB32_OFFSET_R] =
    std::min(uint32_t(specularNH * NS_GET_R(color)), 255U);

  targetData[GFX_ARGB32_OFFSET_A] =
    std::max(minAlpha, std::max(targetData[GFX_ARGB32_OFFSET_B],
                            std::max(targetData[GFX_ARGB32_OFFSET_G],
                                   targetData[GFX_ARGB32_OFFSET_R])));
}

} 
} 
