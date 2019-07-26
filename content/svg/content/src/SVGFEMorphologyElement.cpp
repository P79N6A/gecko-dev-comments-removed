




#include "mozilla/dom/SVGFEMorphologyElement.h"
#include "mozilla/dom/SVGFEMorphologyElementBinding.h"
#include "nsSVGFilterInstance.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(FEMorphology)

namespace mozilla {
namespace dom {

JSObject*
SVGFEMorphologyElement::WrapNode(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return SVGFEMorphologyElementBinding::Wrap(aCx, aScope, this);
}


static const unsigned short SVG_OPERATOR_ERODE = 1;
static const unsigned short SVG_OPERATOR_DILATE = 2;

nsSVGElement::NumberPairInfo SVGFEMorphologyElement::sNumberPairInfo[1] =
{
  { &nsGkAtoms::radius, 0, 0 }
};

nsSVGEnumMapping SVGFEMorphologyElement::sOperatorMap[] = {
  {&nsGkAtoms::erode, SVG_OPERATOR_ERODE},
  {&nsGkAtoms::dilate, SVG_OPERATOR_DILATE},
  {nullptr, 0}
};

nsSVGElement::EnumInfo SVGFEMorphologyElement::sEnumInfo[1] =
{
  { &nsGkAtoms::_operator,
    sOperatorMap,
    SVG_OPERATOR_ERODE
  }
};

nsSVGElement::StringInfo SVGFEMorphologyElement::sStringInfo[2] =
{
  { &nsGkAtoms::result, kNameSpaceID_None, true },
  { &nsGkAtoms::in, kNameSpaceID_None, true }
};





NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGFEMorphologyElement)





already_AddRefed<SVGAnimatedString>
SVGFEMorphologyElement::In1()
{
  return mStringAttributes[IN1].ToDOMAnimatedString(this);
}

already_AddRefed<SVGAnimatedEnumeration>
SVGFEMorphologyElement::Operator()
{
  return mEnumAttributes[OPERATOR].ToDOMAnimatedEnum(this);
}

already_AddRefed<SVGAnimatedNumber>
SVGFEMorphologyElement::RadiusX()
{
  return mNumberPairAttributes[RADIUS].ToDOMAnimatedNumber(nsSVGNumberPair::eFirst, this);
}

already_AddRefed<SVGAnimatedNumber>
SVGFEMorphologyElement::RadiusY()
{
  return mNumberPairAttributes[RADIUS].ToDOMAnimatedNumber(nsSVGNumberPair::eSecond, this);
}

void
SVGFEMorphologyElement::SetRadius(float rx, float ry)
{
  mNumberPairAttributes[RADIUS].SetBaseValues(rx, ry, this);
}

void
SVGFEMorphologyElement::GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources)
{
  aSources.AppendElement(nsSVGStringInfo(&mStringAttributes[IN1], this));
}

nsIntRect
SVGFEMorphologyElement::InflateRect(const nsIntRect& aRect,
                                    const nsSVGFilterInstance& aInstance)
{
  int32_t rx, ry;
  GetRXY(&rx, &ry, aInstance);
  nsIntRect result = aRect;
  result.Inflate(std::max(0, rx), std::max(0, ry));
  return result;
}

nsIntRect
SVGFEMorphologyElement::ComputeTargetBBox(const nsTArray<nsIntRect>& aSourceBBoxes,
        const nsSVGFilterInstance& aInstance)
{
  return InflateRect(aSourceBBoxes[0], aInstance);
}

void
SVGFEMorphologyElement::ComputeNeededSourceBBoxes(const nsIntRect& aTargetBBox,
          nsTArray<nsIntRect>& aSourceBBoxes, const nsSVGFilterInstance& aInstance)
{
  aSourceBBoxes[0] = InflateRect(aTargetBBox, aInstance);
}

nsIntRect
SVGFEMorphologyElement::ComputeChangeBBox(const nsTArray<nsIntRect>& aSourceChangeBoxes,
                                          const nsSVGFilterInstance& aInstance)
{
  return InflateRect(aSourceChangeBoxes[0], aInstance);
}

#define MORPHOLOGY_EPSILON 0.0001

void
SVGFEMorphologyElement::GetRXY(int32_t *aRX, int32_t *aRY,
                               const nsSVGFilterInstance& aInstance)
{
  
  
  
  
  *aRX = NSToIntCeil(aInstance.GetPrimitiveNumber(SVGContentUtils::X,
                                                  &mNumberPairAttributes[RADIUS],
                                                  nsSVGNumberPair::eFirst) -
                     MORPHOLOGY_EPSILON);
  *aRY = NSToIntCeil(aInstance.GetPrimitiveNumber(SVGContentUtils::Y,
                                                  &mNumberPairAttributes[RADIUS],
                                                  nsSVGNumberPair::eSecond) -
                     MORPHOLOGY_EPSILON);
}

template<uint32_t Operator>
static void
DoMorphology(nsSVGFilterInstance* instance,
             uint8_t* sourceData,
             uint8_t* targetData,
             int32_t stride,
             const nsIntRect& rect,
             int32_t rx,
             int32_t ry)
{
  static_assert(Operator == SVG_OPERATOR_ERODE ||
                Operator == SVG_OPERATOR_DILATE,
                "unexpected morphology operator");

  volatile uint8_t extrema[4];         

  
  for (int32_t y = rect.y; y < rect.YMost(); y++) {
    int32_t startY = std::max(0, y - ry);
    
    
    
    int32_t endY = std::min(y + ry, instance->GetSurfaceHeight() - 1);
    for (int32_t x = rect.x; x < rect.XMost(); x++) {
      int32_t startX = std::max(0, x - rx);
      int32_t endX = std::min(x + rx, instance->GetSurfaceWidth() - 1);
      int32_t targIndex = y * stride + 4 * x;

      for (int32_t i = 0; i < 4; i++) {
        extrema[i] = sourceData[targIndex + i];
      }
      for (int32_t y1 = startY; y1 <= endY; y1++) {
        for (int32_t x1 = startX; x1 <= endX; x1++) {
          for (int32_t i = 0; i < 4; i++) {
            uint8_t pixel = sourceData[y1 * stride + 4 * x1 + i];
            if (Operator == SVG_OPERATOR_ERODE) {
              extrema[i] -= (extrema[i] - pixel) & -(extrema[i] > pixel);
            } else {
              extrema[i] -= (extrema[i] - pixel) & -(extrema[i] < pixel);
            }
          }
        }
      }
      targetData[targIndex  ] = extrema[0];
      targetData[targIndex+1] = extrema[1];
      targetData[targIndex+2] = extrema[2];
      targetData[targIndex+3] = extrema[3];
    }
  }
}

nsresult
SVGFEMorphologyElement::Filter(nsSVGFilterInstance* instance,
                               const nsTArray<const Image*>& aSources,
                               const Image* aTarget,
                               const nsIntRect& rect)
{
  int32_t rx, ry;
  GetRXY(&rx, &ry, *instance);

  if (rx < 0 || ry < 0) {
    
    return NS_OK;
  }
  if (rx == 0 && ry == 0) {
    return NS_OK;
  }

  
  rx = std::min(rx, 100000);
  ry = std::min(ry, 100000);

  uint8_t* sourceData = aSources[0]->mImage->Data();
  uint8_t* targetData = aTarget->mImage->Data();
  int32_t stride = aTarget->mImage->Stride();

  if (mEnumAttributes[OPERATOR].GetAnimValue() == SVG_OPERATOR_ERODE) {
    DoMorphology<SVG_OPERATOR_ERODE>(instance, sourceData, targetData, stride,
                                     rect, rx, ry);
  } else {
    DoMorphology<SVG_OPERATOR_DILATE>(instance, sourceData, targetData, stride,
                                      rect, rx, ry);
  }

  return NS_OK;
}

bool
SVGFEMorphologyElement::AttributeAffectsRendering(int32_t aNameSpaceID,
                                                  nsIAtom* aAttribute) const
{
  return SVGFEMorphologyElementBase::AttributeAffectsRendering(aNameSpaceID, aAttribute) ||
         (aNameSpaceID == kNameSpaceID_None &&
          (aAttribute == nsGkAtoms::in ||
           aAttribute == nsGkAtoms::radius ||
           aAttribute == nsGkAtoms::_operator));
}




nsSVGElement::NumberPairAttributesInfo
SVGFEMorphologyElement::GetNumberPairInfo()
{
  return NumberPairAttributesInfo(mNumberPairAttributes, sNumberPairInfo,
                                  ArrayLength(sNumberPairInfo));
}

nsSVGElement::EnumAttributesInfo
SVGFEMorphologyElement::GetEnumInfo()
{
  return EnumAttributesInfo(mEnumAttributes, sEnumInfo,
                            ArrayLength(sEnumInfo));
}

nsSVGElement::StringAttributesInfo
SVGFEMorphologyElement::GetStringInfo()
{
  return StringAttributesInfo(mStringAttributes, sStringInfo,
                              ArrayLength(sStringInfo));
}

} 
} 
