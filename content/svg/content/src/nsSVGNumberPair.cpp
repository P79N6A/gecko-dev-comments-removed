



































#include "nsSVGNumberPair.h"
#include "nsSVGUtils.h"
#include "nsCharSeparatedTokenizer.h"
#include "prdtoa.h"
#include "nsDOMError.h"
#include "nsMathUtils.h"
#ifdef MOZ_SMIL
#include "nsSMILValue.h"
#include "SVGNumberPairSMILType.h"
#endif 

using namespace mozilla;

NS_SVG_VAL_IMPL_CYCLE_COLLECTION(nsSVGNumberPair::DOMAnimatedNumber, mSVGElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsSVGNumberPair::DOMAnimatedNumber)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsSVGNumberPair::DOMAnimatedNumber)

DOMCI_DATA(SVGAnimatedNumberPair, nsSVGNumberPair::DOMAnimatedNumber)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsSVGNumberPair::DOMAnimatedNumber)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedNumber)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGAnimatedNumber)
NS_INTERFACE_MAP_END



static nsresult
ParseNumberOptionalNumber(const nsAString& aValue,
                          float aValues[2])
{
  nsCharSeparatedTokenizerTemplate<IsSVGWhitespace>
    tokenizer(aValue, ',',
              nsCharSeparatedTokenizer::SEPARATOR_OPTIONAL);
  if (tokenizer.firstTokenBeganWithWhitespace()) {
    return NS_ERROR_DOM_SYNTAX_ERR;
  }

  PRUint32 i;
  for (i = 0; i < 2 && tokenizer.hasMoreTokens(); ++i) {
    NS_ConvertUTF16toUTF8 utf8Token(tokenizer.nextToken());
    const char *token = utf8Token.get();
    if (*token == '\0') {
      return NS_ERROR_DOM_SYNTAX_ERR; 
    }

    char *end;
    aValues[i] = float(PR_strtod(token, &end));
    if (*end != '\0' || !NS_finite(aValues[i])) {
      return NS_ERROR_DOM_SYNTAX_ERR; 
    }
  }
  if (i == 1) {
    aValues[1] = aValues[0];
  }

  if (i == 0 ||                                   
      tokenizer.hasMoreTokens() ||                
      tokenizer.lastTokenEndedWithWhitespace() || 
      tokenizer.lastTokenEndedWithSeparator()) {  
    return NS_ERROR_DOM_SYNTAX_ERR;
  }

  return NS_OK;
}

nsresult
nsSVGNumberPair::SetBaseValueString(const nsAString &aValueAsString,
                                    nsSVGElement *aSVGElement,
                                    bool aDoSetAttr)
{
  float val[2];

  nsresult rv = ParseNumberOptionalNumber(aValueAsString, val);
  if (NS_FAILED(rv)) {
    return rv;
  }

  mBaseVal[0] = val[0];
  mBaseVal[1] = val[1];
  mIsBaseSet = PR_TRUE;
  if (!mIsAnimated) {
    mAnimVal[0] = mBaseVal[0];
    mAnimVal[1] = mBaseVal[1];
  }
#ifdef MOZ_SMIL
  else {
    aSVGElement->AnimationNeedsResample();
  }
#endif

  
  
  
  return NS_OK;
}

void
nsSVGNumberPair::GetBaseValueString(nsAString &aValueAsString)
{
  aValueAsString.Truncate();
  aValueAsString.AppendFloat(mBaseVal[0]);
  if (mBaseVal[0] != mBaseVal[1]) {
    aValueAsString.AppendLiteral(", ");
    aValueAsString.AppendFloat(mBaseVal[1]);
  }
}

void
nsSVGNumberPair::SetBaseValue(float aValue, PairIndex aPairIndex,
                              nsSVGElement *aSVGElement,
                              bool aDoSetAttr)
{
  PRUint32 index = (aPairIndex == eFirst ? 0 : 1);
  mBaseVal[index] = aValue;
  mIsBaseSet = PR_TRUE;
  if (!mIsAnimated) {
    mAnimVal[index] = aValue;
  }
#ifdef MOZ_SMIL
  else {
    aSVGElement->AnimationNeedsResample();
  }
#endif
  aSVGElement->DidChangeNumberPair(mAttrEnum, aDoSetAttr);
}

void
nsSVGNumberPair::SetBaseValues(float aValue1, float aValue2,
                               nsSVGElement *aSVGElement,
                               bool aDoSetAttr)
{
  mBaseVal[0] = aValue1;
  mBaseVal[1] = aValue2;
  mIsBaseSet = PR_TRUE;
  if (!mIsAnimated) {
    mAnimVal[0] = aValue1;
    mAnimVal[1] = aValue2;
  }
#ifdef MOZ_SMIL
  else {
    aSVGElement->AnimationNeedsResample();
  }
#endif
  aSVGElement->DidChangeNumberPair(mAttrEnum, aDoSetAttr);
}

void
nsSVGNumberPair::SetAnimValue(const float aValue[2], nsSVGElement *aSVGElement)
{
  mAnimVal[0] = aValue[0];
  mAnimVal[1] = aValue[1];
  mIsAnimated = PR_TRUE;
  aSVGElement->DidAnimateNumberPair(mAttrEnum);
}

nsresult
nsSVGNumberPair::ToDOMAnimatedNumber(nsIDOMSVGAnimatedNumber **aResult,
                                     PairIndex aIndex,
                                     nsSVGElement *aSVGElement)
{
  *aResult = new DOMAnimatedNumber(this, aIndex, aSVGElement);
  NS_ADDREF(*aResult);
  return NS_OK;
}

#ifdef MOZ_SMIL
nsISMILAttr*
nsSVGNumberPair::ToSMILAttr(nsSVGElement *aSVGElement)
{
  return new SMILNumberPair(this, aSVGElement);
}

nsresult
nsSVGNumberPair::SMILNumberPair::ValueFromString(const nsAString& aStr,
                                                 const nsISMILAnimationElement* ,
                                                 nsSMILValue& aValue,
                                                 bool& aPreventCachingOfSandwich) const
{
  float values[2];

  nsresult rv = ParseNumberOptionalNumber(aStr, values);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsSMILValue val(&SVGNumberPairSMILType::sSingleton);
  val.mU.mNumberPair[0] = values[0];
  val.mU.mNumberPair[1] = values[1];
  aValue = val;
  aPreventCachingOfSandwich = PR_FALSE;

  return NS_OK;
}

nsSMILValue
nsSVGNumberPair::SMILNumberPair::GetBaseValue() const
{
  nsSMILValue val(&SVGNumberPairSMILType::sSingleton);
  val.mU.mNumberPair[0] = mVal->mBaseVal[0];
  val.mU.mNumberPair[1] = mVal->mBaseVal[1];
  return val;
}

void
nsSVGNumberPair::SMILNumberPair::ClearAnimValue()
{
  if (mVal->mIsAnimated) {
    mVal->SetAnimValue(mVal->mBaseVal, mSVGElement);
    mVal->mIsAnimated = PR_FALSE;
  }
}

nsresult
nsSVGNumberPair::SMILNumberPair::SetAnimValue(const nsSMILValue& aValue)
{
  NS_ASSERTION(aValue.mType == &SVGNumberPairSMILType::sSingleton,
               "Unexpected type to assign animated value");
  if (aValue.mType == &SVGNumberPairSMILType::sSingleton) {
    mVal->SetAnimValue(aValue.mU.mNumberPair, mSVGElement);
  }
  return NS_OK;
}
#endif 
