




#include "nsSVGIntegerPair.h"
#include "nsSVGAttrTearoffTable.h"
#include "nsCharSeparatedTokenizer.h"
#include "nsError.h"
#include "nsMathUtils.h"
#include "nsSMILValue.h"
#include "SVGContentUtils.h"
#include "SVGIntegerPairSMILType.h"

using namespace mozilla;

NS_SVG_VAL_IMPL_CYCLE_COLLECTION(nsSVGIntegerPair::DOMAnimatedInteger, mSVGElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsSVGIntegerPair::DOMAnimatedInteger)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsSVGIntegerPair::DOMAnimatedInteger)

DOMCI_DATA(SVGAnimatedIntegerPair, nsSVGIntegerPair::DOMAnimatedInteger)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsSVGIntegerPair::DOMAnimatedInteger)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedInteger)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGAnimatedInteger)
NS_INTERFACE_MAP_END

static nsSVGAttrTearoffTable<nsSVGIntegerPair, nsSVGIntegerPair::DOMAnimatedInteger>
  sSVGFirstAnimatedIntegerTearoffTable;
static nsSVGAttrTearoffTable<nsSVGIntegerPair, nsSVGIntegerPair::DOMAnimatedInteger>
  sSVGSecondAnimatedIntegerTearoffTable;



static nsresult
ParseIntegerOptionalInteger(const nsAString& aValue,
                            int32_t aValues[2])
{
  nsCharSeparatedTokenizerTemplate<IsSVGWhitespace>
    tokenizer(aValue, ',',
              nsCharSeparatedTokenizer::SEPARATOR_OPTIONAL);
  if (tokenizer.firstTokenBeganWithWhitespace()) {
    return NS_ERROR_DOM_SYNTAX_ERR;
  }

  uint32_t i;
  for (i = 0; i < 2 && tokenizer.hasMoreTokens(); ++i) {
    NS_ConvertUTF16toUTF8 utf8Token(tokenizer.nextToken());
    const char *token = utf8Token.get();
    if (*token == '\0') {
      return NS_ERROR_DOM_SYNTAX_ERR; 
    }

    char *end;
    aValues[i] = strtol(token, &end, 10);
    if (*end != '\0' || !NS_finite(aValues[i])) {
      return NS_ERROR_DOM_SYNTAX_ERR; 
    }
  }
  if (i == 1) {
    aValues[1] = aValues[0];
  }

  if (i == 0                    ||                
      tokenizer.hasMoreTokens() ||                
      tokenizer.lastTokenEndedWithWhitespace() || 
      tokenizer.lastTokenEndedWithSeparator()) {  
    return NS_ERROR_DOM_SYNTAX_ERR;
  }

  return NS_OK;
}

nsresult
nsSVGIntegerPair::SetBaseValueString(const nsAString &aValueAsString,
                                     nsSVGElement *aSVGElement)
{
  int32_t val[2];

  nsresult rv = ParseIntegerOptionalInteger(aValueAsString, val);

  if (NS_FAILED(rv)) {
    return rv;
  }

  mBaseVal[0] = val[0];
  mBaseVal[1] = val[1];
  mIsBaseSet = true;
  if (!mIsAnimated) {
    mAnimVal[0] = mBaseVal[0];
    mAnimVal[1] = mBaseVal[1];
  }
  else {
    aSVGElement->AnimationNeedsResample();
  }

  
  
  
  return NS_OK;
}

void
nsSVGIntegerPair::GetBaseValueString(nsAString &aValueAsString) const
{
  aValueAsString.Truncate();
  aValueAsString.AppendInt(mBaseVal[0]);
  if (mBaseVal[0] != mBaseVal[1]) {
    aValueAsString.AppendLiteral(", ");
    aValueAsString.AppendInt(mBaseVal[1]);
  }
}

void
nsSVGIntegerPair::SetBaseValue(int32_t aValue, PairIndex aPairIndex,
                               nsSVGElement *aSVGElement)
{
  uint32_t index = (aPairIndex == eFirst ? 0 : 1);
  if (mIsBaseSet && mBaseVal[index] == aValue) {
    return;
  }

  nsAttrValue emptyOrOldValue = aSVGElement->WillChangeIntegerPair(mAttrEnum);
  mBaseVal[index] = aValue;
  mIsBaseSet = true;
  if (!mIsAnimated) {
    mAnimVal[index] = aValue;
  }
  else {
    aSVGElement->AnimationNeedsResample();
  }
  aSVGElement->DidChangeIntegerPair(mAttrEnum, emptyOrOldValue);
}

void
nsSVGIntegerPair::SetBaseValues(int32_t aValue1, int32_t aValue2,
                                nsSVGElement *aSVGElement)
{
  if (mIsBaseSet && mBaseVal[0] == aValue1 && mBaseVal[1] == aValue2) {
    return;
  }

  nsAttrValue emptyOrOldValue = aSVGElement->WillChangeIntegerPair(mAttrEnum);
  mBaseVal[0] = aValue1;
  mBaseVal[1] = aValue2;
  mIsBaseSet = true;
  if (!mIsAnimated) {
    mAnimVal[0] = aValue1;
    mAnimVal[1] = aValue2;
  }
  else {
    aSVGElement->AnimationNeedsResample();
  }
  aSVGElement->DidChangeIntegerPair(mAttrEnum, emptyOrOldValue);
}

void
nsSVGIntegerPair::SetAnimValue(const int32_t aValue[2], nsSVGElement *aSVGElement)
{
  if (mIsAnimated && mAnimVal[0] == aValue[0] && mAnimVal[1] == aValue[1]) {
    return;
  }
  mAnimVal[0] = aValue[0];
  mAnimVal[1] = aValue[1];
  mIsAnimated = true;
  aSVGElement->DidAnimateIntegerPair(mAttrEnum);
}

nsresult
nsSVGIntegerPair::ToDOMAnimatedInteger(nsIDOMSVGAnimatedInteger **aResult,
                                       PairIndex aIndex,
                                       nsSVGElement *aSVGElement)
{
  *aResult = ToDOMAnimatedInteger(aIndex, aSVGElement).get();
  return NS_OK;
}

already_AddRefed<nsIDOMSVGAnimatedInteger>
nsSVGIntegerPair::ToDOMAnimatedInteger(PairIndex aIndex,
                                       nsSVGElement* aSVGElement)
{
  nsRefPtr<DOMAnimatedInteger> domAnimatedInteger =
    aIndex == eFirst ? sSVGFirstAnimatedIntegerTearoffTable.GetTearoff(this) :
                       sSVGSecondAnimatedIntegerTearoffTable.GetTearoff(this);
  if (!domAnimatedInteger) {
    domAnimatedInteger = new DOMAnimatedInteger(this, aIndex, aSVGElement);
    if (aIndex == eFirst) {
      sSVGFirstAnimatedIntegerTearoffTable.AddTearoff(this, domAnimatedInteger);
    } else {
      sSVGSecondAnimatedIntegerTearoffTable.AddTearoff(this, domAnimatedInteger);
    }
  }

  return domAnimatedInteger.forget();
}

nsSVGIntegerPair::DOMAnimatedInteger::~DOMAnimatedInteger()
{
  if (mIndex == eFirst) {
    sSVGFirstAnimatedIntegerTearoffTable.RemoveTearoff(mVal);
  } else {
    sSVGSecondAnimatedIntegerTearoffTable.RemoveTearoff(mVal);
  }
}

nsISMILAttr*
nsSVGIntegerPair::ToSMILAttr(nsSVGElement *aSVGElement)
{
  return new SMILIntegerPair(this, aSVGElement);
}

nsresult
nsSVGIntegerPair::SMILIntegerPair::ValueFromString(const nsAString& aStr,
                                                   const dom::SVGAnimationElement* ,
                                                   nsSMILValue& aValue,
                                                   bool& aPreventCachingOfSandwich) const
{
  int32_t values[2];

  nsresult rv = ParseIntegerOptionalInteger(aStr, values);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsSMILValue val(&SVGIntegerPairSMILType::sSingleton);
  val.mU.mIntPair[0] = values[0];
  val.mU.mIntPair[1] = values[1];
  aValue = val;
  aPreventCachingOfSandwich = false;

  return NS_OK;
}

nsSMILValue
nsSVGIntegerPair::SMILIntegerPair::GetBaseValue() const
{
  nsSMILValue val(&SVGIntegerPairSMILType::sSingleton);
  val.mU.mIntPair[0] = mVal->mBaseVal[0];
  val.mU.mIntPair[1] = mVal->mBaseVal[1];
  return val;
}

void
nsSVGIntegerPair::SMILIntegerPair::ClearAnimValue()
{
  if (mVal->mIsAnimated) {
    mVal->mIsAnimated = false;
    mVal->mAnimVal[0] = mVal->mBaseVal[0];
    mVal->mAnimVal[1] = mVal->mBaseVal[1];
    mSVGElement->DidAnimateIntegerPair(mVal->mAttrEnum);
  }
}

nsresult
nsSVGIntegerPair::SMILIntegerPair::SetAnimValue(const nsSMILValue& aValue)
{
  NS_ASSERTION(aValue.mType == &SVGIntegerPairSMILType::sSingleton,
               "Unexpected type to assign animated value");
  if (aValue.mType == &SVGIntegerPairSMILType::sSingleton) {
    mVal->SetAnimValue(aValue.mU.mIntPair, mSVGElement);
  }
  return NS_OK;
}
