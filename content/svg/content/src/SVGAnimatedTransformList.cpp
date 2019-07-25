



































#include "SVGAnimatedTransformList.h"
#include "DOMSVGAnimatedTransformList.h"

#ifdef MOZ_SMIL
#include "nsSMILValue.h"
#include "SVGTransform.h"
#include "SVGTransformListSMILType.h"
#include "nsSVGUtils.h"
#include "prdtoa.h"
#endif 

namespace mozilla {

nsresult
SVGAnimatedTransformList::SetBaseValueString(const nsAString& aValue)
{
  SVGTransformList newBaseValue;
  nsresult rv = newBaseValue.SetValueFromString(aValue);
  if (NS_FAILED(rv)) {
    return rv;
  }

  DOMSVGAnimatedTransformList *domWrapper =
    DOMSVGAnimatedTransformList::GetDOMWrapperIfExists(this);
  if (domWrapper) {
    
    
    
    
    
    domWrapper->InternalBaseValListWillChangeLengthTo(newBaseValue.Length());
  }

  
  
  

  rv = mBaseVal.CopyFrom(newBaseValue);
  if (NS_FAILED(rv) && domWrapper) {
    
    
    domWrapper->InternalBaseValListWillChangeLengthTo(mBaseVal.Length());
  } else {
    mIsAttrSet = true;
  }
  return rv;
}

void
SVGAnimatedTransformList::ClearBaseValue()
{
  DOMSVGAnimatedTransformList *domWrapper =
    DOMSVGAnimatedTransformList::GetDOMWrapperIfExists(this);
  if (domWrapper) {
    
    domWrapper->InternalBaseValListWillChangeLengthTo(0);
  }
  mBaseVal.Clear();
  mIsAttrSet = false;
  
}

nsresult
SVGAnimatedTransformList::SetAnimValue(const SVGTransformList& aValue,
                                       nsSVGElement *aElement)
{
  DOMSVGAnimatedTransformList *domWrapper =
    DOMSVGAnimatedTransformList::GetDOMWrapperIfExists(this);
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
  aElement->DidAnimateTransformList();
  return NS_OK;
}

void
SVGAnimatedTransformList::ClearAnimValue(nsSVGElement *aElement)
{
  DOMSVGAnimatedTransformList *domWrapper =
    DOMSVGAnimatedTransformList::GetDOMWrapperIfExists(this);
  if (domWrapper) {
    
    
    
    
    
    domWrapper->InternalAnimValListWillChangeLengthTo(mBaseVal.Length());
  }
  mAnimVal = nsnull;
  aElement->DidAnimateTransformList();
}

bool
SVGAnimatedTransformList::IsExplicitlySet() const
{
  
  
  
  
  
  
  
  
  
  
  
  return mIsAttrSet || !mBaseVal.IsEmpty() || mAnimVal;
}

#ifdef MOZ_SMIL
nsISMILAttr*
SVGAnimatedTransformList::ToSMILAttr(nsSVGElement* aSVGElement)
{
  return new SMILAnimatedTransformList(this, aSVGElement);
}

nsresult
SVGAnimatedTransformList::SMILAnimatedTransformList::ValueFromString(
  const nsAString& aStr,
  const nsISMILAnimationElement* aSrcElement,
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
SVGAnimatedTransformList::SMILAnimatedTransformList::ParseValue(
  const nsAString& aSpec,
  const nsIAtom* aTransformType,
  nsSMILValue& aResult)
{
  NS_ABORT_IF_FALSE(aResult.IsNull(), "Unexpected type for SMIL value");

  
  PR_STATIC_ASSERT(SVGTransformSMILData::NUM_SIMPLE_PARAMS == 3);

  float params[3] = { 0.f };
  PRInt32 numParsed = ParseParameterList(aSpec, params, 3);
  PRUint16 transformType;

  if (aTransformType == nsGkAtoms::translate) {
    
    if (numParsed != 1 && numParsed != 2)
      return;
    transformType = nsIDOMSVGTransform::SVG_TRANSFORM_TRANSLATE;
  } else if (aTransformType == nsGkAtoms::scale) {
    
    if (numParsed != 1 && numParsed != 2)
      return;
    if (numParsed == 1) {
      params[1] = params[0];
    }
    transformType = nsIDOMSVGTransform::SVG_TRANSFORM_SCALE;
  } else if (aTransformType == nsGkAtoms::rotate) {
    
    if (numParsed != 1 && numParsed != 3)
      return;
    transformType = nsIDOMSVGTransform::SVG_TRANSFORM_ROTATE;
  } else if (aTransformType == nsGkAtoms::skewX) {
    
    if (numParsed != 1)
      return;
    transformType = nsIDOMSVGTransform::SVG_TRANSFORM_SKEWX;
  } else if (aTransformType == nsGkAtoms::skewY) {
    
    if (numParsed != 1)
      return;
    transformType = nsIDOMSVGTransform::SVG_TRANSFORM_SKEWY;
  } else {
    return;
  }

  nsSMILValue val(&SVGTransformListSMILType::sSingleton);
  SVGTransformSMILData transform(transformType, params);
  if (NS_FAILED(SVGTransformListSMILType::AppendTransform(transform, val))) {
    return; 
  }

  
  aResult.Swap(val);
}

namespace
{
  inline void
  SkipWsp(nsACString::const_iterator& aIter,
          const nsACString::const_iterator& aIterEnd)
  {
    while (aIter != aIterEnd && IsSVGWhitespace(*aIter))
      ++aIter;
  }
} 

PRInt32
SVGAnimatedTransformList::SMILAnimatedTransformList::ParseParameterList(
  const nsAString& aSpec,
  float* aVars,
  PRInt32 aNVars)
{
  NS_ConvertUTF16toUTF8 spec(aSpec);

  nsACString::const_iterator start, end;
  spec.BeginReading(start);
  spec.EndReading(end);

  SkipWsp(start, end);

  int numArgsFound = 0;

  while (start != end) {
    char const *arg = start.get();
    char *argend;
    float f = float(PR_strtod(arg, &argend));
    if (arg == argend || argend > end.get() || !NS_finite(f))
      return -1;

    if (numArgsFound < aNVars) {
      aVars[numArgsFound] = f;
    }

    start.advance(argend - arg);
    numArgsFound++;

    SkipWsp(start, end);
    if (*start == ',') {
      ++start;
      SkipWsp(start, end);
    }
  }

  return numArgsFound;
}

nsSMILValue
SVGAnimatedTransformList::SMILAnimatedTransformList::GetBaseValue() const
{
  
  
  
  nsSMILValue val(&SVGTransformListSMILType::sSingleton);
  if (!SVGTransformListSMILType::AppendTransforms(mVal->mBaseVal, val)) {
    val = nsSMILValue();
  }

  return val;
}

nsresult
SVGAnimatedTransformList::SMILAnimatedTransformList::SetAnimValue(
  const nsSMILValue& aNewAnimValue)
{
  NS_ABORT_IF_FALSE(
    aNewAnimValue.mType == &SVGTransformListSMILType::sSingleton,
    "Unexpected type to assign animated value");
  SVGTransformList animVal;
  if (!SVGTransformListSMILType::GetTransforms(aNewAnimValue,
                                               animVal.mItems)) {
    return NS_ERROR_FAILURE;
  }

  return mVal->SetAnimValue(animVal, mElement);
}

void
SVGAnimatedTransformList::SMILAnimatedTransformList::ClearAnimValue()
{
  if (mVal->mAnimVal) {
    mVal->ClearAnimValue(mElement);
  }
}

#endif 

} 
