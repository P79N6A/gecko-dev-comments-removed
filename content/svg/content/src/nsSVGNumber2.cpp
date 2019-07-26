




#include "nsError.h"
#include "nsSVGAttrTearoffTable.h"
#include "nsSVGNumber2.h"
#include "prdtoa.h"
#include "nsMathUtils.h"
#include "nsContentUtils.h" 
#include "nsSMILValue.h"
#include "nsSMILFloatType.h"
#include "nsIDOMSVGNumber.h"
#include "mozilla/Attributes.h"

class DOMSVGNumber MOZ_FINAL : public nsIDOMSVGNumber
{
public:
  NS_DECL_ISUPPORTS

  DOMSVGNumber() 
    : mVal(0) {}
    
  NS_IMETHOD GetValue(float* aResult)
    { *aResult = mVal; return NS_OK; }
  NS_IMETHOD SetValue(float aValue)
    { NS_ENSURE_FINITE(aValue, NS_ERROR_ILLEGAL_VALUE);
      mVal = aValue;
      return NS_OK; }

private:
  float mVal;
};

NS_SVG_VAL_IMPL_CYCLE_COLLECTION(nsSVGNumber2::DOMAnimatedNumber, mSVGElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsSVGNumber2::DOMAnimatedNumber)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsSVGNumber2::DOMAnimatedNumber)

NS_IMPL_ADDREF(DOMSVGNumber)
NS_IMPL_RELEASE(DOMSVGNumber)

DOMCI_DATA(SVGAnimatedNumber, nsSVGNumber2::DOMAnimatedNumber)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsSVGNumber2::DOMAnimatedNumber)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedNumber)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGAnimatedNumber)
NS_INTERFACE_MAP_END

NS_INTERFACE_MAP_BEGIN(DOMSVGNumber)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGNumber)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGNumber)
NS_INTERFACE_MAP_END



static nsSVGAttrTearoffTable<nsSVGNumber2, nsSVGNumber2::DOMAnimatedNumber>
  sSVGAnimatedNumberTearoffTable;

static nsresult
GetValueFromString(const nsAString &aValueAsString,
                   bool aPercentagesAllowed,
                   float *aValue)
{
  NS_ConvertUTF16toUTF8 value(aValueAsString);
  const char *str = value.get();

  if (NS_IsAsciiWhitespace(*str))
    return NS_ERROR_DOM_SYNTAX_ERR;
  
  char *rest;
  *aValue = float(PR_strtod(str, &rest));
  if (rest == str || !NS_finite(*aValue)) {
    return NS_ERROR_DOM_SYNTAX_ERR;
  }
  if (*rest == '%' && aPercentagesAllowed) {
    *aValue /= 100;
    ++rest;
  }
  if (*rest == '\0') {
    return NS_OK;
  }
  return NS_ERROR_DOM_SYNTAX_ERR;
}

nsresult
nsSVGNumber2::SetBaseValueString(const nsAString &aValueAsString,
                                 nsSVGElement *aSVGElement)
{
  float val;

  nsresult rv = GetValueFromString(
    aValueAsString, aSVGElement->NumberAttrAllowsPercentage(mAttrEnum), &val);

  if (NS_FAILED(rv)) {
    return rv;
  }

  mBaseVal = val;
  mIsBaseSet = true;
  if (!mIsAnimated) {
    mAnimVal = mBaseVal;
  }
  else {
    aSVGElement->AnimationNeedsResample();
  }

  
  
  
  return NS_OK;
}

void
nsSVGNumber2::GetBaseValueString(nsAString & aValueAsString)
{
  aValueAsString.Truncate();
  aValueAsString.AppendFloat(mBaseVal);
}

void
nsSVGNumber2::SetBaseValue(float aValue, nsSVGElement *aSVGElement)
{
  if (mIsBaseSet && aValue == mBaseVal) {
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
  aSVGElement->DidChangeNumber(mAttrEnum);
}

void
nsSVGNumber2::SetAnimValue(float aValue, nsSVGElement *aSVGElement)
{
  if (mIsAnimated && aValue == mAnimVal) {
    return;
  }
  mAnimVal = aValue;
  mIsAnimated = true;
  aSVGElement->DidAnimateNumber(mAttrEnum);
}

already_AddRefed<nsIDOMSVGAnimatedNumber>
nsSVGNumber2::ToDOMAnimatedNumber(nsSVGElement* aSVGElement)
{
  nsRefPtr<DOMAnimatedNumber> domAnimatedNumber =
    sSVGAnimatedNumberTearoffTable.GetTearoff(this);
  if (!domAnimatedNumber) {
    domAnimatedNumber = new DOMAnimatedNumber(this, aSVGElement);
    sSVGAnimatedNumberTearoffTable.AddTearoff(this, domAnimatedNumber);
  }

  return domAnimatedNumber.forget();
}

nsresult
nsSVGNumber2::ToDOMAnimatedNumber(nsIDOMSVGAnimatedNumber **aResult,
                                  nsSVGElement *aSVGElement)
{
  *aResult = ToDOMAnimatedNumber(aSVGElement).get();
  return NS_OK;
}

nsSVGNumber2::DOMAnimatedNumber::~DOMAnimatedNumber()
{
  sSVGAnimatedNumberTearoffTable.RemoveTearoff(mVal);
}

nsISMILAttr*
nsSVGNumber2::ToSMILAttr(nsSVGElement *aSVGElement)
{
  return new SMILNumber(this, aSVGElement);
}

nsresult
nsSVGNumber2::SMILNumber::ValueFromString(const nsAString& aStr,
                                          const nsISMILAnimationElement* ,
                                          nsSMILValue& aValue,
                                          bool& aPreventCachingOfSandwich) const
{
  float value;

  nsresult rv = GetValueFromString(
    aStr, mSVGElement->NumberAttrAllowsPercentage(mVal->mAttrEnum), &value);

  if (NS_FAILED(rv)) {
    return rv;
  }

  nsSMILValue val(&nsSMILFloatType::sSingleton);
  val.mU.mDouble = value;
  aValue = val;
  aPreventCachingOfSandwich = false;

  return NS_OK;
}

nsSMILValue
nsSVGNumber2::SMILNumber::GetBaseValue() const
{
  nsSMILValue val(&nsSMILFloatType::sSingleton);
  val.mU.mDouble = mVal->mBaseVal;
  return val;
}

void
nsSVGNumber2::SMILNumber::ClearAnimValue()
{
  if (mVal->mIsAnimated) {
    mVal->mIsAnimated = false;
    mVal->mAnimVal = mVal->mBaseVal;
    mSVGElement->DidAnimateNumber(mVal->mAttrEnum);
  }
}

nsresult
nsSVGNumber2::SMILNumber::SetAnimValue(const nsSMILValue& aValue)
{
  NS_ASSERTION(aValue.mType == &nsSMILFloatType::sSingleton,
               "Unexpected type to assign animated value");
  if (aValue.mType == &nsSMILFloatType::sSingleton) {
    mVal->SetAnimValue(float(aValue.mU.mDouble), mSVGElement);
  }
  return NS_OK;
}
