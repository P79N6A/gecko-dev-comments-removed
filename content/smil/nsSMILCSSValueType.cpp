






































#include "nsSMILCSSValueType.h"
#include "nsString.h"
#include "nsStyleAnimation.h"
#include "nsSMILParserUtils.h"
#include "nsSMILValue.h"
#include "nsCSSValue.h"
#include "nsColor.h"
#include "nsPresContext.h"
#include "mozilla/dom/Element.h"
#include "nsDebug.h"

using namespace mozilla::dom;

 nsSMILCSSValueType nsSMILCSSValueType::sSingleton;

struct ValueWrapper {
  ValueWrapper(nsCSSProperty aPropID, const nsStyleAnimation::Value& aValue,
               nsPresContext* aPresContext) :
    mPropID(aPropID), mCSSValue(aValue), mPresContext(aPresContext) {}

  nsCSSProperty mPropID;
  nsStyleAnimation::Value mCSSValue;
  nsPresContext* mPresContext;
};



static const nsStyleAnimation::Value
  sZeroCoord(0, nsStyleAnimation::Value::CoordConstructor);
static const nsStyleAnimation::Value
  sZeroPercent(0.0f, nsStyleAnimation::Value::PercentConstructor);
static const nsStyleAnimation::Value
  sZeroFloat(0.0f,  nsStyleAnimation::Value::FloatConstructor);
static const nsStyleAnimation::Value
  sZeroColor(NS_RGB(0,0,0), nsStyleAnimation::Value::ColorConstructor);



static const nsStyleAnimation::Value*
GetZeroValueForUnit(nsStyleAnimation::Unit aUnit)
{
  NS_ABORT_IF_FALSE(aUnit != nsStyleAnimation::eUnit_Null,
                    "Need non-null unit for a zero value");
  switch (aUnit) {
    case nsStyleAnimation::eUnit_Coord:
      return &sZeroCoord;
    case nsStyleAnimation::eUnit_Percent:
      return &sZeroPercent;
    case nsStyleAnimation::eUnit_Float:
      return &sZeroFloat;
    case nsStyleAnimation::eUnit_Color:
      return &sZeroColor;
    default:
      return nsnull;
  }
}











static const bool
FinalizeStyleAnimationValues(const nsStyleAnimation::Value*& aValue1,
                             const nsStyleAnimation::Value*& aValue2)
{
  NS_ABORT_IF_FALSE(aValue1 || aValue2,
                    "expecting at least one non-null value");

  
  if (!aValue1) {
    aValue1 = GetZeroValueForUnit(aValue2->GetUnit());
    return !!aValue1; 
  }
  if (!aValue2) {
    aValue2 = GetZeroValueForUnit(aValue1->GetUnit());
    return !!aValue2; 
  }

  
  
  
  
  
  if (*aValue1 == sZeroCoord &&
      aValue2->GetUnit() == nsStyleAnimation::eUnit_Float) {
    aValue1 = &sZeroFloat;
  } else if (*aValue2 == sZeroCoord &&
             aValue1->GetUnit() == nsStyleAnimation::eUnit_Float) {
    aValue2 = &sZeroFloat;
  }

  return true;
}

static void
InvertSign(nsStyleAnimation::Value& aValue)
{
  switch (aValue.GetUnit()) {
    case nsStyleAnimation::eUnit_Coord:
      aValue.SetCoordValue(-aValue.GetCoordValue());
      break;
    case nsStyleAnimation::eUnit_Percent:
      aValue.SetPercentValue(-aValue.GetPercentValue());
      break;
    case nsStyleAnimation::eUnit_Float:
      aValue.SetFloatValue(-aValue.GetFloatValue());
      break;
    default:
      NS_NOTREACHED("Calling InvertSign with an unsupported unit");
      break;
  }
}

static ValueWrapper*
ExtractValueWrapper(nsSMILValue& aValue)
{
  return static_cast<ValueWrapper*>(aValue.mU.mPtr);
}

static const ValueWrapper*
ExtractValueWrapper(const nsSMILValue& aValue)
{
  return static_cast<const ValueWrapper*>(aValue.mU.mPtr);
}



void
nsSMILCSSValueType::Init(nsSMILValue& aValue) const
{
  NS_ABORT_IF_FALSE(aValue.IsNull(), "Unexpected SMIL value type");

  aValue.mU.mPtr = nsnull;
  aValue.mType = this;
}

void
nsSMILCSSValueType::Destroy(nsSMILValue& aValue) const
{
  NS_ABORT_IF_FALSE(aValue.mType == this, "Unexpected SMIL value type");
  delete static_cast<ValueWrapper*>(aValue.mU.mPtr);
  aValue.mType = &nsSMILNullType::sSingleton;
}

nsresult
nsSMILCSSValueType::Assign(nsSMILValue& aDest, const nsSMILValue& aSrc) const
{
  NS_ABORT_IF_FALSE(aDest.mType == aSrc.mType, "Incompatible SMIL types");
  NS_ABORT_IF_FALSE(aDest.mType == this, "Unexpected SMIL value type");
  const ValueWrapper* srcWrapper = ExtractValueWrapper(aSrc);
  ValueWrapper* destWrapper = ExtractValueWrapper(aDest);

  if (srcWrapper) {
    if (!destWrapper) {
      
      aDest.mU.mPtr = new ValueWrapper(*srcWrapper);
    } else {
      
      *destWrapper = *srcWrapper;
    }
  } else if (destWrapper) {
    
    delete destWrapper;
    aDest.mU.mPtr = destWrapper = nsnull;
  } 

  return NS_OK;
}

bool
nsSMILCSSValueType::IsEqual(const nsSMILValue& aLeft,
                            const nsSMILValue& aRight) const
{
  NS_ABORT_IF_FALSE(aLeft.mType == aRight.mType, "Incompatible SMIL types");
  NS_ABORT_IF_FALSE(aLeft.mType == this, "Unexpected SMIL value");
  const ValueWrapper* leftWrapper = ExtractValueWrapper(aLeft);
  const ValueWrapper* rightWrapper = ExtractValueWrapper(aRight);

  if (leftWrapper) {
    if (rightWrapper) {
      
      NS_WARN_IF_FALSE(leftWrapper != rightWrapper,
                       "Two nsSMILValues with matching ValueWrapper ptr");
      
      return (leftWrapper->mPropID == rightWrapper->mPropID &&
              leftWrapper->mCSSValue == rightWrapper->mCSSValue);
    }
    
    return false;
  }
  if (rightWrapper) {
    
    return false;
  }
  
  return true;
}

nsresult
nsSMILCSSValueType::Add(nsSMILValue& aDest, const nsSMILValue& aValueToAdd,
                        PRUint32 aCount) const
{
  NS_ABORT_IF_FALSE(aValueToAdd.mType == aDest.mType,
                    "Trying to add invalid types");
  NS_ABORT_IF_FALSE(aValueToAdd.mType == this, "Unexpected source type");

  ValueWrapper* destWrapper = ExtractValueWrapper(aDest);
  const ValueWrapper* valueToAddWrapper = ExtractValueWrapper(aValueToAdd);
  NS_ABORT_IF_FALSE(destWrapper || valueToAddWrapper,
                    "need at least one fully-initialized value");

  nsCSSProperty property = (valueToAddWrapper ? valueToAddWrapper->mPropID :
                            destWrapper->mPropID);
  
  
  if (property == eCSSProperty_font_size_adjust ||
      property == eCSSProperty_stroke_dasharray) {
    return NS_ERROR_FAILURE;
  }

  const nsStyleAnimation::Value* valueToAdd = valueToAddWrapper ?
    &valueToAddWrapper->mCSSValue : nsnull;
  const nsStyleAnimation::Value* destValue = destWrapper ?
    &destWrapper->mCSSValue : nsnull;
  if (!FinalizeStyleAnimationValues(valueToAdd, destValue)) {
    return NS_ERROR_FAILURE;
  }
  
  
  if (destWrapper && &destWrapper->mCSSValue != destValue) {
    destWrapper->mCSSValue = *destValue;
  }

  
  if (!destWrapper) {
    aDest.mU.mPtr = destWrapper =
      new ValueWrapper(property, *destValue, valueToAddWrapper->mPresContext);
  }

  return nsStyleAnimation::Add(property,
                               destWrapper->mCSSValue, *valueToAdd, aCount) ?
    NS_OK : NS_ERROR_FAILURE;
}

nsresult
nsSMILCSSValueType::ComputeDistance(const nsSMILValue& aFrom,
                                    const nsSMILValue& aTo,
                                    double& aDistance) const
{
  NS_ABORT_IF_FALSE(aFrom.mType == aTo.mType,
                    "Trying to compare different types");
  NS_ABORT_IF_FALSE(aFrom.mType == this, "Unexpected source type");

  const ValueWrapper* fromWrapper = ExtractValueWrapper(aFrom);
  const ValueWrapper* toWrapper = ExtractValueWrapper(aTo);
  NS_ABORT_IF_FALSE(toWrapper, "expecting non-null endpoint");

  const nsStyleAnimation::Value* fromCSSValue = fromWrapper ?
    &fromWrapper->mCSSValue : nsnull;
  const nsStyleAnimation::Value* toCSSValue = &toWrapper->mCSSValue;
  if (!FinalizeStyleAnimationValues(fromCSSValue, toCSSValue)) {
    return NS_ERROR_FAILURE;
  }

  return nsStyleAnimation::ComputeDistance(toWrapper->mPropID,
                                           *fromCSSValue, *toCSSValue,
                                           aDistance) ?
    NS_OK : NS_ERROR_FAILURE;
}

nsresult
nsSMILCSSValueType::Interpolate(const nsSMILValue& aStartVal,
                                const nsSMILValue& aEndVal,
                                double aUnitDistance,
                                nsSMILValue& aResult) const
{
  NS_ABORT_IF_FALSE(aStartVal.mType == aEndVal.mType,
                    "Trying to interpolate different types");
  NS_ABORT_IF_FALSE(aStartVal.mType == this,
                    "Unexpected types for interpolation");
  NS_ABORT_IF_FALSE(aResult.mType == this, "Unexpected result type");
  NS_ABORT_IF_FALSE(aUnitDistance >= 0.0 && aUnitDistance <= 1.0,
                    "unit distance value out of bounds");
  NS_ABORT_IF_FALSE(!aResult.mU.mPtr, "expecting barely-initialized outparam");

  const ValueWrapper* startWrapper = ExtractValueWrapper(aStartVal);
  const ValueWrapper* endWrapper = ExtractValueWrapper(aEndVal);
  NS_ABORT_IF_FALSE(endWrapper, "expecting non-null endpoint");

  const nsStyleAnimation::Value* startCSSValue = startWrapper ?
    &startWrapper->mCSSValue : nsnull;
  const nsStyleAnimation::Value* endCSSValue = &endWrapper->mCSSValue;
  if (!FinalizeStyleAnimationValues(startCSSValue, endCSSValue)) {
    return NS_ERROR_FAILURE;
  }

  nsStyleAnimation::Value resultValue;
  if (nsStyleAnimation::Interpolate(endWrapper->mPropID,
                                    *startCSSValue, *endCSSValue,
                                    aUnitDistance, resultValue)) {
    aResult.mU.mPtr = new ValueWrapper(endWrapper->mPropID, resultValue,
                                       endWrapper->mPresContext);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}


static nsPresContext*
GetPresContextForElement(Element* aElem)
{
  nsIDocument* doc = aElem->GetCurrentDoc();
  if (!doc) {
    
    
    
    return nsnull;
  }
  nsIPresShell* shell = doc->GetShell();
  return shell ? shell->GetPresContext() : nsnull;
}


static bool
ValueFromStringHelper(nsCSSProperty aPropID,
                      Element* aTargetElement,
                      nsPresContext* aPresContext,
                      const nsAString& aString,
                      nsStyleAnimation::Value& aStyleAnimValue,
                      bool* aIsContextSensitive)
{
  
  
  
  
  bool isNegative = false;
  PRUint32 subStringBegin = 0;
  PRInt32 absValuePos = nsSMILParserUtils::CheckForNegativeNumber(aString);
  if (absValuePos > 0) {
    isNegative = true;
    subStringBegin = (PRUint32)absValuePos; 
  }
  nsDependentSubstring subString(aString, subStringBegin);
  if (!nsStyleAnimation::ComputeValue(aPropID, aTargetElement, subString,
                                      true, aStyleAnimValue,
                                      aIsContextSensitive)) {
    return false;
  }
  if (isNegative) {
    InvertSign(aStyleAnimValue);
  }
  
  if (aPropID == eCSSProperty_font_size) {
    
    NS_ABORT_IF_FALSE(aStyleAnimValue.GetUnit() ==
                        nsStyleAnimation::eUnit_Coord,
                      "'font-size' value with unexpected style unit");
    aStyleAnimValue.SetCoordValue(aStyleAnimValue.GetCoordValue() /
                                  aPresContext->TextZoom());
  }
  return true;
}


void
nsSMILCSSValueType::ValueFromString(nsCSSProperty aPropID,
                                    Element* aTargetElement,
                                    const nsAString& aString,
                                    nsSMILValue& aValue,
                                    bool* aIsContextSensitive)
{
  NS_ABORT_IF_FALSE(aValue.IsNull(), "Outparam should be null-typed");
  nsPresContext* presContext = GetPresContextForElement(aTargetElement);
  if (!presContext) {
    NS_WARNING("Not parsing animation value; unable to get PresContext");
    return;
  }

  nsStyleAnimation::Value parsedValue;
  if (ValueFromStringHelper(aPropID, aTargetElement, presContext,
                            aString, parsedValue, aIsContextSensitive)) {
    sSingleton.Init(aValue);
    aValue.mU.mPtr = new ValueWrapper(aPropID, parsedValue, presContext);
  }
}


bool
nsSMILCSSValueType::ValueToString(const nsSMILValue& aValue,
                                  nsAString& aString)
{
  NS_ABORT_IF_FALSE(aValue.mType == &nsSMILCSSValueType::sSingleton,
                    "Unexpected SMIL value type");
  const ValueWrapper* wrapper = ExtractValueWrapper(aValue);
  return !wrapper ||
    nsStyleAnimation::UncomputeValue(wrapper->mPropID, wrapper->mPresContext,
                                     wrapper->mCSSValue, aString);
}
