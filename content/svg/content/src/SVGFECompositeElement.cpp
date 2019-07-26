




#include "mozilla/dom/SVGFECompositeElement.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(FEComposite)

namespace mozilla {
namespace dom {

nsSVGElement::NumberInfo nsSVGFECompositeElement::sNumberInfo[4] =
{
  { &nsGkAtoms::k1, 0, false },
  { &nsGkAtoms::k2, 0, false },
  { &nsGkAtoms::k3, 0, false },
  { &nsGkAtoms::k4, 0, false }
};

nsSVGEnumMapping nsSVGFECompositeElement::sOperatorMap[] = {
  {&nsGkAtoms::over, nsSVGFECompositeElement::SVG_OPERATOR_OVER},
  {&nsGkAtoms::in, nsSVGFECompositeElement::SVG_OPERATOR_IN},
  {&nsGkAtoms::out, nsSVGFECompositeElement::SVG_OPERATOR_OUT},
  {&nsGkAtoms::atop, nsSVGFECompositeElement::SVG_OPERATOR_ATOP},
  {&nsGkAtoms::xor_, nsSVGFECompositeElement::SVG_OPERATOR_XOR},
  {&nsGkAtoms::arithmetic, nsSVGFECompositeElement::SVG_OPERATOR_ARITHMETIC},
  {nullptr, 0}
};

nsSVGElement::EnumInfo nsSVGFECompositeElement::sEnumInfo[1] =
{
  { &nsGkAtoms::_operator,
    sOperatorMap,
    nsIDOMSVGFECompositeElement::SVG_OPERATOR_OVER
  }
};

nsSVGElement::StringInfo nsSVGFECompositeElement::sStringInfo[3] =
{
  { &nsGkAtoms::result, kNameSpaceID_None, true },
  { &nsGkAtoms::in, kNameSpaceID_None, true },
  { &nsGkAtoms::in2, kNameSpaceID_None, true }
};




NS_IMPL_ADDREF_INHERITED(nsSVGFECompositeElement,nsSVGFECompositeElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGFECompositeElement,nsSVGFECompositeElementBase)

DOMCI_NODE_DATA(SVGFECompositeElement, nsSVGFECompositeElement)

NS_INTERFACE_TABLE_HEAD(nsSVGFECompositeElement)
  NS_NODE_INTERFACE_TABLE5(nsSVGFECompositeElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement,
                           nsIDOMSVGFilterPrimitiveStandardAttributes,
                           nsIDOMSVGFECompositeElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGFECompositeElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGFECompositeElementBase)




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGFECompositeElement)





NS_IMETHODIMP nsSVGFECompositeElement::GetIn1(nsIDOMSVGAnimatedString * *aIn)
{
  return mStringAttributes[IN1].ToDOMAnimatedString(aIn, this);
}


NS_IMETHODIMP nsSVGFECompositeElement::GetIn2(nsIDOMSVGAnimatedString * *aIn)
{
  return mStringAttributes[IN2].ToDOMAnimatedString(aIn, this);
}



NS_IMETHODIMP nsSVGFECompositeElement::GetOperator(nsIDOMSVGAnimatedEnumeration * *aOperator)
{
  return mEnumAttributes[OPERATOR].ToDOMAnimatedEnum(aOperator, this);
}


NS_IMETHODIMP nsSVGFECompositeElement::GetK1(nsIDOMSVGAnimatedNumber * *aK1)
{
  return mNumberAttributes[K1].ToDOMAnimatedNumber(aK1, this);
}


NS_IMETHODIMP nsSVGFECompositeElement::GetK2(nsIDOMSVGAnimatedNumber * *aK2)
{
  return mNumberAttributes[K2].ToDOMAnimatedNumber(aK2, this);
}


NS_IMETHODIMP nsSVGFECompositeElement::GetK3(nsIDOMSVGAnimatedNumber * *aK3)
{
  return mNumberAttributes[K3].ToDOMAnimatedNumber(aK3, this);
}


NS_IMETHODIMP nsSVGFECompositeElement::GetK4(nsIDOMSVGAnimatedNumber * *aK4)
{
  return mNumberAttributes[K4].ToDOMAnimatedNumber(aK4, this);
}

NS_IMETHODIMP
nsSVGFECompositeElement::SetK(float k1, float k2, float k3, float k4)
{
  NS_ENSURE_FINITE4(k1, k2, k3, k4, NS_ERROR_ILLEGAL_VALUE);
  mNumberAttributes[K1].SetBaseValue(k1, this);
  mNumberAttributes[K2].SetBaseValue(k2, this);
  mNumberAttributes[K3].SetBaseValue(k3, this);
  mNumberAttributes[K4].SetBaseValue(k4, this);
  return NS_OK;
}

nsresult
nsSVGFECompositeElement::Filter(nsSVGFilterInstance *instance,
                                const nsTArray<const Image*>& aSources,
                                const Image* aTarget,
                                const nsIntRect& rect)
{
  uint16_t op = mEnumAttributes[OPERATOR].GetAnimValue();

  
  if (op == nsSVGFECompositeElement::SVG_OPERATOR_ARITHMETIC) {
    float k1, k2, k3, k4;
    GetAnimatedNumberValues(&k1, &k2, &k3, &k4, nullptr);

    
    CopyRect(aTarget, aSources[0], rect);

    uint8_t* sourceData = aSources[1]->mImage->Data();
    uint8_t* targetData = aTarget->mImage->Data();
    uint32_t stride = aTarget->mImage->Stride();

    
    float k1Scaled = k1 / 255.0f;
    float k4Scaled = k4*255.0f;
    for (int32_t x = rect.x; x < rect.XMost(); x++) {
      for (int32_t y = rect.y; y < rect.YMost(); y++) {
        uint32_t targIndex = y * stride + 4 * x;
        for (int32_t i = 0; i < 4; i++) {
          uint8_t i1 = targetData[targIndex + i];
          uint8_t i2 = sourceData[targIndex + i];
          float result = k1Scaled*i1*i2 + k2*i1 + k3*i2 + k4Scaled;
          targetData[targIndex + i] =
                       static_cast<uint8_t>(clamped(result, 0.f, 255.f));
        }
      }
    }
    return NS_OK;
  }

  

  gfxContext ctx(aTarget->mImage);
  ctx.SetOperator(gfxContext::OPERATOR_SOURCE);
  ctx.SetSource(aSources[1]->mImage);
  
  ctx.Clip(aTarget->mFilterPrimitiveSubregion);
  ctx.Paint();

  if (op < SVG_OPERATOR_OVER || op > SVG_OPERATOR_XOR) {
    return NS_ERROR_FAILURE;
  }
  static const gfxContext::GraphicsOperator opMap[] = {
                                           gfxContext::OPERATOR_DEST,
                                           gfxContext::OPERATOR_OVER,
                                           gfxContext::OPERATOR_IN,
                                           gfxContext::OPERATOR_OUT,
                                           gfxContext::OPERATOR_ATOP,
                                           gfxContext::OPERATOR_XOR };
  ctx.SetOperator(opMap[op]);
  ctx.SetSource(aSources[0]->mImage);
  ctx.Paint();
  return NS_OK;
}

bool
nsSVGFECompositeElement::AttributeAffectsRendering(int32_t aNameSpaceID,
                                                   nsIAtom* aAttribute) const
{
  return nsSVGFECompositeElementBase::AttributeAffectsRendering(aNameSpaceID, aAttribute) ||
         (aNameSpaceID == kNameSpaceID_None &&
          (aAttribute == nsGkAtoms::in ||
           aAttribute == nsGkAtoms::in2 ||
           aAttribute == nsGkAtoms::k1 ||
           aAttribute == nsGkAtoms::k2 ||
           aAttribute == nsGkAtoms::k3 ||
           aAttribute == nsGkAtoms::k4 ||
           aAttribute == nsGkAtoms::_operator));
}

void
nsSVGFECompositeElement::GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources)
{
  aSources.AppendElement(nsSVGStringInfo(&mStringAttributes[IN1], this));
  aSources.AppendElement(nsSVGStringInfo(&mStringAttributes[IN2], this));
}

nsIntRect
nsSVGFECompositeElement::ComputeTargetBBox(const nsTArray<nsIntRect>& aSourceBBoxes,
        const nsSVGFilterInstance& aInstance)
{
  uint16_t op = mEnumAttributes[OPERATOR].GetAnimValue();

  if (op == nsSVGFECompositeElement::SVG_OPERATOR_ARITHMETIC) {
    
    
    
    
    
    return GetMaxRect();
  }

  if (op == nsSVGFECompositeElement::SVG_OPERATOR_IN ||
      op == nsSVGFECompositeElement::SVG_OPERATOR_ATOP) {
    
    
    return aSourceBBoxes[1];
  }

  
  
  
  return nsSVGFECompositeElementBase::ComputeTargetBBox(aSourceBBoxes, aInstance);
}




nsSVGElement::NumberAttributesInfo
nsSVGFECompositeElement::GetNumberInfo()
{
  return NumberAttributesInfo(mNumberAttributes, sNumberInfo,
                              ArrayLength(sNumberInfo));
}

nsSVGElement::EnumAttributesInfo
nsSVGFECompositeElement::GetEnumInfo()
{
  return EnumAttributesInfo(mEnumAttributes, sEnumInfo,
                            ArrayLength(sEnumInfo));
}

nsSVGElement::StringAttributesInfo
nsSVGFECompositeElement::GetStringInfo()
{
  return StringAttributesInfo(mStringAttributes, sStringInfo,
                              ArrayLength(sStringInfo));
}

} 
} 
