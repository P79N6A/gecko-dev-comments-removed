





































#include "nsSVGViewBox.h"
#include "nsSVGUtils.h"
#include "prdtoa.h"
#include "nsTextFormatter.h"
#include "nsCharSeparatedTokenizer.h"
#include "nsMathUtils.h"
#include "nsSMILValue.h"
#include "SVGViewBoxSMILType.h"

#define NUM_VIEWBOX_COMPONENTS 4
using namespace mozilla;



bool
nsSVGViewBoxRect::operator==(const nsSVGViewBoxRect& aOther) const
{
  if (&aOther == this)
    return true;

  return x == aOther.x &&
    y == aOther.y &&
    width == aOther.width &&
    height == aOther.height;
}



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
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGRect)
NS_INTERFACE_MAP_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsSVGViewBox::DOMAnimVal)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGRect)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGRect)
NS_INTERFACE_MAP_END

DOMCI_DATA(SVGAnimatedRect, nsSVGViewBox::DOMAnimatedRect)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsSVGViewBox::DOMAnimatedRect)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedRect)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGAnimatedRect)
NS_INTERFACE_MAP_END



void
nsSVGViewBox::Init()
{
  mBaseVal = nsSVGViewBoxRect();
  mAnimVal = nsnull;
  mHasBaseVal = false;
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
                           nsSVGElement *aSVGElement)
{
  mBaseVal = nsSVGViewBoxRect(aX, aY, aWidth, aHeight);
  mHasBaseVal = true;

  aSVGElement->DidChangeViewBox(true);
  if (mAnimVal) {
    aSVGElement->AnimationNeedsResample();
  }
}

static nsresult
ToSVGViewBoxRect(const nsAString& aStr, nsSVGViewBoxRect *aViewBox)
{
  nsCharSeparatedTokenizerTemplate<IsSVGWhitespace>
    tokenizer(aStr, ',',
              nsCharSeparatedTokenizer::SEPARATOR_OPTIONAL);
  float vals[NUM_VIEWBOX_COMPONENTS];
  PRUint32 i;
  for (i = 0; i < NUM_VIEWBOX_COMPONENTS && tokenizer.hasMoreTokens(); ++i) {
    NS_ConvertUTF16toUTF8 utf8Token(tokenizer.nextToken());
    const char *token = utf8Token.get();
    if (*token == '\0') {
      return NS_ERROR_DOM_SYNTAX_ERR; 
    }

    char *end;
    vals[i] = float(PR_strtod(token, &end));
    if (*end != '\0' || !NS_finite(vals[i])) {
      return NS_ERROR_DOM_SYNTAX_ERR; 
    }
  }

  if (i != NUM_VIEWBOX_COMPONENTS ||              
      tokenizer.hasMoreTokens() ||                
      tokenizer.lastTokenEndedWithSeparator()) {  
    return NS_ERROR_DOM_SYNTAX_ERR;
  }

  aViewBox->x = vals[0];
  aViewBox->y = vals[1];
  aViewBox->width = vals[2];
  aViewBox->height = vals[3];

  return NS_OK;
}

nsresult
nsSVGViewBox::SetBaseValueString(const nsAString& aValue,
                                 nsSVGElement *aSVGElement)
{
  nsSVGViewBoxRect viewBox;
  nsresult res = ToSVGViewBoxRect(aValue, &viewBox);
  if (NS_SUCCEEDED(res)) {
    mBaseVal = nsSVGViewBoxRect(viewBox.x, viewBox.y, viewBox.width, viewBox.height);
    mHasBaseVal = true;

    if (mAnimVal) {
      aSVGElement->AnimationNeedsResample();
    }
    
    
    
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
                     mSVGElement);
  return NS_OK;
}

NS_IMETHODIMP
nsSVGViewBox::DOMBaseVal::SetY(float aY)
{
  nsSVGViewBoxRect rect = mVal->GetBaseValue();
  rect.y = aY;
  mVal->SetBaseValue(rect.x, rect.y, rect.width, rect.height,
                     mSVGElement);
  return NS_OK;
}

NS_IMETHODIMP
nsSVGViewBox::DOMBaseVal::SetWidth(float aWidth)
{
  nsSVGViewBoxRect rect = mVal->GetBaseValue();
  rect.width = aWidth;
  mVal->SetBaseValue(rect.x, rect.y, rect.width, rect.height,
                     mSVGElement);
  return NS_OK;
}

NS_IMETHODIMP
nsSVGViewBox::DOMBaseVal::SetHeight(float aHeight)
{
  nsSVGViewBoxRect rect = mVal->GetBaseValue();
  rect.height = aHeight;
  mVal->SetBaseValue(rect.x, rect.y, rect.width, rect.height,
                     mSVGElement);
  return NS_OK;
}

nsISMILAttr*
nsSVGViewBox::ToSMILAttr(nsSVGElement *aSVGElement)
{
  return new SMILViewBox(this, aSVGElement);
}

nsresult
nsSVGViewBox::SMILViewBox
            ::ValueFromString(const nsAString& aStr,
                              const nsISMILAnimationElement* ,
                              nsSMILValue& aValue,
                              bool& aPreventCachingOfSandwich) const
{
  nsSVGViewBoxRect viewBox;
  nsresult res = ToSVGViewBoxRect(aStr, &viewBox);
  if (NS_FAILED(res)) {
    return res;
  }
  nsSMILValue val(&SVGViewBoxSMILType::sSingleton);
  *static_cast<nsSVGViewBoxRect*>(val.mU.mPtr) = viewBox;
  aValue.Swap(val);
  aPreventCachingOfSandwich = false;
  
  return NS_OK;
}

nsSMILValue
nsSVGViewBox::SMILViewBox::GetBaseValue() const
{
  nsSMILValue val(&SVGViewBoxSMILType::sSingleton);
  *static_cast<nsSVGViewBoxRect*>(val.mU.mPtr) = mVal->mBaseVal;
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
