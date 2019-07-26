




#include "mozilla/dom/SVGFESpecularLightingElement.h"
#include "mozilla/dom/SVGFESpecularLightingElementBinding.h"
#include "nsSVGUtils.h"
#include "nsSVGFilterInstance.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(FESpecularLighting)

using namespace mozilla::gfx;

namespace mozilla {
namespace dom {

JSObject*
SVGFESpecularLightingElement::WrapNode(JSContext* aCx)
{
  return SVGFESpecularLightingElementBinding::Wrap(aCx, this);
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




FilterPrimitiveDescription
SVGFESpecularLightingElement::GetPrimitiveDescription(nsSVGFilterInstance* aInstance,
                                                      const IntRect& aFilterSubregion,
                                                      const nsTArray<bool>& aInputsAreTainted,
                                                      nsTArray<RefPtr<SourceSurface>>& aInputImages)
{
  float specularExponent = mNumberAttributes[SPECULAR_EXPONENT].GetAnimValue();
  float specularConstant = mNumberAttributes[SPECULAR_CONSTANT].GetAnimValue();

  
  if (specularExponent < 1 || specularExponent > 128) {
    return FilterPrimitiveDescription(PrimitiveType::Empty);
  }

  FilterPrimitiveDescription descr(PrimitiveType::SpecularLighting);
  descr.Attributes().Set(eSpecularLightingSpecularConstant, specularConstant);
  descr.Attributes().Set(eSpecularLightingSpecularExponent, specularExponent);
  return AddLightingAttributes(descr, aInstance);
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

} 
} 
