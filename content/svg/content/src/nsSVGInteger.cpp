




#include "nsError.h"
#include "nsSVGAttrTearoffTable.h"
#include "nsSVGInteger.h"
#include "nsSMILValue.h"
#include "SMILIntegerType.h"
#include "SVGContentUtils.h"

using namespace mozilla;
using namespace mozilla::dom;



static nsSVGAttrTearoffTable<nsSVGInteger, nsSVGInteger::DOMAnimatedInteger>
  sSVGAnimatedIntegerTearoffTable;

nsresult
nsSVGInteger::SetBaseValueString(const nsAString &aValueAsString,
                                 nsSVGElement *aSVGElement)
{
  int32_t value;

  if (!SVGContentUtils::ParseInteger(aValueAsString, value)) {
    return NS_ERROR_DOM_SYNTAX_ERR;
  }

  mIsBaseSet = true;
  mBaseVal = value;
  if (!mIsAnimated) {
    mAnimVal = mBaseVal;
  }
  else {
    aSVGElement->AnimationNeedsResample();
  }
  return NS_OK;
}

void
nsSVGInteger::GetBaseValueString(nsAString & aValueAsString)
{
  aValueAsString.Truncate();
  aValueAsString.AppendInt(mBaseVal);
}

void
nsSVGInteger::SetBaseValue(int aValue, nsSVGElement *aSVGElement)
{
  
  
  
  
  if (aValue == mBaseVal && mIsBaseSet) {
    return;
  }

  mBaseVal = aValue;
  mIsBaseSet = true;
  if (!mIsAnimated) {
    mAnimVal = mBaseVal;
  }
  else {
    aSVGElement->AnimationNeedsResample();
  }
  aSVGElement->DidChangeInteger(mAttrEnum);
}

void
nsSVGInteger::SetAnimValue(int aValue, nsSVGElement *aSVGElement)
{
  if (mIsAnimated && aValue == mAnimVal) {
    return;
  }
  mAnimVal = aValue;
  mIsAnimated = true;
  aSVGElement->DidAnimateInteger(mAttrEnum);
}

already_AddRefed<SVGAnimatedInteger>
nsSVGInteger::ToDOMAnimatedInteger(nsSVGElement *aSVGElement)
{
  nsRefPtr<DOMAnimatedInteger> domAnimatedInteger =
    sSVGAnimatedIntegerTearoffTable.GetTearoff(this);
  if (!domAnimatedInteger) {
    domAnimatedInteger = new DOMAnimatedInteger(this, aSVGElement);
    sSVGAnimatedIntegerTearoffTable.AddTearoff(this, domAnimatedInteger);
  }

  return domAnimatedInteger.forget();
}

nsSVGInteger::DOMAnimatedInteger::~DOMAnimatedInteger()
{
  sSVGAnimatedIntegerTearoffTable.RemoveTearoff(mVal);
}

nsISMILAttr*
nsSVGInteger::ToSMILAttr(nsSVGElement *aSVGElement)
{
  return new SMILInteger(this, aSVGElement);
}

nsresult
nsSVGInteger::SMILInteger::ValueFromString(const nsAString& aStr,
                                           const dom::SVGAnimationElement* ,
                                           nsSMILValue& aValue,
                                           bool& aPreventCachingOfSandwich) const
{
  int32_t val;

  if (!SVGContentUtils::ParseInteger(aStr, val)) {
    return NS_ERROR_DOM_SYNTAX_ERR;
  }

  nsSMILValue smilVal(SMILIntegerType::Singleton());
  smilVal.mU.mInt = val;
  aValue = smilVal;
  aPreventCachingOfSandwich = false;
  return NS_OK;
}

nsSMILValue
nsSVGInteger::SMILInteger::GetBaseValue() const
{
  nsSMILValue val(SMILIntegerType::Singleton());
  val.mU.mInt = mVal->mBaseVal;
  return val;
}

void
nsSVGInteger::SMILInteger::ClearAnimValue()
{
  if (mVal->mIsAnimated) {
    mVal->mIsAnimated = false;
    mVal->mAnimVal = mVal->mBaseVal;
    mSVGElement->DidAnimateInteger(mVal->mAttrEnum);
  }
}

nsresult
nsSVGInteger::SMILInteger::SetAnimValue(const nsSMILValue& aValue)
{
  NS_ASSERTION(aValue.mType == SMILIntegerType::Singleton(),
               "Unexpected type to assign animated value");
  if (aValue.mType == SMILIntegerType::Singleton()) {
    mVal->SetAnimValue(int(aValue.mU.mInt), mSVGElement);
  }
  return NS_OK;
}
