





































#include "SVGAnimatedPreserveAspectRatio.h"
#include "nsWhitespaceTokenizer.h"
#ifdef MOZ_SMIL
#include "nsSMILValue.h"
#include "SMILEnumType.h"
#endif 

using namespace mozilla;




NS_SVG_VAL_IMPL_CYCLE_COLLECTION(
  SVGAnimatedPreserveAspectRatio::DOMBaseVal, mSVGElement)
NS_SVG_VAL_IMPL_CYCLE_COLLECTION(
  SVGAnimatedPreserveAspectRatio::DOMAnimVal, mSVGElement)
NS_SVG_VAL_IMPL_CYCLE_COLLECTION(
  SVGAnimatedPreserveAspectRatio::DOMAnimPAspectRatio, mSVGElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(SVGAnimatedPreserveAspectRatio::DOMBaseVal)
NS_IMPL_CYCLE_COLLECTING_RELEASE(SVGAnimatedPreserveAspectRatio::DOMBaseVal)

NS_IMPL_CYCLE_COLLECTING_ADDREF(SVGAnimatedPreserveAspectRatio::DOMAnimVal)
NS_IMPL_CYCLE_COLLECTING_RELEASE(SVGAnimatedPreserveAspectRatio::DOMAnimVal)

NS_IMPL_CYCLE_COLLECTING_ADDREF(
  SVGAnimatedPreserveAspectRatio::DOMAnimPAspectRatio)
NS_IMPL_CYCLE_COLLECTING_RELEASE(
  SVGAnimatedPreserveAspectRatio::DOMAnimPAspectRatio)

DOMCI_DATA(SVGPreserveAspectRatio, SVGAnimatedPreserveAspectRatio::DOMBaseVal)
DOMCI_DATA(SVGAnimatedPreserveAspectRatio,
           SVGAnimatedPreserveAspectRatio::DOMAnimPAspectRatio)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(
  SVGAnimatedPreserveAspectRatio::DOMBaseVal)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGPreserveAspectRatio)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGPreserveAspectRatio)
NS_INTERFACE_MAP_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(
  SVGAnimatedPreserveAspectRatio::DOMAnimVal)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGPreserveAspectRatio)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGPreserveAspectRatio)
NS_INTERFACE_MAP_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(
  SVGAnimatedPreserveAspectRatio::DOMAnimPAspectRatio)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedPreserveAspectRatio)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGAnimatedPreserveAspectRatio)
NS_INTERFACE_MAP_END



static const char *sAlignStrings[] =
  { "none", "xMinYMin", "xMidYMin", "xMaxYMin", "xMinYMid", "xMidYMid",
    "xMaxYMid", "xMinYMax", "xMidYMax", "xMaxYMax" };

static const char *sMeetOrSliceStrings[] = { "meet", "slice" };

static PRUint16
GetAlignForString(const nsAString &aAlignString)
{
  for (PRUint32 i = 0 ; i < NS_ARRAY_LENGTH(sAlignStrings) ; i++) {
    if (aAlignString.EqualsASCII(sAlignStrings[i])) {
      return (i + nsIDOMSVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_NONE);
    }
  }

  return nsIDOMSVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_UNKNOWN;
}

static void
GetAlignString(nsAString& aAlignString, PRUint16 aAlign)
{
  NS_ASSERTION(
    aAlign >= nsIDOMSVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_NONE &&
    aAlign <= nsIDOMSVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_XMAXYMAX,
    "Unknown align");

  aAlignString.AssignASCII(
    sAlignStrings[aAlign -
                  nsIDOMSVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_NONE]);
}

static PRUint16
GetMeetOrSliceForString(const nsAString &aMeetOrSlice)
{
  for (PRUint32 i = 0 ; i < NS_ARRAY_LENGTH(sMeetOrSliceStrings) ; i++) {
    if (aMeetOrSlice.EqualsASCII(sMeetOrSliceStrings[i])) {
      return (i + nsIDOMSVGPreserveAspectRatio::SVG_MEETORSLICE_MEET);
    }
  }

  return nsIDOMSVGPreserveAspectRatio::SVG_MEETORSLICE_UNKNOWN;
}

static void
GetMeetOrSliceString(nsAString& aMeetOrSliceString, PRUint16 aMeetOrSlice)
{
  NS_ASSERTION(
    aMeetOrSlice >= nsIDOMSVGPreserveAspectRatio::SVG_MEETORSLICE_MEET &&
    aMeetOrSlice <= nsIDOMSVGPreserveAspectRatio::SVG_MEETORSLICE_SLICE,
    "Unknown meetOrSlice");

  aMeetOrSliceString.AssignASCII(
    sMeetOrSliceStrings[aMeetOrSlice -
                        nsIDOMSVGPreserveAspectRatio::SVG_MEETORSLICE_MEET]);
}

nsresult
SVGAnimatedPreserveAspectRatio::ToDOMBaseVal(
  nsIDOMSVGPreserveAspectRatio **aResult,
  nsSVGElement *aSVGElement)
{
  *aResult = new DOMBaseVal(this, aSVGElement);
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult);
  return NS_OK;
}

nsresult
SVGAnimatedPreserveAspectRatio::ToDOMAnimVal(
  nsIDOMSVGPreserveAspectRatio **aResult,
  nsSVGElement *aSVGElement)
{
  *aResult = new DOMAnimVal(this, aSVGElement);
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult);
  return NS_OK;
}

static nsresult
ToPreserveAspectRatio(const nsAString &aString,
                      SVGPreserveAspectRatio *aValue)
{
  if (aString.IsEmpty() || NS_IsAsciiWhitespace(aString[0])) {
    return NS_ERROR_DOM_SYNTAX_ERR;
  }

  nsWhitespaceTokenizer tokenizer(aString);
  if (!tokenizer.hasMoreTokens()) {
    return NS_ERROR_DOM_SYNTAX_ERR;
  }
  const nsAString &token = tokenizer.nextToken();

  nsresult rv;
  SVGPreserveAspectRatio val;

  val.SetDefer(token.EqualsLiteral("defer"));

  if (val.GetDefer()) {
    if (!tokenizer.hasMoreTokens()) {
      return NS_ERROR_DOM_SYNTAX_ERR;
    }
    rv = val.SetAlign(GetAlignForString(tokenizer.nextToken()));
  } else {
    rv = val.SetAlign(GetAlignForString(token));
  }

  if (NS_FAILED(rv)) {
    return NS_ERROR_DOM_SYNTAX_ERR;
  }

  if (tokenizer.hasMoreTokens()) {
    rv = val.SetMeetOrSlice(GetMeetOrSliceForString(tokenizer.nextToken()));
    if (NS_FAILED(rv)) {
      return NS_ERROR_DOM_SYNTAX_ERR;
    }
  } else {
    val.SetMeetOrSlice(nsIDOMSVGPreserveAspectRatio::SVG_MEETORSLICE_MEET);
  }

  if (tokenizer.hasMoreTokens()) {
    return NS_ERROR_DOM_SYNTAX_ERR;
  }

  *aValue = val;
  return NS_OK;
}

nsresult
SVGAnimatedPreserveAspectRatio::SetBaseValueString(
  const nsAString &aValueAsString, nsSVGElement *aSVGElement)
{
  SVGPreserveAspectRatio val;
  nsresult res = ToPreserveAspectRatio(aValueAsString, &val);
  if (NS_FAILED(res)) {
    return res;
  }

  mBaseVal = val;
  mIsBaseSet = PR_TRUE;
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
SVGAnimatedPreserveAspectRatio::GetBaseValueString(nsAString & aValueAsString)
{
  nsAutoString tmpString;

  aValueAsString.Truncate();

  if (mBaseVal.mDefer) {
    aValueAsString.AppendLiteral("defer ");
  }

  GetAlignString(tmpString, mBaseVal.mAlign);
  aValueAsString.Append(tmpString);

  if (mBaseVal.mAlign !=
      nsIDOMSVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_NONE) {

    aValueAsString.AppendLiteral(" ");
    GetMeetOrSliceString(tmpString, mBaseVal.mMeetOrSlice);
    aValueAsString.Append(tmpString);
  }
}

nsresult
SVGAnimatedPreserveAspectRatio::SetBaseAlign(PRUint16 aAlign,
                                             nsSVGElement *aSVGElement)
{
  nsresult rv = mBaseVal.SetAlign(aAlign);
  NS_ENSURE_SUCCESS(rv, rv);
  mIsBaseSet = PR_TRUE;

  mAnimVal.mAlign = mBaseVal.mAlign;
  aSVGElement->DidChangePreserveAspectRatio(PR_TRUE);
#ifdef MOZ_SMIL
  if (mIsAnimated) {
    aSVGElement->AnimationNeedsResample();
  }
#endif
  
  return NS_OK;
}

nsresult
SVGAnimatedPreserveAspectRatio::SetBaseMeetOrSlice(PRUint16 aMeetOrSlice,
                                                   nsSVGElement *aSVGElement)
{
  nsresult rv = mBaseVal.SetMeetOrSlice(aMeetOrSlice);
  NS_ENSURE_SUCCESS(rv, rv);
  mIsBaseSet = PR_TRUE;

  mAnimVal.mMeetOrSlice = mBaseVal.mMeetOrSlice;
  aSVGElement->DidChangePreserveAspectRatio(PR_TRUE);
#ifdef MOZ_SMIL
  if (mIsAnimated) {
    aSVGElement->AnimationNeedsResample();
  }
#endif
  
  return NS_OK;
}

void
SVGAnimatedPreserveAspectRatio::SetAnimValue(PRUint64 aPackedValue,
                                             nsSVGElement *aSVGElement)
{
  mAnimVal.SetDefer(((aPackedValue & 0xff0000) >> 16) ? PR_TRUE : PR_FALSE);
  mAnimVal.SetAlign(PRUint16((aPackedValue & 0xff00) >> 8));
  mAnimVal.SetMeetOrSlice(PRUint16(aPackedValue & 0xff));
  mIsAnimated = PR_TRUE;
  aSVGElement->DidAnimatePreserveAspectRatio();
}

nsresult
SVGAnimatedPreserveAspectRatio::ToDOMAnimatedPreserveAspectRatio(
  nsIDOMSVGAnimatedPreserveAspectRatio **aResult,
  nsSVGElement *aSVGElement)
{
  *aResult = new DOMAnimPAspectRatio(this, aSVGElement);
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult);
  return NS_OK;
}

#ifdef MOZ_SMIL
nsISMILAttr*
SVGAnimatedPreserveAspectRatio::ToSMILAttr(nsSVGElement *aSVGElement)
{
  return new SMILPreserveAspectRatio(this, aSVGElement);
}

static PRUint64
PackPreserveAspectRatio(const SVGPreserveAspectRatio& par)
{
  
  
  PRUint64 packed = 0;
  packed |= PRUint64(par.GetDefer() ? 1 : 0) << 16;
  packed |= PRUint64(par.GetAlign()) << 8;
  packed |= PRUint64(par.GetMeetOrSlice());
  return packed;
}


typedef SVGAnimatedPreserveAspectRatio::SMILPreserveAspectRatio
  SMILPreserveAspectRatio;

nsresult
SMILPreserveAspectRatio::ValueFromString(const nsAString& aStr,
                                         const nsISMILAnimationElement* ,
                                         nsSMILValue& aValue,
                                         bool& aPreventCachingOfSandwich) const
{
  SVGPreserveAspectRatio par;
  nsresult res = ToPreserveAspectRatio(aStr, &par);
  NS_ENSURE_SUCCESS(res, res);

  nsSMILValue val(&SMILEnumType::sSingleton);
  val.mU.mUint = PackPreserveAspectRatio(par);
  aValue = val;
  aPreventCachingOfSandwich = PR_FALSE;
  return NS_OK;
}

nsSMILValue
SMILPreserveAspectRatio::GetBaseValue() const
{
  nsSMILValue val(&SMILEnumType::sSingleton);
  val.mU.mUint = PackPreserveAspectRatio(mVal->GetBaseValue());
  return val;
}

void
SMILPreserveAspectRatio::ClearAnimValue()
{
  if (mVal->mIsAnimated) {
    mVal->SetAnimValue(PackPreserveAspectRatio(mVal->GetBaseValue()),
                       mSVGElement);
    mVal->mIsAnimated = PR_FALSE;
  }
}

nsresult
SMILPreserveAspectRatio::SetAnimValue(const nsSMILValue& aValue)
{
  NS_ASSERTION(aValue.mType == &SMILEnumType::sSingleton,
               "Unexpected type to assign animated value");
  if (aValue.mType == &SMILEnumType::sSingleton) {
    mVal->SetAnimValue(aValue.mU.mUint, mSVGElement);
  }
  return NS_OK;
}
#endif 
