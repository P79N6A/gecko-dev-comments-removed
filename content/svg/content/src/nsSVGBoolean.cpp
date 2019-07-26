




#include "nsError.h"
#include "nsSVGAttrTearoffTable.h"
#include "nsSVGBoolean.h"
#include "nsSMILValue.h"
#include "SMILBoolType.h"
#include "SVGAnimatedBoolean.h"

using namespace mozilla;
using namespace mozilla::dom;



static nsSVGAttrTearoffTable<nsSVGBoolean, SVGAnimatedBoolean>
  sSVGAnimatedBooleanTearoffTable;

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
  if (mIsAnimated && mAnimVal == aValue) {
    return;
  }
  mAnimVal = aValue;
  mIsAnimated = true;
  aSVGElement->DidAnimateBoolean(mAttrEnum);
}

nsresult
nsSVGBoolean::ToDOMAnimatedBoolean(nsISupports **aResult,
                                   nsSVGElement *aSVGElement)
{
  nsRefPtr<SVGAnimatedBoolean> domAnimatedBoolean =
    sSVGAnimatedBooleanTearoffTable.GetTearoff(this);
  if (!domAnimatedBoolean) {
    domAnimatedBoolean = new SVGAnimatedBoolean(this, aSVGElement);
    sSVGAnimatedBooleanTearoffTable.AddTearoff(this, domAnimatedBoolean);
  }

  domAnimatedBoolean.forget(aResult);
  return NS_OK;
}

SVGAnimatedBoolean::~SVGAnimatedBoolean()
{
  sSVGAnimatedBooleanTearoffTable.RemoveTearoff(mVal);
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
    mVal->mIsAnimated = false;
    mVal->mAnimVal = mVal->mBaseVal;
    mSVGElement->DidAnimateBoolean(mVal->mAttrEnum);
  }
}

nsresult
nsSVGBoolean::SMILBool::SetAnimValue(const nsSMILValue& aValue)
{
  NS_ASSERTION(aValue.mType == &SMILBoolType::sSingleton,
               "Unexpected type to assign animated value");
  if (aValue.mType == &SMILBoolType::sSingleton) {
    mVal->SetAnimValue(uint16_t(aValue.mU.mBool), mSVGElement);
  }
  return NS_OK;
}
