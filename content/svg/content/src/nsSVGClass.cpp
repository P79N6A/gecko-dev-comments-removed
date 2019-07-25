



































#include "nsSVGClass.h"
#ifdef MOZ_SMIL
#include "nsSMILValue.h"
#include "SMILStringType.h"
#endif 

using namespace mozilla;

NS_SVG_VAL_IMPL_CYCLE_COLLECTION(nsSVGClass::DOMAnimatedString, mSVGElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsSVGClass::DOMAnimatedString)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsSVGClass::DOMAnimatedString)

DOMCI_DATA(SVGAnimatedClass, nsSVGClass::DOMAnimatedString)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsSVGClass::DOMAnimatedString)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedString)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGAnimatedString)
NS_INTERFACE_MAP_END



void
nsSVGClass::SetBaseValue(const nsAString& aValue,
                         nsSVGElement *aSVGElement,
                         bool aDoSetAttr)
{
  NS_ASSERTION(aSVGElement, "Null element passed to SetBaseValue");

  aSVGElement->SetFlags(NODE_MAY_HAVE_CLASS);
  if (aDoSetAttr) {
    aSVGElement->SetAttr(kNameSpaceID_None, nsGkAtoms::_class, aValue, PR_TRUE);
  }
#ifdef MOZ_SMIL
  if (mAnimVal) {
    aSVGElement->AnimationNeedsResample();
  }
#endif
}

void
nsSVGClass::GetAnimValue(nsAString& aResult, const nsSVGElement *aSVGElement) const
{
  if (mAnimVal) {
    aResult = *mAnimVal;
    return;
  }

  aSVGElement->GetAttr(kNameSpaceID_None, nsGkAtoms::_class, aResult);
}

void
nsSVGClass::SetAnimValue(const nsAString& aValue, nsSVGElement *aSVGElement)
{
  if (!mAnimVal) {
    mAnimVal = new nsString();
  }
  *mAnimVal = aValue;
  aSVGElement->SetFlags(NODE_MAY_HAVE_CLASS);
  aSVGElement->DidAnimateClass();
}

nsresult
nsSVGClass::ToDOMAnimatedString(nsIDOMSVGAnimatedString **aResult,
                                nsSVGElement *aSVGElement)
{
  *aResult = new DOMAnimatedString(this, aSVGElement);
  NS_ADDREF(*aResult);
  return NS_OK;
}

#ifdef MOZ_SMIL
nsISMILAttr*
nsSVGClass::ToSMILAttr(nsSVGElement *aSVGElement)
{
  return new SMILString(this, aSVGElement);
}

nsresult
nsSVGClass::SMILString::ValueFromString(const nsAString& aStr,
                                        const nsISMILAnimationElement* ,
                                        nsSMILValue& aValue,
                                        bool& aPreventCachingOfSandwich) const
{
  nsSMILValue val(&SMILStringType::sSingleton);

  *static_cast<nsAString*>(val.mU.mPtr) = aStr;
  aValue.Swap(val);
  aPreventCachingOfSandwich = PR_FALSE;
  return NS_OK;
}

nsSMILValue
nsSVGClass::SMILString::GetBaseValue() const
{
  nsSMILValue val(&SMILStringType::sSingleton);
  mSVGElement->GetAttr(kNameSpaceID_None, nsGkAtoms::_class,
                       *static_cast<nsAString*>(val.mU.mPtr));
  return val;
}

void
nsSVGClass::SMILString::ClearAnimValue()
{
  if (mVal->mAnimVal) {
    mVal->mAnimVal = nsnull;
    mSVGElement->DidAnimateClass();
  }
}

nsresult
nsSVGClass::SMILString::SetAnimValue(const nsSMILValue& aValue)
{
  NS_ASSERTION(aValue.mType == &SMILStringType::sSingleton,
               "Unexpected type to assign animated value");
  if (aValue.mType == &SMILStringType::sSingleton) {
    mVal->SetAnimValue(*static_cast<nsAString*>(aValue.mU.mPtr), mSVGElement);
  }
  return NS_OK;
}
#endif 
