




#include "DOMSVGNumber.h"
#include "DOMSVGNumberList.h"
#include "DOMSVGAnimatedNumberList.h"
#include "SVGAnimatedNumberList.h"
#include "nsSVGElement.h"
#include "nsIDOMSVGNumber.h"
#include "nsError.h"
#include "nsContentUtils.h" 



namespace mozilla {





NS_IMPL_CYCLE_COLLECTION_CLASS(DOMSVGNumber)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(DOMSVGNumber)
  
  if (tmp->mList) {
    tmp->mList->mItems[tmp->mListIndex] = nullptr;
  }
NS_IMPL_CYCLE_COLLECTION_UNLINK(mList)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(DOMSVGNumber)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mList)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(DOMSVGNumber)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DOMSVGNumber)
}
DOMCI_DATA(SVGNumber, mozilla::DOMSVGNumber)

namespace mozilla {
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMSVGNumber)
  NS_INTERFACE_MAP_ENTRY(mozilla::DOMSVGNumber) 
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGNumber)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGNumber)
NS_INTERFACE_MAP_END





class MOZ_STACK_CLASS AutoChangeNumberNotifier
{
public:
  AutoChangeNumberNotifier(DOMSVGNumber* aNumber MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
    : mNumber(aNumber)
  {
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    MOZ_ASSERT(mNumber, "Expecting non-null number");
    MOZ_ASSERT(mNumber->HasOwner(),
               "Expecting list to have an owner for notification");
    mEmptyOrOldValue =
      mNumber->Element()->WillChangeNumberList(mNumber->mAttrEnum);
  }

  ~AutoChangeNumberNotifier()
  {
    mNumber->Element()->DidChangeNumberList(mNumber->mAttrEnum,
                                            mEmptyOrOldValue);
    if (mNumber->mList->IsAnimating()) {
      mNumber->Element()->AnimationNeedsResample();
    }
  }

private:
  DOMSVGNumber* const mNumber;
  nsAttrValue   mEmptyOrOldValue;
  MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

DOMSVGNumber::DOMSVGNumber(DOMSVGNumberList *aList,
                           uint8_t aAttrEnum,
                           uint32_t aListIndex,
                           bool aIsAnimValItem)
  : mList(aList)
  , mListIndex(aListIndex)
  , mAttrEnum(aAttrEnum)
  , mIsAnimValItem(aIsAnimValItem)
  , mValue(0.0f)
{
  
  NS_ABORT_IF_FALSE(aList &&
                    aAttrEnum < (1 << 4) &&
                    aListIndex <= MaxListIndex(), "bad arg");

  NS_ABORT_IF_FALSE(IndexIsValid(), "Bad index for DOMSVGNumber!");
}

DOMSVGNumber::DOMSVGNumber()
  : mList(nullptr)
  , mListIndex(0)
  , mAttrEnum(0)
  , mIsAnimValItem(false)
  , mValue(0.0f)
{
}

NS_IMETHODIMP
DOMSVGNumber::GetValue(float* aValue)
{
  if (mIsAnimValItem && HasOwner()) {
    Element()->FlushAnimations(); 
  }
  *aValue = HasOwner() ? InternalItem() : mValue;
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGNumber::SetValue(float aValue)
{
  if (mIsAnimValItem) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  NS_ENSURE_FINITE(aValue, NS_ERROR_ILLEGAL_VALUE);

  if (HasOwner()) {
    if (InternalItem() == aValue) {
      return NS_OK;
    }
    AutoChangeNumberNotifier notifier(this);
    InternalItem() = aValue;
    return NS_OK;
  }
  mValue = aValue;
  return NS_OK;
}

void
DOMSVGNumber::InsertingIntoList(DOMSVGNumberList *aList,
                                uint8_t aAttrEnum,
                                uint32_t aListIndex,
                                bool aIsAnimValItem)
{
  NS_ASSERTION(!HasOwner(), "Inserting item that is already in a list");

  mList = aList;
  mAttrEnum = aAttrEnum;
  mListIndex = aListIndex;
  mIsAnimValItem = aIsAnimValItem;

  NS_ABORT_IF_FALSE(IndexIsValid(), "Bad index for DOMSVGNumber!");
}

void
DOMSVGNumber::RemovingFromList()
{
  mValue = InternalItem();
  mList = nullptr;
  mIsAnimValItem = false;
}

float
DOMSVGNumber::ToSVGNumber()
{
  return HasOwner() ? InternalItem() : mValue;
}

float&
DOMSVGNumber::InternalItem()
{
  SVGAnimatedNumberList *alist = Element()->GetAnimatedNumberList(mAttrEnum);
  return mIsAnimValItem && alist->mAnimVal ?
    (*alist->mAnimVal)[mListIndex] :
    alist->mBaseVal[mListIndex];
}

#ifdef DEBUG
bool
DOMSVGNumber::IndexIsValid()
{
  SVGAnimatedNumberList *alist = Element()->GetAnimatedNumberList(mAttrEnum);
  return (mIsAnimValItem &&
          mListIndex < alist->GetAnimValue().Length()) ||
         (!mIsAnimValItem &&
          mListIndex < alist->GetBaseValue().Length());
}
#endif

} 
