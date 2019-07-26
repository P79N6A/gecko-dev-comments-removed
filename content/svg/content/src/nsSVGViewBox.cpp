




#include "nsSVGViewBox.h"
#include "prdtoa.h"
#include "nsTextFormatter.h"
#include "nsCharSeparatedTokenizer.h"
#include "nsMathUtils.h"
#include "nsSMILValue.h"
#include "SVGContentUtils.h"
#include "SVGViewBoxSMILType.h"
#include "nsAttrValueInlines.h"

#define NUM_VIEWBOX_COMPONENTS 4
using namespace mozilla;



bool
nsSVGViewBoxRect::operator==(const nsSVGViewBoxRect& aOther) const
{
  if (&aOther == this)
    return true;

  return (none && aOther.none) ||
    (!none && !aOther.none &&
     x == aOther.x &&
     y == aOther.y &&
     width == aOther.width &&
     height == aOther.height);
}



NS_SVG_VAL_IMPL_CYCLE_COLLECTION(nsSVGViewBox::DOMBaseVal, mSVGElement)
NS_SVG_VAL_IMPL_CYCLE_COLLECTION(nsSVGViewBox::DOMAnimVal, mSVGElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsSVGViewBox::DOMBaseVal)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsSVGViewBox::DOMBaseVal)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsSVGViewBox::DOMAnimVal)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsSVGViewBox::DOMAnimVal)

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

static nsSVGAttrTearoffTable<nsSVGViewBox, nsSVGViewBox::DOMBaseVal>
  sBaseSVGViewBoxTearoffTable;
static nsSVGAttrTearoffTable<nsSVGViewBox, nsSVGViewBox::DOMAnimVal>
  sAnimSVGViewBoxTearoffTable;
nsSVGAttrTearoffTable<nsSVGViewBox, mozilla::dom::SVGAnimatedRect>
  nsSVGViewBox::sSVGAnimatedRectTearoffTable;




void
nsSVGViewBox::Init()
{
  mHasBaseVal = false;
  mAnimVal = nullptr;
}

void
nsSVGViewBox::SetAnimValue(const nsSVGViewBoxRect& aRect,
                           nsSVGElement *aSVGElement)
{
  if (!mAnimVal) {
    
    mAnimVal = new nsSVGViewBoxRect(aRect);
  } else {
    if (aRect == *mAnimVal) {
      return;
    }
    *mAnimVal = aRect;
  }
  aSVGElement->DidAnimateViewBox();
}

void
nsSVGViewBox::SetBaseValue(const nsSVGViewBoxRect& aRect,
                           nsSVGElement *aSVGElement)
{
  if (!mHasBaseVal || mBaseVal == aRect) {
    
    
    
    
    mBaseVal = aRect;
    return;
  }

  nsAttrValue emptyOrOldValue = aSVGElement->WillChangeViewBox();

  mBaseVal = aRect;
  mHasBaseVal = true;

  aSVGElement->DidChangeViewBox(emptyOrOldValue);
  if (mAnimVal) {
    aSVGElement->AnimationNeedsResample();
  }
}

static nsresult
ToSVGViewBoxRect(const nsAString& aStr, nsSVGViewBoxRect *aViewBox)
{
  if (aStr.EqualsLiteral("none")) {
    aViewBox->none = true;
    return NS_OK;
  }

  nsCharSeparatedTokenizerTemplate<IsSVGWhitespace>
    tokenizer(aStr, ',',
              nsCharSeparatedTokenizer::SEPARATOR_OPTIONAL);
  float vals[NUM_VIEWBOX_COMPONENTS];
  uint32_t i;
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
  aViewBox->none = false;

  return NS_OK;
}

nsresult
nsSVGViewBox::SetBaseValueString(const nsAString& aValue,
                                 nsSVGElement *aSVGElement,
                                 bool aDoSetAttr)
{
  nsSVGViewBoxRect viewBox;

  nsresult rv = ToSVGViewBoxRect(aValue, &viewBox);
  if (NS_FAILED(rv)) {
    return rv;
  }
  if (viewBox == mBaseVal) {
    return NS_OK;
  }

  nsAttrValue emptyOrOldValue;
  if (aDoSetAttr) {
    emptyOrOldValue = aSVGElement->WillChangeViewBox();
  }
  mHasBaseVal = true;
  mBaseVal = viewBox;

  if (aDoSetAttr) {
    aSVGElement->DidChangeViewBox(emptyOrOldValue);
  }
  if (mAnimVal) {
    aSVGElement->AnimationNeedsResample();
  }
  return NS_OK;
}

void
nsSVGViewBox::GetBaseValueString(nsAString& aValue) const
{
  if (mBaseVal.none) {
    aValue.AssignLiteral("none");
    return;
  }
  PRUnichar buf[200];
  nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
                            NS_LITERAL_STRING("%g %g %g %g").get(),
                            (double)mBaseVal.x, (double)mBaseVal.y,
                            (double)mBaseVal.width, (double)mBaseVal.height);
  aValue.Assign(buf);
}

nsresult
nsSVGViewBox::ToDOMAnimatedRect(dom::SVGAnimatedRect **aResult,
                                nsSVGElement* aSVGElement)
{
  nsRefPtr<dom::SVGAnimatedRect> domAnimatedRect =
    sSVGAnimatedRectTearoffTable.GetTearoff(this);
  if (!domAnimatedRect) {
    domAnimatedRect = new dom::SVGAnimatedRect(this, aSVGElement);
    sSVGAnimatedRectTearoffTable.AddTearoff(this, domAnimatedRect);
  }

  domAnimatedRect.forget(aResult);
  return NS_OK;
}

nsresult
nsSVGViewBox::ToDOMBaseVal(dom::SVGIRect **aResult,
                           nsSVGElement *aSVGElement)
{
  if (!mHasBaseVal || mBaseVal.none) {
    *aResult = nullptr;
    return NS_OK;
  }
  nsRefPtr<DOMBaseVal> domBaseVal =
    sBaseSVGViewBoxTearoffTable.GetTearoff(this);
  if (!domBaseVal) {
    domBaseVal = new DOMBaseVal(this, aSVGElement);
    sBaseSVGViewBoxTearoffTable.AddTearoff(this, domBaseVal);
  }

  domBaseVal.forget(aResult);
  return NS_OK;
}

nsSVGViewBox::DOMBaseVal::~DOMBaseVal()
{
  sBaseSVGViewBoxTearoffTable.RemoveTearoff(mVal);
}

nsresult
nsSVGViewBox::ToDOMAnimVal(dom::SVGIRect **aResult,
                           nsSVGElement *aSVGElement)
{
  if ((mAnimVal && mAnimVal->none) ||
      (!mAnimVal && (!mHasBaseVal || mBaseVal.none))) {
    *aResult = nullptr;
    return NS_OK;
  }
  nsRefPtr<DOMAnimVal> domAnimVal =
    sAnimSVGViewBoxTearoffTable.GetTearoff(this);
  if (!domAnimVal) {
    domAnimVal = new DOMAnimVal(this, aSVGElement);
    sAnimSVGViewBoxTearoffTable.AddTearoff(this, domAnimVal);
  }

  domAnimVal.forget(aResult);
  return NS_OK;
}

nsSVGViewBox::DOMAnimVal::~DOMAnimVal()
{
  sAnimSVGViewBoxTearoffTable.RemoveTearoff(mVal);
}

void
nsSVGViewBox::DOMBaseVal::SetX(float aX, ErrorResult& aRv)
{
  nsSVGViewBoxRect rect = mVal->GetBaseValue();
  rect.x = aX;
  mVal->SetBaseValue(rect, mSVGElement);
}

void
nsSVGViewBox::DOMBaseVal::SetY(float aY, ErrorResult& aRv)
{
  nsSVGViewBoxRect rect = mVal->GetBaseValue();
  rect.y = aY;
  mVal->SetBaseValue(rect, mSVGElement);
}

void
nsSVGViewBox::DOMBaseVal::SetWidth(float aWidth, ErrorResult& aRv)
{
  nsSVGViewBoxRect rect = mVal->GetBaseValue();
  rect.width = aWidth;
  mVal->SetBaseValue(rect, mSVGElement);
}

void
nsSVGViewBox::DOMBaseVal::SetHeight(float aHeight, ErrorResult& aRv)
{
  nsSVGViewBoxRect rect = mVal->GetBaseValue();
  rect.height = aHeight;
  mVal->SetBaseValue(rect, mSVGElement);
}

nsISMILAttr*
nsSVGViewBox::ToSMILAttr(nsSVGElement *aSVGElement)
{
  return new SMILViewBox(this, aSVGElement);
}

nsresult
nsSVGViewBox::SMILViewBox
            ::ValueFromString(const nsAString& aStr,
                              const dom::SVGAnimationElement* ,
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
    mVal->mAnimVal = nullptr;
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
    mVal->SetAnimValue(vb, mSVGElement);
  }
  return NS_OK;
}
