



































#include "nsSVGInteger.h"
#ifdef MOZ_SMIL
#include "nsSMILValue.h"
#include "SMILIntegerType.h"
#endif 

using namespace mozilla;

NS_SVG_VAL_IMPL_CYCLE_COLLECTION(nsSVGInteger::DOMAnimatedInteger, mSVGElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsSVGInteger::DOMAnimatedInteger)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsSVGInteger::DOMAnimatedInteger)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsSVGInteger::DOMAnimatedInteger)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedInteger)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGAnimatedInteger)
NS_INTERFACE_MAP_END



nsresult
nsSVGInteger::SetBaseValueString(const nsAString &aValueAsString,
                                 nsSVGElement *aSVGElement,
                                 PRBool aDoSetAttr)
{
  NS_ConvertUTF16toUTF8 value(aValueAsString);
  const char *str = value.get();

  if (NS_IsAsciiWhitespace(*str))
    return NS_ERROR_DOM_SYNTAX_ERR;
  
  char *rest;
  PRInt32 val = strtol(str, &rest, 10);
  if (rest == str || *rest != '\0') {
    return NS_ERROR_DOM_SYNTAX_ERR;
  }

  if (val != mBaseVal) {
    mBaseVal = mAnimVal = val;
#ifdef MOZ_SMIL
    if (mIsAnimated) {
      aSVGElement->AnimationNeedsResample();
    }
#endif
  }
  return NS_OK;
}

void
nsSVGInteger::GetBaseValueString(nsAString & aValueAsString)
{
  nsAutoString s;
  s.AppendInt(mBaseVal);
  aValueAsString.Assign(s);
}

void
nsSVGInteger::SetBaseValue(int aValue,
                           nsSVGElement *aSVGElement,
                           PRBool aDoSetAttr)
{
  if (aValue != mBaseVal) {
    mBaseVal = mAnimVal = aValue;
    aSVGElement->DidChangeInteger(mAttrEnum, aDoSetAttr);
#ifdef MOZ_SMIL
    if (mIsAnimated) {
      aSVGElement->AnimationNeedsResample();
    }
#endif
  }
}

void
nsSVGInteger::SetAnimValue(int aValue, nsSVGElement *aSVGElement)
{
  mAnimVal = aValue;
  mIsAnimated = PR_TRUE;
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
                                           nsSMILValue& aValue) const
{
  NS_ConvertUTF16toUTF8 value(aStr);
  const char *str = value.get();

  if (NS_IsAsciiWhitespace(*str))
    return NS_ERROR_FAILURE;

  char *rest;
  PRInt32 val = strtol(str, &rest, 10);
  if (rest == str || *rest != '\0') {
    return NS_ERROR_FAILURE;
  }

  nsSMILValue smilVal(&SMILIntegerType::sSingleton);
  smilVal.mU.mInt = val;
  aValue = smilVal;
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
    mVal->mIsAnimated = PR_FALSE;
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
