



































#include "nsSVGBoolean.h"
#include "nsSMILValue.h"
#include "SMILBoolType.h"

using namespace mozilla;

NS_SVG_VAL_IMPL_CYCLE_COLLECTION(nsSVGBoolean::DOMAnimatedBoolean, mSVGElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsSVGBoolean::DOMAnimatedBoolean)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsSVGBoolean::DOMAnimatedBoolean)

DOMCI_DATA(SVGAnimatedBoolean, nsSVGBoolean::DOMAnimatedBoolean)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsSVGBoolean::DOMAnimatedBoolean)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedBoolean)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGAnimatedBoolean)
NS_INTERFACE_MAP_END



static nsresult
GetValueFromString(const nsAString &aValueAsString,
                   bool *aValue)
{
  if (aValueAsString.EqualsLiteral("true")) {
    *aValue = true;
    return NS_OK;
  }
  if (aValueAsString.EqualsLiteral("false")) {
    *aValue = false;
    return NS_OK;
  }
  return NS_ERROR_DOM_SYNTAX_ERR;
}

static nsresult
GetValueFromAtom(const nsIAtom* aValueAsAtom, bool *aValue)
{
  if (aValueAsAtom == nsGkAtoms::_true) {
    *aValue = true;
    return NS_OK;
  }
  if (aValueAsAtom == nsGkAtoms::_false) {
    *aValue = false;
    return NS_OK;
  }
  return NS_ERROR_DOM_SYNTAX_ERR;
}

nsresult
nsSVGBoolean::SetBaseValueAtom(const nsIAtom* aValue, nsSVGElement *aSVGElement)
{
  bool val;

  nsresult rv = GetValueFromAtom(aValue, &val);
  if (NS_FAILED(rv)) {
    return rv;
  }

  mBaseVal = val;
  if (!mIsAnimated) {
    mAnimVal = mBaseVal;
  }
  else {
    aSVGElement->AnimationNeedsResample();
  }

  
  
  
  return NS_OK;
}

nsIAtom*
nsSVGBoolean::GetBaseValueAtom() const
{
  return mBaseVal ? nsGkAtoms::_true : nsGkAtoms::_false;
}

void
nsSVGBoolean::SetBaseValue(bool aValue, nsSVGElement *aSVGElement)
{
  NS_PRECONDITION(aValue == true || aValue == false, "Boolean out of range");

  if (aValue == mBaseVal) {
    return;
  }

  mBaseVal = aValue;
  if (!mIsAnimated) {
    mAnimVal = mBaseVal;
  } else {
    aSVGElement->AnimationNeedsResample();
  }
  aSVGElement->DidChangeBoolean(mAttrEnum);
}

void
nsSVGBoolean::SetAnimValue(bool aValue, nsSVGElement *aSVGElement)
{
  mAnimVal = aValue;
  mIsAnimated = true;
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

nsISMILAttr*
nsSVGBoolean::ToSMILAttr(nsSVGElement *aSVGElement)
{
  return new SMILBool(this, aSVGElement);
}

nsresult
nsSVGBoolean::SMILBool::ValueFromString(const nsAString& aStr,
                                        const nsISMILAnimationElement* ,
                                        nsSMILValue& aValue,
                                        bool& aPreventCachingOfSandwich) const
{
  bool value;
  nsresult rv = GetValueFromString(aStr, &value);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsSMILValue val(&SMILBoolType::sSingleton);
  val.mU.mBool = value;
  aValue = val;
  aPreventCachingOfSandwich = false;

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
    mVal->mIsAnimated = false;
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
