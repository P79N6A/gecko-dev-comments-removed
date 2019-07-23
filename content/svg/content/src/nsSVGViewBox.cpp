





































#include "nsSVGViewBox.h"
#include "prdtoa.h"
#include "nsTextFormatter.h"
#ifdef MOZ_SMIL
#include "nsSMILValue.h"
#include "SVGViewBoxSMILType.h"
#endif 

using namespace mozilla;

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
nsSVGViewBox::GetAnimValue(nsSVGElement *aSVGElement) const
{
#ifdef MOZ_SMIL
  aSVGElement->FlushAnimations();
#endif
  return mAnimVal ? *mAnimVal : mBaseVal;
}

void
nsSVGViewBox::SetAnimValue(float aX, float aY, float aWidth, float aHeight,
                           nsSVGElement *aSVGElement)
{
  if (!mAnimVal) {
    
    mAnimVal = new nsSVGViewBoxRect(aX, aY, aWidth, aHeight);
  } else {
    mAnimVal->x = aX;
    mAnimVal->y = aY;
    mAnimVal->width = aWidth;
    mAnimVal->height = aHeight;
  }
  aSVGElement->DidAnimateViewBox();
}

void
nsSVGViewBox::SetBaseValue(float aX, float aY, float aWidth, float aHeight,
                           nsSVGElement *aSVGElement, PRBool aDoSetAttr)
{
  mBaseVal = nsSVGViewBoxRect(aX, aY, aWidth, aHeight);
  mHasBaseVal = PR_TRUE;

  aSVGElement->DidChangeViewBox(aDoSetAttr);
#ifdef MOZ_SMIL
  if (mAnimVal) {
    aSVGElement->AnimationNeedsResample();
  }
#endif
}

static nsresult
ToSVGViewBoxRect(const nsAString& aStr, nsSVGViewBoxRect *aViewBox)
{
  nsresult rv = NS_OK;

  char *str = ToNewUTF8String(aStr);

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
    
    rv = NS_ERROR_DOM_SYNTAX_ERR;
  } else {
    aViewBox->x = vals[0];
    aViewBox->y = vals[1];
    aViewBox->width = vals[2];
    aViewBox->height = vals[3];
  }

  nsMemory::Free(str);

  return rv;
}

nsresult
nsSVGViewBox::SetBaseValueString(const nsAString& aValue,
                                 nsSVGElement *aSVGElement,
                                 PRBool aDoSetAttr)
{
  nsSVGViewBoxRect viewBox;
  nsresult res = ToSVGViewBoxRect(aValue, &viewBox);
  if (NS_SUCCEEDED(res)) {
    SetBaseValue(viewBox.x, viewBox.y, viewBox.width, viewBox.height, aSVGElement, aDoSetAttr);
  }
  return res;
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

#ifdef MOZ_SMIL
nsISMILAttr*
nsSVGViewBox::ToSMILAttr(nsSVGElement *aSVGElement)
{
  return new SMILViewBox(this, aSVGElement);
}

nsresult
nsSVGViewBox::SMILViewBox
            ::ValueFromString(const nsAString& aStr,
                              const nsISMILAnimationElement* ,
                              nsSMILValue& aValue) const
{
  nsSVGViewBoxRect viewBox;
  nsresult res = ToSVGViewBoxRect(aStr, &viewBox);
  if (NS_FAILED(res)) {
    return res;
  }
  nsSMILValue val(&SVGViewBoxSMILType::sSingleton);
  
  if (val.IsNull()) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  *static_cast<nsSVGViewBoxRect*>(val.mU.mPtr) = viewBox;
  aValue = val;
  
  return NS_OK;
}

nsSMILValue
nsSVGViewBox::SMILViewBox::GetBaseValue() const
{
  nsSMILValue val(&SVGViewBoxSMILType::sSingleton);
  
  if (!val.IsNull()) {
    *static_cast<nsSVGViewBoxRect*>(val.mU.mPtr) = mVal->mBaseVal;
  }
  return val;
}

void
nsSVGViewBox::SMILViewBox::ClearAnimValue()
{
  if (mVal->mAnimVal) {
    mVal->mAnimVal = nsnull;
    mSVGElement->DidAnimateViewBox();
  }
}

nsresult
nsSVGViewBox::SMILViewBox::SetAnimValue(const nsSMILValue& aValue)
{
  NS_ASSERTION(aValue.mType == &SVGViewBoxSMILType::sSingleton,
               "Unexpected type to assign animated value");
  if (aValue.mType == &SVGViewBoxSMILType::sSingleton) {
    nsSVGViewBoxRect &vb = *static_cast<nsSVGViewBoxRect*>(aValue.mU.mPtr);
    mVal->SetAnimValue(vb.x, vb.y, vb.width, vb.height, mSVGElement);
  }
  return NS_OK;
}
#endif 
