




#include "nsSVGAnimatedTransformList.h"
#include "mozilla/dom/SVGAnimatedTransformList.h"
#include "mozilla/dom/SVGAnimationElement.h"
#include "nsCharSeparatedTokenizer.h"
#include "nsSVGTransform.h"
#include "nsSMILValue.h"
#include "SVGContentUtils.h"
#include "SVGTransformListSMILType.h"
#include "nsIDOMMutationEvent.h"

namespace mozilla {

using namespace dom;

nsresult
nsSVGAnimatedTransformList::SetBaseValueString(const nsAString& aValue)
{
  SVGTransformList newBaseValue;
  nsresult rv = newBaseValue.SetValueFromString(aValue);
  if (NS_FAILED(rv)) {
    return rv;
  }

  return SetBaseValue(newBaseValue);
}

nsresult
nsSVGAnimatedTransformList::SetBaseValue(const SVGTransformList& aValue)
{
  SVGAnimatedTransformList *domWrapper =
    SVGAnimatedTransformList::GetDOMWrapperIfExists(this);
  if (domWrapper) {
    
    
    
    
    
    domWrapper->InternalBaseValListWillChangeLengthTo(aValue.Length());
  }

  
  
  

  nsresult rv = mBaseVal.CopyFrom(aValue);
  if (NS_FAILED(rv) && domWrapper) {
    
    
    domWrapper->InternalBaseValListWillChangeLengthTo(mBaseVal.Length());
  } else {
    mIsAttrSet = true;
  }
  return rv;
}

void
nsSVGAnimatedTransformList::ClearBaseValue()
{
  SVGAnimatedTransformList *domWrapper =
    SVGAnimatedTransformList::GetDOMWrapperIfExists(this);
  if (domWrapper) {
    
    domWrapper->InternalBaseValListWillChangeLengthTo(0);
  }
  mBaseVal.Clear();
  mIsAttrSet = false;
  
}

nsresult
nsSVGAnimatedTransformList::SetAnimValue(const SVGTransformList& aValue,
                                         nsSVGElement *aElement)
{
  bool prevSet = HasTransform() || aElement->GetAnimateMotionTransform();
  SVGAnimatedTransformList *domWrapper =
    SVGAnimatedTransformList::GetDOMWrapperIfExists(this);
  if (domWrapper) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    domWrapper->InternalAnimValListWillChangeLengthTo(aValue.Length());
  }
  if (!mAnimVal) {
    mAnimVal = new SVGTransformList();
  }
  nsresult rv = mAnimVal->CopyFrom(aValue);
  if (NS_FAILED(rv)) {
    
    
    ClearAnimValue(aElement);
    return rv;
  }
  int32_t modType;
  if(prevSet) {
    modType = nsIDOMMutationEvent::MODIFICATION;
  } else {
    modType = nsIDOMMutationEvent::ADDITION;
  }
  aElement->DidAnimateTransformList(modType);
  return NS_OK;
}

void
nsSVGAnimatedTransformList::ClearAnimValue(nsSVGElement *aElement)
{
  SVGAnimatedTransformList *domWrapper =
    SVGAnimatedTransformList::GetDOMWrapperIfExists(this);
  if (domWrapper) {
    
    
    
    
    
    domWrapper->InternalAnimValListWillChangeLengthTo(mBaseVal.Length());
  }
  mAnimVal = nullptr;
  int32_t modType;
  if (HasTransform() || aElement->GetAnimateMotionTransform()) {
    modType = nsIDOMMutationEvent::MODIFICATION;
  } else {
    modType = nsIDOMMutationEvent::REMOVAL;
  }
  aElement->DidAnimateTransformList(modType);
}

bool
nsSVGAnimatedTransformList::IsExplicitlySet() const
{
  
  
  
  
  
  
  
  
  
  
  
  return mIsAttrSet || !mBaseVal.IsEmpty() || mAnimVal;
}

nsISMILAttr*
nsSVGAnimatedTransformList::ToSMILAttr(nsSVGElement* aSVGElement)
{
  return new SMILAnimatedTransformList(this, aSVGElement);
}

nsresult
nsSVGAnimatedTransformList::SMILAnimatedTransformList::ValueFromString(
  const nsAString& aStr,
  const dom::SVGAnimationElement* aSrcElement,
  nsSMILValue& aValue,
  bool& aPreventCachingOfSandwich) const
{
  NS_ENSURE_TRUE(aSrcElement, NS_ERROR_FAILURE);
  NS_ABORT_IF_FALSE(aValue.IsNull(),
    "aValue should have been cleared before calling ValueFromString");

  const nsAttrValue* typeAttr = aSrcElement->GetAnimAttr(nsGkAtoms::type);
  const nsIAtom* transformType = nsGkAtoms::translate; 
  if (typeAttr) {
    if (typeAttr->Type() != nsAttrValue::eAtom) {
      
      
      
      return NS_ERROR_FAILURE;
    }
    transformType = typeAttr->GetAtomValue();
  }

  ParseValue(aStr, transformType, aValue);
  aPreventCachingOfSandwich = false;
  return aValue.IsNull() ? NS_ERROR_FAILURE : NS_OK;
}

void
nsSVGAnimatedTransformList::SMILAnimatedTransformList::ParseValue(
  const nsAString& aSpec,
  const nsIAtom* aTransformType,
  nsSMILValue& aResult)
{
  NS_ABORT_IF_FALSE(aResult.IsNull(), "Unexpected type for SMIL value");

  static_assert(SVGTransformSMILData::NUM_SIMPLE_PARAMS == 3,
                "nsSVGSMILTransform constructor should be expecting array "
                "with 3 params");

  float params[3] = { 0.f };
  int32_t numParsed = ParseParameterList(aSpec, params, 3);
  uint16_t transformType;

  if (aTransformType == nsGkAtoms::translate) {
    
    if (numParsed != 1 && numParsed != 2)
      return;
    transformType = SVG_TRANSFORM_TRANSLATE;
  } else if (aTransformType == nsGkAtoms::scale) {
    
    if (numParsed != 1 && numParsed != 2)
      return;
    if (numParsed == 1) {
      params[1] = params[0];
    }
    transformType = SVG_TRANSFORM_SCALE;
  } else if (aTransformType == nsGkAtoms::rotate) {
    
    if (numParsed != 1 && numParsed != 3)
      return;
    transformType = SVG_TRANSFORM_ROTATE;
  } else if (aTransformType == nsGkAtoms::skewX) {
    
    if (numParsed != 1)
      return;
    transformType = SVG_TRANSFORM_SKEWX;
  } else if (aTransformType == nsGkAtoms::skewY) {
    
    if (numParsed != 1)
      return;
    transformType = SVG_TRANSFORM_SKEWY;
  } else {
    return;
  }

  nsSMILValue val(SVGTransformListSMILType::Singleton());
  SVGTransformSMILData transform(transformType, params);
  if (NS_FAILED(SVGTransformListSMILType::AppendTransform(transform, val))) {
    return; 
  }

  
  aResult.Swap(val);
}

int32_t
nsSVGAnimatedTransformList::SMILAnimatedTransformList::ParseParameterList(
  const nsAString& aSpec,
  float* aVars,
  int32_t aNVars)
{
  nsCharSeparatedTokenizerTemplate<IsSVGWhitespace>
    tokenizer(aSpec, ',', nsCharSeparatedTokenizer::SEPARATOR_OPTIONAL);

  int numArgsFound = 0;

  while (tokenizer.hasMoreTokens()) {
    float f;
    if (!SVGContentUtils::ParseNumber(tokenizer.nextToken(), f)) {
      return -1;    
    }
    if (numArgsFound < aNVars) {
      aVars[numArgsFound] = f;
    }
    numArgsFound++;
  }
  return numArgsFound;
}

nsSMILValue
nsSVGAnimatedTransformList::SMILAnimatedTransformList::GetBaseValue() const
{
  
  
  
  nsSMILValue val(SVGTransformListSMILType::Singleton());
  if (!SVGTransformListSMILType::AppendTransforms(mVal->mBaseVal, val)) {
    val = nsSMILValue();
  }

  return val;
}

nsresult
nsSVGAnimatedTransformList::SMILAnimatedTransformList::SetAnimValue(
  const nsSMILValue& aNewAnimValue)
{
  NS_ABORT_IF_FALSE(
    aNewAnimValue.mType == SVGTransformListSMILType::Singleton(),
    "Unexpected type to assign animated value");
  SVGTransformList animVal;
  if (!SVGTransformListSMILType::GetTransforms(aNewAnimValue,
                                               animVal.mItems)) {
    return NS_ERROR_FAILURE;
  }

  return mVal->SetAnimValue(animVal, mElement);
}

void
nsSVGAnimatedTransformList::SMILAnimatedTransformList::ClearAnimValue()
{
  if (mVal->mAnimVal) {
    mVal->ClearAnimValue(mElement);
  }
}

} 
