



































#include "nsSVGBoolean.h"
#ifdef MOZ_SMIL
#include "nsSMILValue.h"
#include "SMILBoolType.h"
#endif 

using namespace mozilla;

NS_SVG_VAL_IMPL_CYCLE_COLLECTION(nsSVGBoolean::DOMAnimatedBoolean, mSVGElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsSVGBoolean::DOMAnimatedBoolean)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsSVGBoolean::DOMAnimatedBoolean)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsSVGBoolean::DOMAnimatedBoolean)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedBoolean)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGAnimatedBoolean)
NS_INTERFACE_MAP_END



nsresult
nsSVGBoolean::SetBaseValueString(const nsAString &aValueAsString,
                                 nsSVGElement *aSVGElement,
                                 PRBool aDoSetAttr)
{
  PRBool val;

  if (aValueAsString.EqualsLiteral("true"))
    val = PR_TRUE;
  else if (aValueAsString.EqualsLiteral("false"))
    val = PR_FALSE;
  else
    return NS_ERROR_DOM_SYNTAX_ERR;

  mBaseVal = val;
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
nsSVGBoolean::GetBaseValueString(nsAString & aValueAsString)
{
  aValueAsString.Assign(mBaseVal
                        ? NS_LITERAL_STRING("true")
                        : NS_LITERAL_STRING("false"));
}

void
nsSVGBoolean::SetBaseValue(PRBool aValue,
                           nsSVGElement *aSVGElement)
{
  NS_PRECONDITION(aValue == PR_TRUE || aValue == PR_FALSE, "Boolean out of range");

  if (aValue != mBaseVal) {
    mBaseVal = aValue;
    if (!mIsAnimated) {
      mAnimVal = mBaseVal;
    }
#ifdef MOZ_SMIL
    else {
      aSVGElement->AnimationNeedsResample();
    }
#endif
    aSVGElement->DidChangeBoolean(mAttrEnum, PR_TRUE);
  }
}

void
nsSVGBoolean::SetAnimValue(PRBool aValue, nsSVGElement *aSVGElement)
{
  mAnimVal = aValue;
  mIsAnimated = PR_TRUE;
  aSVGElement->DidAnimateBoolean(mAttrEnum);
}

nsresult
nsSVGBoolean::ToDOMAnimatedBoolean(nsIDOMSVGAnimatedBoolean **aResult,
                                   nsSVGElement *aSVGElement)
{
  *aResult = new DOMAnimatedBoolean(this, aSVGElement);
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult);
  return NS_OK;
}

#ifdef MOZ_SMIL
nsISMILAttr*
nsSVGBoolean::ToSMILAttr(nsSVGElement *aSVGElement)
{
  return new SMILBool(this, aSVGElement);
}

nsresult
nsSVGBoolean::SMILBool::ValueFromString(const nsAString& aStr,
                                        const nsISMILAnimationElement* ,
                                        nsSMILValue& aValue) const
{
  nsSMILValue val(&SMILBoolType::sSingleton);

  if (aStr.EqualsLiteral("true"))
    val.mU.mBool = PR_TRUE;
  else if (aStr.EqualsLiteral("false"))
    val.mU.mBool = PR_FALSE;
  else
    return NS_ERROR_FAILURE;

  aValue = val;
  return NS_OK;
}

nsSMILValue
nsSVGBoolean::SMILBool::GetBaseValue() const
{
  nsSMILValue val(&SMILBoolType::sSingleton);
  val.mU.mBool = mVal->mBaseVal;
  return val;
}

void
nsSVGBoolean::SMILBool::ClearAnimValue()
{
  if (mVal->mIsAnimated) {
    mVal->SetAnimValue(mVal->mBaseVal, mSVGElement);
    mVal->mIsAnimated = PR_FALSE;
  }
}

nsresult
nsSVGBoolean::SMILBool::SetAnimValue(const nsSMILValue& aValue)
{
  NS_ASSERTION(aValue.mType == &SMILBoolType::sSingleton,
               "Unexpected type to assign animated value");
  if (aValue.mType == &SMILBoolType::sSingleton) {
    mVal->SetAnimValue(PRUint16(aValue.mU.mBool), mSVGElement);
  }
  return NS_OK;
}
#endif 
