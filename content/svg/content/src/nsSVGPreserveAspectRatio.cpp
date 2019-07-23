





































#include "nsSVGPreserveAspectRatio.h"
#include "nsWhitespaceTokenizer.h"




NS_SVG_VAL_IMPL_CYCLE_COLLECTION(
  nsSVGPreserveAspectRatio::DOMBaseVal, mSVGElement)
NS_SVG_VAL_IMPL_CYCLE_COLLECTION(
  nsSVGPreserveAspectRatio::DOMAnimVal, mSVGElement)
NS_SVG_VAL_IMPL_CYCLE_COLLECTION(
  nsSVGPreserveAspectRatio::DOMAnimPAspectRatio, mSVGElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsSVGPreserveAspectRatio::DOMBaseVal)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsSVGPreserveAspectRatio::DOMBaseVal)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsSVGPreserveAspectRatio::DOMAnimVal)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsSVGPreserveAspectRatio::DOMAnimVal)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsSVGPreserveAspectRatio::DOMAnimPAspectRatio)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsSVGPreserveAspectRatio::DOMAnimPAspectRatio)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsSVGPreserveAspectRatio::DOMBaseVal)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGPreserveAspectRatio)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGPreserveAspectRatio)
NS_INTERFACE_MAP_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsSVGPreserveAspectRatio::DOMAnimVal)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGPreserveAspectRatio)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGPreserveAspectRatio)
NS_INTERFACE_MAP_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsSVGPreserveAspectRatio::DOMAnimPAspectRatio)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedPreserveAspectRatio)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGAnimatedPreserveAspectRatio)
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
nsSVGPreserveAspectRatio::ToDOMBaseVal(nsIDOMSVGPreserveAspectRatio **aResult,
                                       nsSVGElement *aSVGElement)
{
  *aResult = new DOMBaseVal(this, aSVGElement);
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult);
  return NS_OK;
}

nsresult
nsSVGPreserveAspectRatio::ToDOMAnimVal(nsIDOMSVGPreserveAspectRatio **aResult,
                                       nsSVGElement *aSVGElement)
{
  *aResult = new DOMAnimVal(this, aSVGElement);
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult);
  return NS_OK;
}

nsresult
nsSVGPreserveAspectRatio::SetBaseValueString(const nsAString &aValueAsString,
                                             nsSVGElement *aSVGElement,
                                             PRBool aDoSetAttr)
{
  if (aValueAsString.IsEmpty() ||
      NS_IsAsciiWhitespace(aValueAsString[0])) {
    return NS_ERROR_FAILURE;
  }

  nsWhitespaceTokenizer tokenizer(aValueAsString);
  if (!tokenizer.hasMoreTokens()) {
    return NS_ERROR_FAILURE;
  }
  const nsAString &token = tokenizer.nextToken();

  nsresult rv;
  PreserveAspectRatio val;

  val.mDefer = token.EqualsLiteral("defer");

  if (val.mDefer) {
    if (!tokenizer.hasMoreTokens()) {
      return NS_ERROR_FAILURE;
    }
    rv = val.SetAlign(GetAlignForString(tokenizer.nextToken()));
  } else {
    rv = val.SetAlign(GetAlignForString(token));
  }

  NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

  if (tokenizer.hasMoreTokens()) {
    rv = val.SetMeetOrSlice(GetMeetOrSliceForString(tokenizer.nextToken()));
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
  } else {
    val.mMeetOrSlice = nsIDOMSVGPreserveAspectRatio::SVG_MEETORSLICE_MEET;
  }

  if (tokenizer.hasMoreTokens()) {
    return NS_ERROR_FAILURE;
  }

  mAnimVal = mBaseVal = val;
  return NS_OK;
}

void
nsSVGPreserveAspectRatio::GetBaseValueString(nsAString & aValueAsString)
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
nsSVGPreserveAspectRatio::SetBaseAlign(PRUint16 aAlign,
                                       nsSVGElement *aSVGElement)
{
  nsresult rv = mBaseVal.SetAlign(aAlign);
  NS_ENSURE_SUCCESS(rv, rv);

  mAnimVal.mAlign = mBaseVal.mAlign;
  aSVGElement->DidChangePreserveAspectRatio(PR_TRUE);

  return NS_OK;
}

nsresult
nsSVGPreserveAspectRatio::SetBaseMeetOrSlice(PRUint16 aMeetOrSlice,
                                             nsSVGElement *aSVGElement)
{
  nsresult rv = mBaseVal.SetMeetOrSlice(aMeetOrSlice);
  NS_ENSURE_SUCCESS(rv, rv);

  mAnimVal.mMeetOrSlice = mBaseVal.mMeetOrSlice;
  aSVGElement->DidChangePreserveAspectRatio(PR_TRUE);

  return NS_OK;
}

nsresult
nsSVGPreserveAspectRatio::ToDOMAnimatedPreserveAspectRatio(
  nsIDOMSVGAnimatedPreserveAspectRatio **aResult,
  nsSVGElement *aSVGElement)
{
  *aResult = new DOMAnimPAspectRatio(this, aSVGElement);
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult);
  return NS_OK;
}
