






































#include "nsSMILCSSValueType.h"
#include "nsString.h"
#include "nsStyleAnimation.h"
#include "nsStyleCoord.h"
#include "nsSMILParserUtils.h"
#include "nsSMILValue.h"
#include "nsCSSValue.h"
#include "nsCSSDeclaration.h"
#include "nsColor.h"
#include "nsPresContext.h"
#include "nsIContent.h"
#include "nsDebug.h"

 nsSMILCSSValueType nsSMILCSSValueType::sSingleton;

struct ValueWrapper {
  ValueWrapper() : mCSSValue(), mPropID(eCSSProperty_UNKNOWN),
                   mPresContext(nsnull) {}

  nsStyleCoord   mCSSValue;
  nsCSSProperty  mPropID;
  nsPresContext* mPresContext;
};



static const nsStyleCoord sZeroCoord(0);
static const nsStyleCoord sZeroPercent(0.0f, eStyleUnit_Percent);
static const nsStyleCoord sZeroFactor(0.0f,  eStyleUnit_Factor);
static const nsStyleCoord sZeroColor(NS_RGB(0,0,0));



static const nsStyleCoord*
GetZeroValueForUnit(nsStyleUnit aUnit)
{
  NS_ABORT_IF_FALSE(aUnit != eStyleUnit_Null,
                    "Need non-null unit for a zero value.");
  switch (aUnit) {
    case eStyleUnit_Coord:
      return &sZeroCoord;
    case eStyleUnit_Percent:
      return &sZeroPercent;
    case eStyleUnit_Factor:
      return &sZeroFactor;
    case eStyleUnit_Color:
      return &sZeroColor;
    default:
      NS_NOTREACHED("Calling GetZeroValueForUnit with an unsupported unit");
      return nsnull;
  }
}

static void
InvertStyleCoordSign(nsStyleCoord& aStyleCoord)
{
  switch (aStyleCoord.GetUnit()) {
    case eStyleUnit_Coord:
      aStyleCoord.SetCoordValue(-aStyleCoord.GetCoordValue());
      break;
    case eStyleUnit_Percent:
      aStyleCoord.SetPercentValue(-aStyleCoord.GetPercentValue());
      break;
    case eStyleUnit_Factor:
      aStyleCoord.SetFactorValue(-aStyleCoord.GetFactorValue());
      break;
    default:
      NS_NOTREACHED("Calling InvertStyleCoordSign with an unsupported unit");
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



nsresult
nsSMILCSSValueType::Init(nsSMILValue& aValue) const
{
  NS_ABORT_IF_FALSE(aValue.IsNull(),
                    "Unexpected value type");

  aValue.mU.mPtr = new ValueWrapper();
  if (!aValue.mU.mPtr) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  aValue.mType = this;
  return NS_OK;
}

void
nsSMILCSSValueType::Destroy(nsSMILValue& aValue) const
{
  NS_ABORT_IF_FALSE(aValue.mType == this, "Unexpected SMIL value type");
  NS_ABORT_IF_FALSE(aValue.mU.mPtr,
                    "nsSMILValue for CSS val should have non-null pointer");

  delete static_cast<ValueWrapper*>(aValue.mU.mPtr);
  aValue.mU.mPtr    = nsnull;
  aValue.mType      = &nsSMILNullType::sSingleton;
}

nsresult
nsSMILCSSValueType::Assign(nsSMILValue& aDest, const nsSMILValue& aSrc) const
{
  NS_ABORT_IF_FALSE(aDest.mType == aSrc.mType, "Incompatible SMIL types");
  NS_ABORT_IF_FALSE(aDest.mType == this, "Unexpected SMIL value type");
  NS_ABORT_IF_FALSE(aDest.mU.mPtr,
                    "nsSMILValue for CSS val should have non-null pointer");
  NS_ABORT_IF_FALSE(aSrc.mU.mPtr,
                    "nsSMILValue for CSS val should have non-null pointer");
  const ValueWrapper* srcWrapper = ExtractValueWrapper(aSrc);
  ValueWrapper* destWrapper = ExtractValueWrapper(aDest);
  *destWrapper = *srcWrapper; 

  return NS_OK;
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

  NS_ABORT_IF_FALSE(destWrapper && valueToAddWrapper,
                    "these pointers shouldn't be null");

  if (destWrapper->mPropID == eCSSProperty_UNKNOWN) {
    NS_ABORT_IF_FALSE(destWrapper->mCSSValue.IsNull(),
                      "If property ID is unset, then the unit should be, too");
    
    destWrapper->mCSSValue =
      *GetZeroValueForUnit(valueToAddWrapper->mCSSValue.GetUnit());
    destWrapper->mPropID = valueToAddWrapper->mPropID;
    destWrapper->mPresContext = valueToAddWrapper->mPresContext;
  }
  NS_ABORT_IF_FALSE(valueToAddWrapper->mPropID != eCSSProperty_UNKNOWN &&
                    !valueToAddWrapper->mCSSValue.IsNull(),
                    "Added amount should be a parsed value");

  
  if (destWrapper->mPropID == eCSSProperty_font_size_adjust) {
    return NS_ERROR_FAILURE;
  }
  return nsStyleAnimation::Add(destWrapper->mCSSValue,
                               valueToAddWrapper->mCSSValue, aCount) ?
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
  NS_ABORT_IF_FALSE(fromWrapper && toWrapper,
                    "These pointers shouldn't be null");

  const nsStyleCoord* fromCSSValue;
  if (fromWrapper->mPropID == eCSSProperty_UNKNOWN) {
    NS_ABORT_IF_FALSE(fromWrapper->mCSSValue.IsNull(),
                      "If property ID is unset, then the unit should be, too");
    fromCSSValue = GetZeroValueForUnit(toWrapper->mCSSValue.GetUnit());
  } else {
    fromCSSValue = &fromWrapper->mCSSValue;
  }
  NS_ABORT_IF_FALSE(toWrapper->mPropID != eCSSProperty_UNKNOWN &&
                    !toWrapper->mCSSValue.IsNull(),
                    "ComputeDistance endpoint should be a parsed value");

  return nsStyleAnimation::ComputeDistance(*fromCSSValue, toWrapper->mCSSValue,
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

  const ValueWrapper* startWrapper = ExtractValueWrapper(aStartVal);
  const ValueWrapper* endWrapper = ExtractValueWrapper(aEndVal);
  ValueWrapper* resultWrapper = ExtractValueWrapper(aResult);

  NS_ABORT_IF_FALSE(startWrapper && endWrapper && resultWrapper,
                    "These pointers shouldn't be null");

  const nsStyleCoord* startCSSValue;
  if (startWrapper->mPropID == eCSSProperty_UNKNOWN) {
    NS_ABORT_IF_FALSE(startWrapper->mCSSValue.IsNull(),
                      "If property ID is unset, then the unit should be, too");
    startCSSValue = GetZeroValueForUnit(endWrapper->mCSSValue.GetUnit());
  } else {
    startCSSValue = &startWrapper->mCSSValue;
  }
  NS_ABORT_IF_FALSE(endWrapper->mPropID != eCSSProperty_UNKNOWN &&
                    !endWrapper->mCSSValue.IsNull(),
                    "Interpolate endpoint should be a parsed value");

  if (nsStyleAnimation::Interpolate(*startCSSValue,
                                    endWrapper->mCSSValue,
                                    aUnitDistance,
                                    resultWrapper->mCSSValue)) {
    resultWrapper->mPropID = endWrapper->mPropID;
    resultWrapper->mPresContext = endWrapper->mPresContext;
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

PRBool
nsSMILCSSValueType::ValueFromString(nsCSSProperty aPropID,
                                    nsIContent* aTargetElement,
                                    const nsAString& aString,
                                    nsSMILValue& aValue) const
{
  NS_ABORT_IF_FALSE(aValue.mType == &nsSMILCSSValueType::sSingleton,
                    "Passed-in value is wrong type");
  NS_ABORT_IF_FALSE(aPropID < eCSSProperty_COUNT_no_shorthands,
                    "nsSMILCSSValueType shouldn't be used with "
                    "shorthand properties");

  
  
  
  
  PRBool isNegative = PR_FALSE;
  PRUint32 subStringBegin = 0;
  PRInt32 absValuePos = nsSMILParserUtils::CheckForNegativeNumber(aString);
  if (absValuePos > 0) {
    subStringBegin = (PRUint32)absValuePos;
    isNegative = PR_TRUE;
  }
  nsDependentSubstring subString(aString, subStringBegin);
  ValueWrapper* wrapper = ExtractValueWrapper(aValue);
  NS_ABORT_IF_FALSE(wrapper, "Wrapper should be non-null if type is set.");

  if (nsStyleAnimation::ComputeValue(aPropID, aTargetElement,
                                     subString, wrapper->mCSSValue)) {
    wrapper->mPropID = aPropID;
    if (isNegative) {
      InvertStyleCoordSign(wrapper->mCSSValue);
    }
    
    nsIDocument* doc = aTargetElement->GetCurrentDoc();
    NS_ABORT_IF_FALSE(doc, "active animations should only be able to "
                      "target elements that are in a document");
    nsIPresShell* shell = doc->GetPrimaryShell();
    if (shell) {
      wrapper->mPresContext = shell->GetPresContext();
    }
    if (wrapper->mPresContext) {
      
      if (aPropID == eCSSProperty_font_size) {
        NS_ABORT_IF_FALSE(wrapper->mCSSValue.GetUnit() == eStyleUnit_Coord,
                          "'font-size' value with unexpected style unit");
        wrapper->mCSSValue.SetCoordValue(wrapper->mCSSValue.GetCoordValue() /
                                         wrapper->mPresContext->TextZoom());
      }
      return PR_TRUE;
    }
    
    NS_NOTREACHED("Not parsing animation value; unable to get PresContext");
    
    
    Destroy(aValue);
    Init(aValue);
  }
  return PR_FALSE;
}

PRBool
nsSMILCSSValueType::ValueToString(const nsSMILValue& aValue,
                                  nsAString& aString) const
{
  NS_ABORT_IF_FALSE(aValue.mType == &nsSMILCSSValueType::sSingleton,
                    "Passed-in value is wrong type");

  const ValueWrapper* wrapper = ExtractValueWrapper(aValue);
  return nsStyleAnimation::UncomputeValue(wrapper->mPropID,
                                          wrapper->mPresContext,
                                          wrapper->mCSSValue, aString);
}
