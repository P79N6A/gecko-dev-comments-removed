





































#include "nsSVGViewBox.h"
#include "prdtoa.h"
#include "nsTextFormatter.h"

NS_SVG_VAL_IMPL_CYCLE_COLLECTION(nsSVGViewBox::DOMBaseVal, mSVGElement)
NS_SVG_VAL_IMPL_CYCLE_COLLECTION(nsSVGViewBox::DOMAnimVal, mSVGElement)
NS_SVG_VAL_IMPL_CYCLE_COLLECTION(nsSVGViewBox::DOMAnimatedRect, mSVGElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsSVGViewBox::DOMBaseVal)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsSVGViewBox::DOMBaseVal)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsSVGViewBox::DOMAnimVal)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsSVGViewBox::DOMAnimVal)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsSVGViewBox::DOMAnimatedRect)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsSVGViewBox::DOMAnimatedRect)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsSVGViewBox::DOMBaseVal)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGRect)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGRect)
NS_INTERFACE_MAP_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsSVGViewBox::DOMAnimVal)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGRect)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGRect)
NS_INTERFACE_MAP_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsSVGViewBox::DOMAnimatedRect)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedRect)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGAnimatedRect)
NS_INTERFACE_MAP_END



void
nsSVGViewBox::Init()
{
  mBaseVal = nsSVGViewBoxRect();
  mAnimVal = nsnull;
  mHasBaseVal = PR_FALSE;
}

const nsSVGViewBoxRect&
nsSVGViewBox::GetAnimValue() const
{
  if (mAnimVal)
    return *mAnimVal;

  return mBaseVal;
}

void
nsSVGViewBox::SetBaseValue(float aX, float aY, float aWidth, float aHeight,
                           nsSVGElement *aSVGElement, PRBool aDoSetAttr)
{
  mAnimVal = nsnull;
  mBaseVal = nsSVGViewBoxRect(aX, aY, aWidth, aHeight);
  mHasBaseVal = PR_TRUE;

  aSVGElement->DidChangeViewBox(aDoSetAttr);
}

nsresult
nsSVGViewBox::SetBaseValueString(const nsAString& aValue,
                                 nsSVGElement *aSVGElement,
                                 PRBool aDoSetAttr)
{
  nsresult rv = NS_OK;

  char *str = ToNewUTF8String(aValue);

  char *rest = str;
  char *token;
  const char *delimiters = ",\x20\x9\xD\xA";

  float vals[4];
  PRUint32 i;
  for (i = 0; i < 4; ++i) {
    if (!(token = nsCRT::strtok(rest, delimiters, &rest))) break; 

    char *end;
    vals[i] = float(PR_strtod(token, &end));
    if (*end != '\0' || !NS_FloatIsFinite(vals[i])) break; 
  }
  if (i!=4 || nsCRT::strtok(rest, delimiters, &rest)!=0) {
    
    rv = NS_ERROR_FAILURE;
  } else {
    SetBaseValue(vals[0], vals[1], vals[2], vals[3], aSVGElement, aDoSetAttr);
  }

  nsMemory::Free(str);

  return rv;
}

void
nsSVGViewBox::GetBaseValueString(nsAString& aValue) const
{
  PRUnichar buf[200];
  nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
                            NS_LITERAL_STRING("%g %g %g %g").get(),
                            (double)mBaseVal.x, (double)mBaseVal.y,
                            (double)mBaseVal.width, (double)mBaseVal.height);
  aValue.Assign(buf);
}

nsresult
nsSVGViewBox::ToDOMAnimatedRect(nsIDOMSVGAnimatedRect **aResult,
                                nsSVGElement* aSVGElement)
{
  *aResult = new DOMAnimatedRect(this, aSVGElement);
  NS_ENSURE_TRUE(*aResult, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsSVGViewBox::DOMAnimatedRect::GetBaseVal(nsIDOMSVGRect **aResult)
{
  *aResult = new nsSVGViewBox::DOMBaseVal(mVal, mSVGElement);
  NS_ENSURE_TRUE(*aResult, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsSVGViewBox::DOMAnimatedRect::GetAnimVal(nsIDOMSVGRect **aResult)
{
  *aResult = new nsSVGViewBox::DOMAnimVal(mVal, mSVGElement);
  NS_ENSURE_TRUE(*aResult, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsSVGViewBox::DOMBaseVal::SetX(float aX)
{
  nsSVGViewBoxRect rect = mVal->GetBaseValue();
  rect.x = aX;
  mVal->SetBaseValue(rect.x, rect.y, rect.width, rect.height,
                     mSVGElement, PR_TRUE);
  return NS_OK;
}

NS_IMETHODIMP
nsSVGViewBox::DOMBaseVal::SetY(float aY)
{
  nsSVGViewBoxRect rect = mVal->GetBaseValue();
  rect.y = aY;
  mVal->SetBaseValue(rect.x, rect.y, rect.width, rect.height,
                     mSVGElement, PR_TRUE);
  return NS_OK;
}

NS_IMETHODIMP
nsSVGViewBox::DOMBaseVal::SetWidth(float aWidth)
{
  nsSVGViewBoxRect rect = mVal->GetBaseValue();
  rect.width = aWidth;
  mVal->SetBaseValue(rect.x, rect.y, rect.width, rect.height,
                     mSVGElement, PR_TRUE);
  return NS_OK;
}

NS_IMETHODIMP
nsSVGViewBox::DOMBaseVal::SetHeight(float aHeight)
{
  nsSVGViewBoxRect rect = mVal->GetBaseValue();
  rect.height = aHeight;
  mVal->SetBaseValue(rect.x, rect.y, rect.width, rect.height,
                     mSVGElement, PR_TRUE);
  return NS_OK;
}
