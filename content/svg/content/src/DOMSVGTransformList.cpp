




#include "DOMSVGTransformList.h"
#include "DOMSVGTransform.h"
#include "DOMSVGMatrix.h"
#include "SVGAnimatedTransformList.h"
#include "nsSVGElement.h"
#include "nsContentUtils.h"
#include "mozilla/dom/SVGTransformListBinding.h"
#include "nsError.h"


namespace {

void UpdateListIndicesFromIndex(
  nsTArray<mozilla::DOMSVGTransform*>& aItemsArray,
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





NS_IMPL_CYCLE_COLLECTION_CLASS(DOMSVGTransformList)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(DOMSVGTransformList)
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
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(DOMSVGTransformList)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mAList)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(DOMSVGTransformList)
  NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(DOMSVGTransformList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DOMSVGTransformList)

} 
DOMCI_DATA(SVGTransformList, mozilla::DOMSVGTransformList)
namespace mozilla {

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMSVGTransformList)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGTransformList)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGTransformList)
NS_INTERFACE_MAP_END




JSObject*
DOMSVGTransformList::WrapObject(JSContext *cx, JSObject *scope,
                                bool *triedToWrap)
{
  return mozilla::dom::SVGTransformListBinding::Wrap(cx, scope, this,
                                                     triedToWrap);
}

void
DOMSVGTransformList::InternalListLengthWillChange(uint32_t aNewLength)
{
  uint32_t oldLength = mItems.Length();

  if (aNewLength > DOMSVGTransform::MaxListIndex()) {
    
    
    aNewLength = DOMSVGTransform::MaxListIndex();
  }

  nsRefPtr<DOMSVGTransformList> kungFuDeathGrip;
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

SVGTransformList&
DOMSVGTransformList::InternalList() const
{
  SVGAnimatedTransformList *alist = Element()->GetAnimatedTransformList();
  return IsAnimValList() && alist->mAnimVal ?
    *alist->mAnimVal :
    alist->mBaseVal;
}





NS_IMETHODIMP
DOMSVGTransformList::GetNumberOfItems(uint32_t *aNumberOfItems)
{
  *aNumberOfItems = NumberOfItems();
  return NS_OK;
}


NS_IMETHODIMP
DOMSVGTransformList::GetLength(uint32_t *aLength)
{
  *aLength = Length();
  return NS_OK;
}

void
DOMSVGTransformList::Clear(ErrorResult& error)
{
  if (IsAnimValList()) {
    error.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  if (LengthNoFlush() > 0) {
    nsAttrValue emptyOrOldValue = Element()->WillChangeTransformList();
    
    
    
    mAList->InternalBaseValListWillChangeLengthTo(0);

    mItems.Clear();
    InternalList().Clear();
    Element()->DidChangeTransformList(emptyOrOldValue);
    if (mAList->IsAnimating()) {
      Element()->AnimationNeedsResample();
    }
  }
}


NS_IMETHODIMP
DOMSVGTransformList::Clear()
{
  ErrorResult rv;
  Clear(rv);
  return rv.ErrorCode();
}

already_AddRefed<nsIDOMSVGTransform>
DOMSVGTransformList::Initialize(nsIDOMSVGTransform *newItem, ErrorResult& error)
{
  if (IsAnimValList()) {
    error.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return nullptr;
  }

  
  
  
  
  
  
  

  nsCOMPtr<DOMSVGTransform> domItem = do_QueryInterface(newItem);
  if (!domItem) {
    error.Throw(NS_ERROR_DOM_SVG_WRONG_TYPE_ERR);
    return nullptr;
  }
  if (domItem->HasOwner()) {
    newItem = domItem->Clone();
  }

  Clear(error);
  MOZ_ASSERT(!error.Failed(), "How could this fail?");
  return InsertItemBefore(newItem, 0, error);
}


NS_IMETHODIMP
DOMSVGTransformList::Initialize(nsIDOMSVGTransform *newItem,
                                nsIDOMSVGTransform **_retval)
{
  ErrorResult rv;
  *_retval = Initialize(newItem, rv).get();
  return rv.ErrorCode();
}

nsIDOMSVGTransform*
DOMSVGTransformList::IndexedGetter(uint32_t index, bool& found,
                                   ErrorResult& error)
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
DOMSVGTransformList::GetItem(uint32_t index, nsIDOMSVGTransform **_retval)
{
  ErrorResult rv;
  NS_IF_ADDREF(*_retval = GetItem(index, rv));
  return rv.ErrorCode();
}

already_AddRefed<nsIDOMSVGTransform>
DOMSVGTransformList::InsertItemBefore(nsIDOMSVGTransform *newItem,
                                      uint32_t index, ErrorResult& error)
{
  if (IsAnimValList()) {
    error.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return nullptr;
  }

  index = NS_MIN(index, LengthNoFlush());
  if (index >= DOMSVGTransform::MaxListIndex()) {
    error.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return nullptr;
  }

  nsCOMPtr<DOMSVGTransform> domItem = do_QueryInterface(newItem);
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

  nsAttrValue emptyOrOldValue = Element()->WillChangeTransformList();
  
  MaybeInsertNullInAnimValListAt(index);

  InternalList().InsertItem(index, domItem->ToSVGTransform());
  mItems.InsertElementAt(index, domItem.get());

  
  
  
  domItem->InsertingIntoList(this, index, IsAnimValList());

  UpdateListIndicesFromIndex(mItems, index + 1);

  Element()->DidChangeTransformList(emptyOrOldValue);
  if (mAList->IsAnimating()) {
    Element()->AnimationNeedsResample();
  }
  return domItem.forget();
}



NS_IMETHODIMP
DOMSVGTransformList::InsertItemBefore(nsIDOMSVGTransform *newItem,
                                      uint32_t index,
                                      nsIDOMSVGTransform **_retval)
{
  ErrorResult rv;
  *_retval = InsertItemBefore(newItem, index, rv).get();
  return rv.ErrorCode();
}

already_AddRefed<nsIDOMSVGTransform>
DOMSVGTransformList::ReplaceItem(nsIDOMSVGTransform *newItem,
                                 uint32_t index, ErrorResult& error)
{
  if (IsAnimValList()) {
    error.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return nullptr;
  }

  nsCOMPtr<DOMSVGTransform> domItem = do_QueryInterface(newItem);
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

  nsAttrValue emptyOrOldValue = Element()->WillChangeTransformList();
  if (mItems[index]) {
    
    
    mItems[index]->RemovingFromList();
  }

  InternalList()[index] = domItem->ToSVGTransform();
  mItems[index] = domItem;

  
  
  domItem->InsertingIntoList(this, index, IsAnimValList());

  Element()->DidChangeTransformList(emptyOrOldValue);
  if (mAList->IsAnimating()) {
    Element()->AnimationNeedsResample();
  }
  return domItem.forget();
}



NS_IMETHODIMP
DOMSVGTransformList::ReplaceItem(nsIDOMSVGTransform *newItem,
                                 uint32_t index,
                                 nsIDOMSVGTransform **_retval)
{
  ErrorResult rv;
  *_retval = ReplaceItem(newItem, index, rv).get();
  return rv.ErrorCode();
}

already_AddRefed<nsIDOMSVGTransform>
DOMSVGTransformList::RemoveItem(uint32_t index, ErrorResult& error)
{
  if (IsAnimValList()) {
    error.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return nullptr;
  }

  if (index >= LengthNoFlush()) {
    error.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return nullptr;
  }

  nsAttrValue emptyOrOldValue = Element()->WillChangeTransformList();
  
  
  
  MaybeRemoveItemFromAnimValListAt(index);

  
  EnsureItemAt(index);

  
  
  mItems[index]->RemovingFromList();
  nsCOMPtr<nsIDOMSVGTransform> result = mItems[index];

  InternalList().RemoveItem(index);
  mItems.RemoveElementAt(index);

  UpdateListIndicesFromIndex(mItems, index);

  Element()->DidChangeTransformList(emptyOrOldValue);
  if (mAList->IsAnimating()) {
    Element()->AnimationNeedsResample();
  }
  return result.forget();
}


NS_IMETHODIMP
DOMSVGTransformList::RemoveItem(uint32_t index, nsIDOMSVGTransform **_retval)
{
  ErrorResult rv;
  *_retval = RemoveItem(index, rv).get();
  return rv.ErrorCode();
}


NS_IMETHODIMP
DOMSVGTransformList::AppendItem(nsIDOMSVGTransform *newItem,
                                nsIDOMSVGTransform **_retval)
{
  ErrorResult rv;
  *_retval = AppendItem(newItem, rv).get();
  return rv.ErrorCode();
}

already_AddRefed<nsIDOMSVGTransform>
DOMSVGTransformList::CreateSVGTransformFromMatrix(nsIDOMSVGMatrix *matrix,
                                                  ErrorResult& error)
{
  nsCOMPtr<DOMSVGMatrix> domItem = do_QueryInterface(matrix);
  if (!domItem) {
    error.Throw(NS_ERROR_DOM_SVG_WRONG_TYPE_ERR);
    return nullptr;
  }

  nsCOMPtr<nsIDOMSVGTransform> result = new DOMSVGTransform(domItem->Matrix());
  return result.forget();
}



NS_IMETHODIMP
DOMSVGTransformList::CreateSVGTransformFromMatrix(nsIDOMSVGMatrix *matrix,
                                                  nsIDOMSVGTransform **_retval)
{
  ErrorResult rv;
  *_retval = CreateSVGTransformFromMatrix(matrix, rv).get();
  return rv.ErrorCode();
}

already_AddRefed<nsIDOMSVGTransform>
DOMSVGTransformList::Consolidate(ErrorResult& error)
{
  if (IsAnimValList()) {
    error.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return nullptr;
  }

  if (LengthNoFlush() == 0)
    return nullptr;

  
  
  
  

  
  gfxMatrix mx = InternalList().GetConsolidationMatrix();

  
  Clear(error);
  MOZ_ASSERT(!error.Failed(), "How could this fail?");

  
  nsRefPtr<DOMSVGTransform> transform = new DOMSVGTransform(mx);
  return InsertItemBefore(transform, LengthNoFlush(), error);
}


NS_IMETHODIMP
DOMSVGTransformList::Consolidate(nsIDOMSVGTransform **_retval)
{
  ErrorResult rv;
  *_retval = Consolidate(rv).get();
  return rv.ErrorCode();
}




void
DOMSVGTransformList::EnsureItemAt(uint32_t aIndex)
{
  if (!mItems[aIndex]) {
    mItems[aIndex] = new DOMSVGTransform(this, aIndex, IsAnimValList());
  }
}

void
DOMSVGTransformList::MaybeInsertNullInAnimValListAt(uint32_t aIndex)
{
  NS_ABORT_IF_FALSE(!IsAnimValList(), "call from baseVal to animVal");

  DOMSVGTransformList* animVal = mAList->mAnimVal;

  if (!animVal || mAList->IsAnimating()) {
    
    return;
  }

  NS_ABORT_IF_FALSE(animVal->mItems.Length() == mItems.Length(),
                    "animVal list not in sync!");

  animVal->mItems.InsertElementAt(aIndex,
                                  static_cast<DOMSVGTransform*>(nullptr));

  UpdateListIndicesFromIndex(animVal->mItems, aIndex + 1);
}

void
DOMSVGTransformList::MaybeRemoveItemFromAnimValListAt(uint32_t aIndex)
{
  NS_ABORT_IF_FALSE(!IsAnimValList(), "call from baseVal to animVal");

  
  
  nsRefPtr<DOMSVGTransformList> animVal = mAList->mAnimVal;

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
