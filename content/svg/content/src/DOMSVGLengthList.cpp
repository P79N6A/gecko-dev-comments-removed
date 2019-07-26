




#include "nsSVGElement.h"
#include "DOMSVGLengthList.h"
#include "DOMSVGLength.h"
#include "nsError.h"
#include "SVGAnimatedLengthList.h"
#include "nsCOMPtr.h"
#include "nsContentUtils.h"
#include "mozilla/dom/SVGLengthListBinding.h"




namespace {

using mozilla::DOMSVGLength;

void UpdateListIndicesFromIndex(nsTArray<DOMSVGLength*>& aItemsArray,
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

namespace mozilla {





NS_IMPL_CYCLE_COLLECTION_CLASS(DOMSVGLengthList)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(DOMSVGLengthList)
  if (tmp->mAList) {
    if (tmp->IsAnimValList()) {
      tmp->mAList->mAnimVal = nullptr;
    } else {
      tmp->mAList->mBaseVal = nullptr;
    }
    NS_IMPL_CYCLE_COLLECTION_UNLINK(mAList)
  }
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(DOMSVGLengthList)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mAList)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(DOMSVGLengthList)
  NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(DOMSVGLengthList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DOMSVGLengthList)

} 
DOMCI_DATA(SVGLengthList, mozilla::DOMSVGLengthList)
namespace mozilla {

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMSVGLengthList)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGLengthList)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGLengthList)
NS_INTERFACE_MAP_END

JSObject*
DOMSVGLengthList::WrapObject(JSContext *cx, JSObject *scope, bool *triedToWrap)
{
  return mozilla::dom::SVGLengthListBinding::Wrap(cx, scope, this, triedToWrap);
}

void
DOMSVGLengthList::InternalListLengthWillChange(uint32_t aNewLength)
{
  uint32_t oldLength = mItems.Length();

  if (aNewLength > DOMSVGLength::MaxListIndex()) {
    
    
    aNewLength = DOMSVGLength::MaxListIndex();
  }

  nsRefPtr<DOMSVGLengthList> kungFuDeathGrip;
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

SVGLengthList&
DOMSVGLengthList::InternalList() const
{
  SVGAnimatedLengthList *alist = Element()->GetAnimatedLengthList(AttrEnum());
  return IsAnimValList() && alist->mAnimVal ? *alist->mAnimVal : alist->mBaseVal;
}




NS_IMETHODIMP
DOMSVGLengthList::GetNumberOfItems(uint32_t *aNumberOfItems)
{
  *aNumberOfItems = NumberOfItems();
  return NS_OK;
}

void
DOMSVGLengthList::Clear(ErrorResult& aError)
{
  if (IsAnimValList()) {
    aError.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  if (LengthNoFlush() > 0) {
    nsAttrValue emptyOrOldValue = Element()->WillChangeLengthList(AttrEnum());
    
    
    
    mAList->InternalBaseValListWillChangeTo(SVGLengthList());

    mItems.Clear();
    InternalList().Clear();
    Element()->DidChangeLengthList(AttrEnum(), emptyOrOldValue);
    if (mAList->IsAnimating()) {
      Element()->AnimationNeedsResample();
    }
  }
}

NS_IMETHODIMP
DOMSVGLengthList::Clear()
{
  ErrorResult rv;
  Clear(rv);
  return rv.ErrorCode();
}

already_AddRefed<nsIDOMSVGLength>
DOMSVGLengthList::Initialize(nsIDOMSVGLength *newItem,
                             ErrorResult& error)
{
  if (IsAnimValList()) {
    error.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return nullptr;
  }

  
  
  
  
  
  
  

  nsCOMPtr<DOMSVGLength> domItem = do_QueryInterface(newItem);
  if (!domItem) {
    error.Throw(NS_ERROR_DOM_SVG_WRONG_TYPE_ERR);
    return nullptr;
  }
  if (domItem->HasOwner()) {
    newItem = domItem->Copy();
  }

  Clear();
  return InsertItemBefore(newItem, 0, error);
}

NS_IMETHODIMP
DOMSVGLengthList::Initialize(nsIDOMSVGLength *newItem,
                             nsIDOMSVGLength **_retval)
{
  ErrorResult rv;
  *_retval = Initialize(newItem, rv).get();
  return rv.ErrorCode();
}

nsIDOMSVGLength*
DOMSVGLengthList::IndexedGetter(uint32_t index, bool& found, ErrorResult& error)
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
DOMSVGLengthList::GetItem(uint32_t index,
                          nsIDOMSVGLength **_retval)
{
  ErrorResult rv;
  NS_IF_ADDREF(*_retval = GetItem(index, rv));
  return rv.ErrorCode();
}

already_AddRefed<nsIDOMSVGLength>
DOMSVGLengthList::InsertItemBefore(nsIDOMSVGLength *newItem,
                                   uint32_t index,
                                   ErrorResult& error)
{
  if (IsAnimValList()) {
    error.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return nullptr;
  }

  index = NS_MIN(index, LengthNoFlush());
  if (index >= DOMSVGLength::MaxListIndex()) {
    error.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return nullptr;
  }

  nsCOMPtr<DOMSVGLength> domItem = do_QueryInterface(newItem);
  if (!domItem) {
    error.Throw(NS_ERROR_DOM_SVG_WRONG_TYPE_ERR);
    return nullptr;
  }
  if (domItem->HasOwner()) {
    domItem = domItem->Copy(); 
  }

  
  if (!mItems.SetCapacity(mItems.Length() + 1) ||
      !InternalList().SetCapacity(InternalList().Length() + 1)) {
    error.Throw(NS_ERROR_OUT_OF_MEMORY);
    return nullptr;
  }

  nsAttrValue emptyOrOldValue = Element()->WillChangeLengthList(AttrEnum());
  
  MaybeInsertNullInAnimValListAt(index);

  InternalList().InsertItem(index, domItem->ToSVGLength());
  mItems.InsertElementAt(index, domItem.get());

  
  
  
  domItem->InsertingIntoList(this, AttrEnum(), index, IsAnimValList());

  UpdateListIndicesFromIndex(mItems, index + 1);

  Element()->DidChangeLengthList(AttrEnum(), emptyOrOldValue);
  if (mAList->IsAnimating()) {
    Element()->AnimationNeedsResample();
  }
  return domItem.forget();
}

NS_IMETHODIMP
DOMSVGLengthList::InsertItemBefore(nsIDOMSVGLength *newItem,
                                   uint32_t index,
                                   nsIDOMSVGLength **_retval)
{
  ErrorResult rv;
  *_retval = InsertItemBefore(newItem, index, rv).get();
  return rv.ErrorCode();
}

already_AddRefed<nsIDOMSVGLength>
DOMSVGLengthList::ReplaceItem(nsIDOMSVGLength *newItem,
                              uint32_t index,
                              ErrorResult& error)
{
  if (IsAnimValList()) {
    error.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return nullptr;
  }

  nsCOMPtr<DOMSVGLength> domItem = do_QueryInterface(newItem);
  if (!domItem) {
    error.Throw(NS_ERROR_DOM_SVG_WRONG_TYPE_ERR);
    return nullptr;
  }
  if (index >= LengthNoFlush()) {
    error.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return nullptr;
  }
  if (domItem->HasOwner()) {
    domItem = domItem->Copy(); 
  }

  nsAttrValue emptyOrOldValue = Element()->WillChangeLengthList(AttrEnum());
  if (mItems[index]) {
    
    
    mItems[index]->RemovingFromList();
  }

  InternalList()[index] = domItem->ToSVGLength();
  mItems[index] = domItem;

  
  
  domItem->InsertingIntoList(this, AttrEnum(), index, IsAnimValList());

  Element()->DidChangeLengthList(AttrEnum(), emptyOrOldValue);
  if (mAList->IsAnimating()) {
    Element()->AnimationNeedsResample();
  }
  return domItem.forget();
}

NS_IMETHODIMP
DOMSVGLengthList::ReplaceItem(nsIDOMSVGLength *newItem,
                              uint32_t index,
                              nsIDOMSVGLength **_retval)
{
  ErrorResult rv;
  *_retval = ReplaceItem(newItem, index, rv).get();
  return rv.ErrorCode();
}

already_AddRefed<nsIDOMSVGLength>
DOMSVGLengthList::RemoveItem(uint32_t index,
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

  nsAttrValue emptyOrOldValue = Element()->WillChangeLengthList(AttrEnum());
  
  
  
  MaybeRemoveItemFromAnimValListAt(index);

  
  EnsureItemAt(index);

  
  
  mItems[index]->RemovingFromList();
  nsCOMPtr<nsIDOMSVGLength> result = mItems[index];

  InternalList().RemoveItem(index);
  mItems.RemoveElementAt(index);

  UpdateListIndicesFromIndex(mItems, index);

  Element()->DidChangeLengthList(AttrEnum(), emptyOrOldValue);
  if (mAList->IsAnimating()) {
    Element()->AnimationNeedsResample();
  }
  return result.forget();
}

NS_IMETHODIMP
DOMSVGLengthList::RemoveItem(uint32_t index,
                             nsIDOMSVGLength **_retval)
{
  ErrorResult rv;
  *_retval = RemoveItem(index, rv).get();
  return rv.ErrorCode();
}

NS_IMETHODIMP
DOMSVGLengthList::AppendItem(nsIDOMSVGLength *newItem,
                             nsIDOMSVGLength **_retval)
{
  ErrorResult rv;
  *_retval = AppendItem(newItem, rv).get();
  return rv.ErrorCode();
}

NS_IMETHODIMP
DOMSVGLengthList::GetLength(uint32_t *aLength)
{
  *aLength = Length();
  return NS_OK;
}

void
DOMSVGLengthList::EnsureItemAt(uint32_t aIndex)
{
  if (!mItems[aIndex]) {
    mItems[aIndex] = new DOMSVGLength(this, AttrEnum(), aIndex, IsAnimValList());
  }
}

void
DOMSVGLengthList::MaybeInsertNullInAnimValListAt(uint32_t aIndex)
{
  NS_ABORT_IF_FALSE(!IsAnimValList(), "call from baseVal to animVal");

  DOMSVGLengthList* animVal = mAList->mAnimVal;

  if (!animVal || mAList->IsAnimating()) {
    
    return;
  }

  NS_ABORT_IF_FALSE(animVal->mItems.Length() == mItems.Length(),
                    "animVal list not in sync!");

  animVal->mItems.InsertElementAt(aIndex, static_cast<DOMSVGLength*>(nullptr));

  UpdateListIndicesFromIndex(animVal->mItems, aIndex + 1);
}

void
DOMSVGLengthList::MaybeRemoveItemFromAnimValListAt(uint32_t aIndex)
{
  NS_ABORT_IF_FALSE(!IsAnimValList(), "call from baseVal to animVal");

  
  
  nsRefPtr<DOMSVGLengthList> animVal = mAList->mAnimVal;

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
