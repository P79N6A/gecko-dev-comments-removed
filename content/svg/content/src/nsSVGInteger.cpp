



































#include "nsSVGInteger.h"
#ifdef MOZ_SMIL
#include "nsSMILValue.h"
#include "SMILIntegerType.h"
#endif 

using namespace mozilla;

NS_SVG_VAL_IMPL_CYCLE_COLLECTION(nsSVGInteger::DOMAnimatedInteger, mSVGElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsSVGInteger::DOMAnimatedInteger)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsSVGInteger::DOMAnimatedInteger)

DOMCI_DATA(SVGAnimatedInteger, nsSVGInteger::DOMAnimatedInteger)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsSVGInteger::DOMAnimatedInteger)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedInteger)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGAnimatedInteger)
NS_INTERFACE_MAP_END



static nsresult
GetValueFromString(const nsAString &aValueAsString,
                   PRInt32 *aValue)
{
  NS_ConvertUTF16toUTF8 value(aValueAsString);
  const char *str = value.get();

  if (NS_IsAsciiWhitespace(*str))
    return NS_ERROR_DOM_SYNTAX_ERR;
  
  char *rest;
  *aValue = strtol(str, &rest, 10);
  if (rest == str || *rest != '\0') {
    return NS_ERROR_DOM_SYNTAX_ERR;
  }
  if (*rest == '\0') {
    return NS_OK;
  }
  return NS_ERROR_DOM_SYNTAX_ERR;
}

nsresult
nsSVGInteger::SetBaseValueString(const nsAString &aValueAsString,
                                 nsSVGElement *aSVGElement)
{
  PRInt32 value;

  nsresult rv = GetValueFromString(aValueAsString, &value);
  if (NS_FAILED(rv)) {
    return rv;
  }

  mIsBaseSet = true;
  mBaseVal = value;
  if (!mIsAnimated) {
    mAnimVal = mBaseVal;
  }
#ifdef MOZ_SMIL
  else {
    aSVGElement->AnimationNeedsResample();
  }
#endif
  return NS_OK;
}

void
nsSVGInteger::GetBaseValueString(nsAString & aValueAsString)
{
  aValueAsString.Truncate();
  aValueAsString.AppendInt(mBaseVal);
}

void
nsSVGInteger::SetBaseValue(int aValue,
                           nsSVGElement *aSVGElement)
{
  mBaseVal = aValue;
  mIsBaseSet = true;
  if (!mIsAnimated) {
    mAnimVal = mBaseVal;
  }
#ifdef MOZ_SMIL
  else {
    aSVGElement->AnimationNeedsResample();
  }
#endif
  aSVGElement->DidChangeInteger(mAttrEnum, true);
}

void
nsSVGInteger::SetAnimValue(int aValue, nsSVGElement *aSVGElement)
{
  mAnimVal = aValue;
  mIsAnimated = true;
  aSVGElement->DidAnimateInteger(mAttrEnum);
}

nsresult
nsSVGInteger::ToDOMAnimatedInteger(nsIDOMSVGAnimatedInteger **aResult,
                                   nsSVGElement *aSVGElement)
{
  *aResult = new DOMAnimatedInteger(this, aSVGElement);
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult);
  return NS_OK;
}

#ifdef MOZ_SMIL
nsISMILAttr*
nsSVGInteger::ToSMILAttr(nsSVGElement *aSVGElement)
{
  return new SMILInteger(this, aSVGElement);
}

nsresult
nsSVGInteger::SMILInteger::ValueFromString(const nsAString& aStr,
                                           const nsISMILAnimationElement* ,
                                           nsSMILValue& aValue,
                                           bool& aPreventCachingOfSandwich) const
{
  PRInt32 val;

  nsresult rv = GetValueFromString(aStr, &val);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsSMILValue smilVal(&SMILIntegerType::sSingleton);
  smilVal.mU.mInt = val;
  aValue = smilVal;
  aPreventCachingOfSandwich = false;
  return NS_OK;
}

nsSMILValue
nsSVGInteger::SMILInteger::GetBaseValue() const
{
  nsSMILValue val(&SMILIntegerType::sSingleton);
  val.mU.mInt = mVal->mBaseVal;
  return val;
}

void
nsSVGInteger::SMILInteger::ClearAnimValue()
{
  if (mVal->mIsAnimated) {
    mVal->SetAnimValue(mVal->mBaseVal, mSVGElement);
    mVal->mIsAnimated = false;
  }
}

nsresult
nsSVGInteger::SMILInteger::SetAnimValue(const nsSMILValue& aValue)
{
  NS_ASSERTION(aValue.mType == &SMILIntegerType::sSingleton,
               "Unexpected type to assign animated value");
  if (aValue.mType == &SMILIntegerType::sSingleton) {
    mVal->SetAnimValue(int(aValue.mU.mInt), mSVGElement);
  }
  return NS_OK;
}
#endif 
