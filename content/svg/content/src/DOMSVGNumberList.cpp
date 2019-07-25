




#include "nsSVGElement.h"
#include "DOMSVGNumberList.h"
#include "DOMSVGNumber.h"
#include "nsDOMError.h"
#include "SVGAnimatedNumberList.h"
#include "nsCOMPtr.h"
#include "nsContentUtils.h"
#include "dombindings.h"



namespace mozilla {


namespace {

using mozilla::DOMSVGNumber;

void UpdateListIndicesFromIndex(nsTArray<DOMSVGNumber*>& aItemsArray,
                                PRUint32 aStartingIndex)
{
  PRUint32 length = aItemsArray.Length();

  for (PRUint32 i = aStartingIndex; i < length; ++i) {
    if (aItemsArray[i]) {
      aItemsArray[i]->UpdateListIndex(i);
    }
  }
}

} 





NS_IMPL_CYCLE_COLLECTION_CLASS(DOMSVGNumberList)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(DOMSVGNumberList)
  
  ( tmp->IsAnimValList() ? tmp->mAList->mAnimVal : tmp->mAList->mBaseVal ) = nsnull;
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mAList)
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
  return mozilla::dom::binding::SVGNumberList::create(cx, scope, this,
                                                      triedToWrap);
}

nsIDOMSVGNumber*
DOMSVGNumberList::GetItemAt(PRUint32 aIndex)
{
  if (IsAnimValList()) {
    Element()->FlushAnimations();
  }
  if (aIndex < Length()) {
    EnsureItemAt(aIndex);
    return mItems[aIndex];
  }
  return nsnull;
}

void
DOMSVGNumberList::InternalListLengthWillChange(PRUint32 aNewLength)
{
  PRUint32 oldLength = mItems.Length();

  if (aNewLength > DOMSVGNumber::MaxListIndex()) {
    
    
    aNewLength = DOMSVGNumber::MaxListIndex();
  }

  nsRefPtr<DOMSVGNumberList> kungFuDeathGrip;
  if (aNewLength < oldLength) {
    
    
    kungFuDeathGrip = this;
  }

  
  for (PRUint32 i = aNewLength; i < oldLength; ++i) {
    if (mItems[i]) {
      mItems[i]->RemovingFromList();
    }
  }

  if (!mItems.SetLength(aNewLength)) {
    
    
    mItems.Clear();
    return;
  }

  
  for (PRUint32 i = oldLength; i < aNewLength; ++i) {
    mItems[i] = nsnull;
  }
}

SVGNumberList&
DOMSVGNumberList::InternalList() const
{
  SVGAnimatedNumberList *alist = Element()->GetAnimatedNumberList(AttrEnum());
  return IsAnimValList() && alist->mAnimVal ? *alist->mAnimVal : alist->mBaseVal;
}




NS_IMETHODIMP
DOMSVGNumberList::GetNumberOfItems(PRUint32 *aNumberOfItems)
{
  if (IsAnimValList()) {
    Element()->FlushAnimations();
  }
  *aNumberOfItems = Length();
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGNumberList::Clear()
{
  if (IsAnimValList()) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  if (Length() > 0) {
    nsAttrValue emptyOrOldValue = Element()->WillChangeNumberList(AttrEnum());
    
    
    
    mAList->InternalBaseValListWillChangeTo(SVGNumberList());

    mItems.Clear();
    InternalList().Clear();
    Element()->DidChangeNumberList(AttrEnum(), emptyOrOldValue);
    if (mAList->IsAnimating()) {
      Element()->AnimationNeedsResample();
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGNumberList::Initialize(nsIDOMSVGNumber *newItem,
                             nsIDOMSVGNumber **_retval)
{
  *_retval = nsnull;
  if (IsAnimValList()) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  
  
  
  
  
  
  

  nsCOMPtr<DOMSVGNumber> domItem = do_QueryInterface(newItem);
  if (!domItem) {
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;
  }
  if (domItem->HasOwner()) {
    newItem = domItem->Clone();
  }

  Clear();
  return InsertItemBefore(newItem, 0, _retval);
}

NS_IMETHODIMP
DOMSVGNumberList::GetItem(PRUint32 index,
                          nsIDOMSVGNumber **_retval)
{
  *_retval = GetItemAt(index);
  if (!*_retval) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }
  NS_ADDREF(*_retval);
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGNumberList::InsertItemBefore(nsIDOMSVGNumber *newItem,
                                   PRUint32 index,
                                   nsIDOMSVGNumber **_retval)
{
  *_retval = nsnull;
  if (IsAnimValList()) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  index = NS_MIN(index, Length());
  if (index >= DOMSVGNumber::MaxListIndex()) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  nsCOMPtr<DOMSVGNumber> domItem = do_QueryInterface(newItem);
  if (!domItem) {
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;
  }
  if (domItem->HasOwner()) {
    domItem = domItem->Clone(); 
  }

  
  if (!mItems.SetCapacity(mItems.Length() + 1) ||
      !InternalList().SetCapacity(InternalList().Length() + 1)) {
    return NS_ERROR_OUT_OF_MEMORY;
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
  *_retval = domItem.forget().get();
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGNumberList::ReplaceItem(nsIDOMSVGNumber *newItem,
                              PRUint32 index,
                              nsIDOMSVGNumber **_retval)
{
  *_retval = nsnull;
  if (IsAnimValList()) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  nsCOMPtr<DOMSVGNumber> domItem = do_QueryInterface(newItem);
  if (!domItem) {
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;
  }
  if (index >= Length()) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
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
  NS_ADDREF(*_retval = domItem.get());
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGNumberList::RemoveItem(PRUint32 index,
                             nsIDOMSVGNumber **_retval)
{
  *_retval = nsnull;
  if (IsAnimValList()) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  if (index >= Length()) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  
  
  
  MaybeRemoveItemFromAnimValListAt(index);

  
  EnsureItemAt(index);

  nsAttrValue emptyOrOldValue = Element()->WillChangeNumberList(AttrEnum());
  
  
  mItems[index]->RemovingFromList();
  NS_ADDREF(*_retval = mItems[index]);

  InternalList().RemoveItem(index);
  mItems.RemoveElementAt(index);

  UpdateListIndicesFromIndex(mItems, index);

  Element()->DidChangeNumberList(AttrEnum(), emptyOrOldValue);
  if (mAList->IsAnimating()) {
    Element()->AnimationNeedsResample();
  }
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGNumberList::AppendItem(nsIDOMSVGNumber *newItem,
                             nsIDOMSVGNumber **_retval)
{
  return InsertItemBefore(newItem, Length(), _retval);
}

NS_IMETHODIMP
DOMSVGNumberList::GetLength(PRUint32 *aNumberOfItems)
{
  return GetNumberOfItems(aNumberOfItems);
}

void
DOMSVGNumberList::EnsureItemAt(PRUint32 aIndex)
{
  if (!mItems[aIndex]) {
    mItems[aIndex] = new DOMSVGNumber(this, AttrEnum(), aIndex, IsAnimValList());
  }
}

void
DOMSVGNumberList::MaybeInsertNullInAnimValListAt(PRUint32 aIndex)
{
  NS_ABORT_IF_FALSE(!IsAnimValList(), "call from baseVal to animVal");

  DOMSVGNumberList* animVal = mAList->mAnimVal;

  if (!animVal || mAList->IsAnimating()) {
    
    return;
  }

  NS_ABORT_IF_FALSE(animVal->mItems.Length() == mItems.Length(),
                    "animVal list not in sync!");

  animVal->mItems.InsertElementAt(aIndex, static_cast<DOMSVGNumber*>(nsnull));

  UpdateListIndicesFromIndex(animVal->mItems, aIndex + 1);
}

void
DOMSVGNumberList::MaybeRemoveItemFromAnimValListAt(PRUint32 aIndex)
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
