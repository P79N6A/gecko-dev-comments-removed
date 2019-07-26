




#include "nsSVGElement.h"
#include "DOMSVGNumberList.h"
#include "DOMSVGNumber.h"
#include "nsError.h"
#include "SVGAnimatedNumberList.h"
#include "nsCOMPtr.h"
#include "nsContentUtils.h"
#include "dombindings.h"



namespace mozilla {


namespace {

using mozilla::DOMSVGNumber;

void UpdateListIndicesFromIndex(nsTArray<DOMSVGNumber*>& aItemsArray,
                                uint32_t aStartingIndex)
{
  uint32_t length = aItemsArray.Length();

  for (uint32_t i = aStartingIndex; i < length; ++i) {
    if (aItemsArray[i]) {
      aItemsArray[i]->UpdateListIndex(i);
    }
  }
}

} 





NS_IMPL_CYCLE_COLLECTION_CLASS(DOMSVGNumberList)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(DOMSVGNumberList)
  if (tmp->mAList) {
    if (tmp->IsAnimValList()) {
      tmp->mAList->mAnimVal = nullptr;
    } else {
      tmp->mAList->mBaseVal = nullptr;
    }
    NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mAList)
  }
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(DOMSVGNumberList)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mAList)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(DOMSVGNumberList)
  NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(DOMSVGNumberList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DOMSVGNumberList)

} 
DOMCI_DATA(SVGNumberList, mozilla::DOMSVGNumberList)
namespace mozilla {

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMSVGNumberList)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGNumberList)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGNumberList)
NS_INTERFACE_MAP_END


JSObject*
DOMSVGNumberList::WrapObject(JSContext *cx, JSObject *scope, bool *triedToWrap)
{
  return mozilla::dom::oldproxybindings::SVGNumberList::create(cx, scope, this,
                                                      triedToWrap);
}

nsIDOMSVGNumber*
DOMSVGNumberList::GetItemAt(uint32_t aIndex)
{
  ErrorResult rv;
  return GetItem(aIndex, rv);
}

void
DOMSVGNumberList::InternalListLengthWillChange(uint32_t aNewLength)
{
  uint32_t oldLength = mItems.Length();

  if (aNewLength > DOMSVGNumber::MaxListIndex()) {
    
    
    aNewLength = DOMSVGNumber::MaxListIndex();
  }

  nsRefPtr<DOMSVGNumberList> kungFuDeathGrip;
  if (aNewLength < oldLength) {
    
    
    kungFuDeathGrip = this;
  }

  
  for (uint32_t i = aNewLength; i < oldLength; ++i) {
    if (mItems[i]) {
      mItems[i]->RemovingFromList();
    }
  }

  if (!mItems.SetLength(aNewLength)) {
    
    
    mItems.Clear();
    return;
  }

  
  for (uint32_t i = oldLength; i < aNewLength; ++i) {
    mItems[i] = nullptr;
  }
}

SVGNumberList&
DOMSVGNumberList::InternalList() const
{
  SVGAnimatedNumberList *alist = Element()->GetAnimatedNumberList(AttrEnum());
  return IsAnimValList() && alist->mAnimVal ? *alist->mAnimVal : alist->mBaseVal;
}




NS_IMETHODIMP
DOMSVGNumberList::GetNumberOfItems(uint32_t *aNumberOfItems)
{
  *aNumberOfItems = NumberOfItems();
  return NS_OK;
}

void
DOMSVGNumberList::Clear(ErrorResult& error)
{
  if (IsAnimValList()) {
    error.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  if (LengthNoFlush() > 0) {
    nsAttrValue emptyOrOldValue = Element()->WillChangeNumberList(AttrEnum());
    
    
    
    mAList->InternalBaseValListWillChangeTo(SVGNumberList());

    mItems.Clear();
    InternalList().Clear();
    Element()->DidChangeNumberList(AttrEnum(), emptyOrOldValue);
    if (mAList->IsAnimating()) {
      Element()->AnimationNeedsResample();
    }
  }
}

NS_IMETHODIMP
DOMSVGNumberList::Clear()
{
  ErrorResult rv;
  Clear(rv);
  return rv.ErrorCode();
}

already_AddRefed<nsIDOMSVGNumber>
DOMSVGNumberList::Initialize(nsIDOMSVGNumber *newItem,
                             ErrorResult& error)
{
  if (IsAnimValList()) {
    error.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return nullptr;
  }

  
  
  
  
  
  
  

  nsCOMPtr<DOMSVGNumber> domItem = do_QueryInterface(newItem);
  if (!domItem) {
    error.Throw(NS_ERROR_DOM_SVG_WRONG_TYPE_ERR);
    return nullptr;
  }
  if (domItem->HasOwner()) {
    newItem = domItem->Clone();
  }

  Clear();
  return InsertItemBefore(newItem, 0, error);
}

NS_IMETHODIMP
DOMSVGNumberList::Initialize(nsIDOMSVGNumber *newItem,
                             nsIDOMSVGNumber **_retval)
{
  ErrorResult rv;
  *_retval = Initialize(newItem, rv).get();
  return rv.ErrorCode();
}

nsIDOMSVGNumber*
DOMSVGNumberList::IndexedGetter(uint32_t index, bool& found, ErrorResult& error)
{
  if (IsAnimValList()) {
    Element()->FlushAnimations();
  }
  found = index < LengthNoFlush();
  if (found) {
    EnsureItemAt(index);
    return mItems[index];
  }
  return nullptr;
}

NS_IMETHODIMP
DOMSVGNumberList::GetItem(uint32_t index,
                          nsIDOMSVGNumber **_retval)
{
  ErrorResult rv;
  NS_IF_ADDREF(*_retval = GetItem(index, rv));
  return rv.ErrorCode();
}

already_AddRefed<nsIDOMSVGNumber>
DOMSVGNumberList::InsertItemBefore(nsIDOMSVGNumber *newItem,
                                   uint32_t index,
                                   ErrorResult& error)
{
  if (IsAnimValList()) {
    error.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return nullptr;
  }

  index = NS_MIN(index, LengthNoFlush());
  if (index >= DOMSVGNumber::MaxListIndex()) {
    error.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return nullptr;
  }

  nsCOMPtr<DOMSVGNumber> domItem = do_QueryInterface(newItem);
  if (!domItem) {
    error.Throw(NS_ERROR_DOM_SVG_WRONG_TYPE_ERR);
    return nullptr;
  }
  if (domItem->HasOwner()) {
    domItem = domItem->Clone(); 
  }

  
  if (!mItems.SetCapacity(mItems.Length() + 1) ||
      !InternalList().SetCapacity(InternalList().Length() + 1)) {
    error.Throw(NS_ERROR_OUT_OF_MEMORY);
    return nullptr;
  }

  nsAttrValue emptyOrOldValue = Element()->WillChangeNumberList(AttrEnum());
  
  MaybeInsertNullInAnimValListAt(index);

  InternalList().InsertItem(index, domItem->ToSVGNumber());
  mItems.InsertElementAt(index, domItem.get());

  
  
  
  domItem->InsertingIntoList(this, AttrEnum(), index, IsAnimValList());

  UpdateListIndicesFromIndex(mItems, index + 1);

  Element()->DidChangeNumberList(AttrEnum(), emptyOrOldValue);
  if (mAList->IsAnimating()) {
    Element()->AnimationNeedsResample();
  }
  return domItem.forget();
}

NS_IMETHODIMP
DOMSVGNumberList::InsertItemBefore(nsIDOMSVGNumber *newItem,
                                   uint32_t index,
                                   nsIDOMSVGNumber **_retval)
{
  ErrorResult rv;
  *_retval = InsertItemBefore(newItem, index, rv).get();
  return rv.ErrorCode();
}

already_AddRefed<nsIDOMSVGNumber>
DOMSVGNumberList::ReplaceItem(nsIDOMSVGNumber *newItem,
                              uint32_t index,
                              ErrorResult& error)
{
  if (IsAnimValList()) {
    error.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return nullptr;
  }

  nsCOMPtr<DOMSVGNumber> domItem = do_QueryInterface(newItem);
  if (!domItem) {
    error.Throw(NS_ERROR_DOM_SVG_WRONG_TYPE_ERR);
    return nullptr;
  }
  if (index >= LengthNoFlush()) {
    error.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return nullptr;
  }
  if (domItem->HasOwner()) {
    domItem = domItem->Clone(); 
  }

  nsAttrValue emptyOrOldValue = Element()->WillChangeNumberList(AttrEnum());
  if (mItems[index]) {
    
    
    mItems[index]->RemovingFromList();
  }

  InternalList()[index] = domItem->ToSVGNumber();
  mItems[index] = domItem;

  
  
  domItem->InsertingIntoList(this, AttrEnum(), index, IsAnimValList());

  Element()->DidChangeNumberList(AttrEnum(), emptyOrOldValue);
  if (mAList->IsAnimating()) {
    Element()->AnimationNeedsResample();
  }
  return domItem.forget();
}

NS_IMETHODIMP
DOMSVGNumberList::ReplaceItem(nsIDOMSVGNumber *newItem,
                              uint32_t index,
                              nsIDOMSVGNumber **_retval)
{
  ErrorResult rv;
  *_retval = ReplaceItem(newItem, index, rv).get();
  return rv.ErrorCode();
}

already_AddRefed<nsIDOMSVGNumber>
DOMSVGNumberList::RemoveItem(uint32_t index,
                             ErrorResult& error)
{
  if (IsAnimValList()) {
    error.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return nullptr;
  }

  if (index >= LengthNoFlush()) {
    error.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return nullptr;
  }

  
  
  
  MaybeRemoveItemFromAnimValListAt(index);

  
  EnsureItemAt(index);

  nsAttrValue emptyOrOldValue = Element()->WillChangeNumberList(AttrEnum());
  
  
  mItems[index]->RemovingFromList();
  nsCOMPtr<nsIDOMSVGNumber> result = mItems[index];

  InternalList().RemoveItem(index);
  mItems.RemoveElementAt(index);

  UpdateListIndicesFromIndex(mItems, index);

  Element()->DidChangeNumberList(AttrEnum(), emptyOrOldValue);
  if (mAList->IsAnimating()) {
    Element()->AnimationNeedsResample();
  }
  return result.forget();
}

NS_IMETHODIMP
DOMSVGNumberList::RemoveItem(uint32_t index,
                             nsIDOMSVGNumber **_retval)
{
  ErrorResult rv;
  *_retval = RemoveItem(index, rv).get();
  return rv.ErrorCode();
}

NS_IMETHODIMP
DOMSVGNumberList::AppendItem(nsIDOMSVGNumber *newItem,
                             nsIDOMSVGNumber **_retval)
{
  ErrorResult rv;
  *_retval = AppendItem(newItem, rv).get();
  return rv.ErrorCode();
}

NS_IMETHODIMP
DOMSVGNumberList::GetLength(uint32_t *aLength)
{
  *aLength = Length();
  return NS_OK;
}

void
DOMSVGNumberList::EnsureItemAt(uint32_t aIndex)
{
  if (!mItems[aIndex]) {
    mItems[aIndex] = new DOMSVGNumber(this, AttrEnum(), aIndex, IsAnimValList());
  }
}

void
DOMSVGNumberList::MaybeInsertNullInAnimValListAt(uint32_t aIndex)
{
  NS_ABORT_IF_FALSE(!IsAnimValList(), "call from baseVal to animVal");

  DOMSVGNumberList* animVal = mAList->mAnimVal;

  if (!animVal || mAList->IsAnimating()) {
    
    return;
  }

  NS_ABORT_IF_FALSE(animVal->mItems.Length() == mItems.Length(),
                    "animVal list not in sync!");

  animVal->mItems.InsertElementAt(aIndex, static_cast<DOMSVGNumber*>(nullptr));

  UpdateListIndicesFromIndex(animVal->mItems, aIndex + 1);
}

void
DOMSVGNumberList::MaybeRemoveItemFromAnimValListAt(uint32_t aIndex)
{
  NS_ABORT_IF_FALSE(!IsAnimValList(), "call from baseVal to animVal");

  
  
  nsRefPtr<DOMSVGNumberList> animVal = mAList->mAnimVal;

  if (!animVal || mAList->IsAnimating()) {
    
    return;
  }

  NS_ABORT_IF_FALSE(animVal->mItems.Length() == mItems.Length(),
                    "animVal list not in sync!");

  if (animVal->mItems[aIndex]) {
    animVal->mItems[aIndex]->RemovingFromList();
  }
  animVal->mItems.RemoveElementAt(aIndex);

  UpdateListIndicesFromIndex(animVal->mItems, aIndex);
}

} 
