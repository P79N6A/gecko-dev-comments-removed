



































#include "nsSVGNumber2.h"
#include "nsSVGUtils.h"
#include "nsTextFormatter.h"
#include "prdtoa.h"
#ifdef MOZ_SMIL
#include "nsSMILValue.h"
#include "nsSMILFloatType.h"
#endif 

class DOMSVGNumber : public nsIDOMSVGNumber
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

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsSVGNumber2::DOMAnimatedNumber)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedNumber)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGAnimatedNumber)
NS_INTERFACE_MAP_END

NS_INTERFACE_MAP_BEGIN(DOMSVGNumber)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGNumber)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGNumber)
NS_INTERFACE_MAP_END



nsresult
nsSVGNumber2::SetBaseValueString(const nsAString &aValueAsString,
                                 nsSVGElement *aSVGElement,
                                 PRBool aDoSetAttr)
{
  NS_ConvertUTF16toUTF8 value(aValueAsString);
  const char *str = value.get();

  if (NS_IsAsciiWhitespace(*str))
    return NS_ERROR_DOM_SYNTAX_ERR;
  
  char *rest;
  float val = float(PR_strtod(str, &rest));
  if (rest == str || *rest != '\0' || !NS_FloatIsFinite(val)) {
    return NS_ERROR_DOM_SYNTAX_ERR;
  }

  mBaseVal = mAnimVal = val;

  

#ifdef MOZ_SMIL
  if (mIsAnimated) {
    aSVGElement->AnimationNeedsResample();
  }
#endif

  return NS_OK;
}

void
nsSVGNumber2::GetBaseValueString(nsAString & aValueAsString)
{
  nsAutoString s;
  s.AppendFloat(mBaseVal);
  aValueAsString.Assign(s);
}

void
nsSVGNumber2::SetBaseValue(float aValue,
                           nsSVGElement *aSVGElement,
                           PRBool aDoSetAttr)
{
  mAnimVal = mBaseVal = aValue;
  aSVGElement->DidChangeNumber(mAttrEnum, aDoSetAttr);
#ifdef MOZ_SMIL
  if (mIsAnimated) {
    aSVGElement->AnimationNeedsResample();
  }
#endif
}

void
nsSVGNumber2::SetAnimValue(float aValue, nsSVGElement *aSVGElement)
{
  mAnimVal = aValue;
  mIsAnimated = PR_TRUE;
  aSVGElement->DidAnimateNumber(mAttrEnum);
}

nsresult
nsSVGNumber2::ToDOMAnimatedNumber(nsIDOMSVGAnimatedNumber **aResult,
                                  nsSVGElement *aSVGElement)
{
  *aResult = new DOMAnimatedNumber(this, aSVGElement);
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult);
  return NS_OK;
}

#ifdef MOZ_SMIL
nsISMILAttr*
nsSVGNumber2::ToSMILAttr(nsSVGElement *aSVGElement)
{
  return new SMILNumber(this, aSVGElement);
}

nsresult
nsSVGNumber2::SMILNumber::ValueFromString(const nsAString& aStr,
                                          const nsISMILAnimationElement* ,
                                          nsSMILValue& aValue) const
{
  float value;

  PRBool ok = nsSVGUtils::NumberFromString(aStr, &value);
  if (!ok) {
    return NS_ERROR_FAILURE;
  }

  nsSMILValue val(&nsSMILFloatType::sSingleton);
  val.mU.mDouble = value;
  aValue = val;

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
    mVal->SetAnimValue(mVal->mBaseVal, mSVGElement);
    mVal->mIsAnimated = PR_FALSE;
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
#endif 
