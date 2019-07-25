



































#include "DOMSVGLength.h"
#include "DOMSVGLengthList.h"
#include "DOMSVGAnimatedLengthList.h"
#include "SVGLength.h"
#include "SVGAnimatedLengthList.h"
#include "nsSVGElement.h"
#include "nsIDOMSVGLength.h"
#include "nsDOMError.h"



namespace mozilla {





NS_IMPL_CYCLE_COLLECTION_CLASS(DOMSVGLength)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(DOMSVGLength)
  
  if (tmp->mList) {
    tmp->mList->mItems[tmp->mListIndex] = nsnull;
  }
NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mList)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(DOMSVGLength)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mList)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(DOMSVGLength)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DOMSVGLength)

}
DOMCI_DATA(SVGLength, DOMSVGLength)
namespace mozilla {

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMSVGLength)
  NS_INTERFACE_MAP_ENTRY(DOMSVGLength) 
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGLength)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGLength)
NS_INTERFACE_MAP_END

DOMSVGLength::DOMSVGLength(DOMSVGLengthList *aList,
                           PRUint32 aAttrEnum,
                           PRUint8 aListIndex,
                           PRUint8 aIsAnimValItem)
  : mList(aList)
  , mListIndex(aListIndex)
  , mAttrEnum(aAttrEnum)
  , mIsAnimValItem(aIsAnimValItem)
  , mUnit(nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER)
  , mValue(0.0f)
{
#ifdef DEBUG
  
  NS_ABORT_IF_FALSE(aList &&
                    aAttrEnum < (1 << 22) &&
                    aListIndex < (1 << 4) &&
                    aIsAnimValItem < (1 << 1), "bad arg");
  if (aIsAnimValItem &&
      mListIndex >= Element()->GetAnimatedLengthList(mAttrEnum)->GetAnimValue().Length() ||
      !aIsAnimValItem &&
      mListIndex >= Element()->GetAnimatedLengthList(mAttrEnum)->GetBaseValue().Length()) {
    NS_ABORT_IF_FALSE(0, "Bad aListIndex!");
    mList = nsnull;
  }
#endif
}

DOMSVGLength::DOMSVGLength()
  : mList(nsnull)
  , mListIndex(0)
  , mAttrEnum(0)
  , mIsAnimValItem(0)
  , mUnit(nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER)
  , mValue(0.0f)
{
}

NS_IMETHODIMP
DOMSVGLength::GetUnitType(PRUint16* aUnit)
{
#ifdef MOZ_SMIL
  if (mIsAnimValItem && HasOwner()) {
    Element()->FlushAnimations(); 
  }
#endif
  *aUnit = HasOwner() ? InternalItem().GetUnit() : mUnit;
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGLength::GetValue(float* aValue)
{
#ifdef MOZ_SMIL
  if (mIsAnimValItem && HasOwner()) {
    Element()->FlushAnimations(); 
  }
#endif
  if (HasOwner()) {
    *aValue = InternalItem().GetValueInUserUnits(Element(), Axis());
    if (NS_FloatIsFinite(*aValue)) {
      return NS_OK;
    }
  } else if (mUnit == nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER ||
             mUnit == nsIDOMSVGLength::SVG_LENGTHTYPE_PX) {
    *aValue = mValue;
    return NS_OK;
  }
  
  
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
DOMSVGLength::SetValue(float aUserUnitValue)
{
  if (mIsAnimValItem) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  NS_ENSURE_FINITE(aUserUnitValue, NS_ERROR_ILLEGAL_VALUE);

  
  
  
  

  if (HasOwner()) {
    if (InternalItem().SetFromUserUnitValue(aUserUnitValue, Element(), Axis())) {
      Element()->DidChangeLengthList(mAttrEnum, PR_TRUE);
#ifdef MOZ_SMIL
      if (mList->mAList->IsAnimating()) {
        Element()->AnimationNeedsResample();
      }
#endif
      return NS_OK;
    }
  } else if (mUnit == nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER ||
             mUnit == nsIDOMSVGLength::SVG_LENGTHTYPE_PX) {
    mValue = aUserUnitValue;
    return NS_OK;
  }
  
  
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
DOMSVGLength::GetValueInSpecifiedUnits(float* aValue)
{
#ifdef MOZ_SMIL
  if (mIsAnimValItem && HasOwner()) {
    Element()->FlushAnimations(); 
  }
#endif
  *aValue = HasOwner() ? InternalItem().GetValueInCurrentUnits() : mValue;
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGLength::SetValueInSpecifiedUnits(float aValue)
{
  if (mIsAnimValItem) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  NS_ENSURE_FINITE(aValue, NS_ERROR_ILLEGAL_VALUE);

  if (HasOwner()) {
    InternalItem().SetValueInCurrentUnits(aValue);
    Element()->DidChangeLengthList(mAttrEnum, PR_TRUE);
#ifdef MOZ_SMIL
    if (mList->mAList->IsAnimating()) {
      Element()->AnimationNeedsResample();
    }
#endif
    return NS_OK;
  }
  mValue = aValue;
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGLength::SetValueAsString(const nsAString& aValue)
{
  if (mIsAnimValItem) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  SVGLength value;
  if (!value.SetValueFromString(aValue)) {
    return NS_ERROR_DOM_SYNTAX_ERR;
  }
  if (HasOwner()) {
    InternalItem() = value;
    Element()->DidChangeLengthList(mAttrEnum, PR_TRUE);
#ifdef MOZ_SMIL
    if (mList->mAList->IsAnimating()) {
      Element()->AnimationNeedsResample();
    }
#endif
    return NS_OK;
  }
  mValue = value.GetValueInCurrentUnits();
  mUnit = value.GetUnit();
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGLength::GetValueAsString(nsAString& aValue)
{
#ifdef MOZ_SMIL
  if (mIsAnimValItem && HasOwner()) {
    Element()->FlushAnimations(); 
  }
#endif
  if (HasOwner()) {
    InternalItem().GetValueAsString(aValue);
    return NS_OK;
  }
  SVGLength(mValue, mUnit).GetValueAsString(aValue);
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGLength::NewValueSpecifiedUnits(PRUint16 aUnit, float aValue)
{
  if (mIsAnimValItem) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  NS_ENSURE_FINITE(aValue, NS_ERROR_ILLEGAL_VALUE);

  if (!SVGLength::IsValidUnitType(aUnit)) {
    return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
  }
  if (HasOwner()) {
    InternalItem().SetValueAndUnit(aValue, aUnit);
    Element()->DidChangeLengthList(mAttrEnum, PR_TRUE);
#ifdef MOZ_SMIL
    if (mList->mAList->IsAnimating()) {
      Element()->AnimationNeedsResample();
    }
#endif
    return NS_OK;
  }
  mUnit = PRUint8(aUnit);
  mValue = aValue;
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGLength::ConvertToSpecifiedUnits(PRUint16 aUnit)
{
  if (mIsAnimValItem) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  if (!SVGLength::IsValidUnitType(aUnit)) {
    return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
  }
  if (HasOwner()) {
    if (InternalItem().ConvertToUnit(PRUint8(aUnit), Element(), Axis())) {
      return NS_OK;
    }
  } else {
    SVGLength len(mValue, mUnit);
    if (len.ConvertToUnit(PRUint8(aUnit), nsnull, 0)) {
      mValue = len.GetValueInCurrentUnits();
      mUnit = aUnit;
      return NS_OK;
    }
  }
  
  
  return NS_ERROR_FAILURE;
}

void
DOMSVGLength::InsertingIntoList(DOMSVGLengthList *aList,
                                PRUint32 aAttrEnum,
                                PRUint8 aListIndex,
                                PRUint8 aIsAnimValItem)
{
  NS_ASSERTION(!HasOwner(), "Inserting item that is already in a list");
  NS_ASSERTION(mIsAnimValItem &&
               aListIndex < aList->Element()->GetAnimatedLengthList(aAttrEnum)->GetAnimValue().Length() ||
               !aIsAnimValItem &&
               aListIndex < aList->Element()->GetAnimatedLengthList(aAttrEnum)->GetBaseValue().Length(),
               "mListIndex too big");

  mList = aList;
  mAttrEnum = aAttrEnum;
  mListIndex = aListIndex;
  mIsAnimValItem = aIsAnimValItem;
}

void
DOMSVGLength::RemovingFromList()
{
  mValue = InternalItem().GetValueInCurrentUnits();
  mUnit  = InternalItem().GetUnit();
  mList = nsnull;
  mIsAnimValItem = 0;
}

SVGLength
DOMSVGLength::ToSVGLength()
{
  if (HasOwner()) {
    return SVGLength(InternalItem().GetValueInCurrentUnits(),
                     InternalItem().GetUnit());
  }
  return SVGLength(mValue, mUnit);
}

SVGLength&
DOMSVGLength::InternalItem()
{
  SVGAnimatedLengthList *alist = Element()->GetAnimatedLengthList(mAttrEnum);
  return mIsAnimValItem && alist->mAnimVal ?
    (*alist->mAnimVal)[mListIndex] :
    alist->mBaseVal[mListIndex];
}

} 
