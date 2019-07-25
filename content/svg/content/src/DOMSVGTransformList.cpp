






































#include "DOMSVGTransformList.h"
#include "DOMSVGTransform.h"
#include "DOMSVGMatrix.h"
#include "SVGAnimatedTransformList.h"
#include "nsSVGElement.h"


namespace {

void UpdateListIndicesFromIndex(
  nsTArray<mozilla::DOMSVGTransform*>& aItemsArray,
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

namespace mozilla {





NS_IMPL_CYCLE_COLLECTION_CLASS(DOMSVGTransformList)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(DOMSVGTransformList)
  
  ( tmp->IsAnimValList() ? tmp->mAList->mAnimVal : tmp->mAList->mBaseVal ) =
    nsnull;
NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mAList)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(DOMSVGTransformList)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mAList)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(DOMSVGTransformList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DOMSVGTransformList)

} 
DOMCI_DATA(SVGTransformList, mozilla::DOMSVGTransformList)
namespace mozilla {

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMSVGTransformList)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGTransformList)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGTransformList)
NS_INTERFACE_MAP_END




nsIDOMSVGTransform*
DOMSVGTransformList::GetItemWithoutAddRef(PRUint32 aIndex)
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
DOMSVGTransformList::InternalListLengthWillChange(PRUint32 aNewLength)
{
  PRUint32 oldLength = mItems.Length();

  if (aNewLength > DOMSVGTransform::MaxListIndex()) {
    
    
    aNewLength = DOMSVGTransform::MaxListIndex();
  }

  nsRefPtr<DOMSVGTransformList> kungFuDeathGrip;
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

SVGTransformList&
DOMSVGTransformList::InternalList() const
{
  SVGAnimatedTransformList *alist = Element()->GetAnimatedTransformList();
  return IsAnimValList() && alist->mAnimVal ?
    *alist->mAnimVal :
    alist->mBaseVal;
}





NS_IMETHODIMP
DOMSVGTransformList::GetNumberOfItems(PRUint32 *aNumberOfItems)
{
  if (IsAnimValList()) {
    Element()->FlushAnimations();
  }
  *aNumberOfItems = Length();
  return NS_OK;
}


NS_IMETHODIMP
DOMSVGTransformList::GetLength(PRUint32 *aLength)
{
  return GetNumberOfItems(aLength);
}


NS_IMETHODIMP
DOMSVGTransformList::Clear()
{
  if (IsAnimValList()) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  if (Length() > 0) {
    
    
    
    mAList->InternalBaseValListWillChangeLengthTo(0);

    mItems.Clear();
    InternalList().Clear();
    Element()->DidChangeTransformList(true);
    if (mAList->IsAnimating()) {
      Element()->AnimationNeedsResample();
    }
  }
  return NS_OK;
}


NS_IMETHODIMP
DOMSVGTransformList::Initialize(nsIDOMSVGTransform *newItem,
                                nsIDOMSVGTransform **_retval)
{
  *_retval = nsnull;
  if (IsAnimValList()) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  
  
  
  
  
  
  

  nsCOMPtr<DOMSVGTransform> domItem = do_QueryInterface(newItem);
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
DOMSVGTransformList::GetItem(PRUint32 index, nsIDOMSVGTransform **_retval)
{
  *_retval = GetItemWithoutAddRef(index);
  if (!*_retval) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }
  NS_ADDREF(*_retval);
  return NS_OK;
}



NS_IMETHODIMP
DOMSVGTransformList::InsertItemBefore(nsIDOMSVGTransform *newItem,
                                      PRUint32 index,
                                      nsIDOMSVGTransform **_retval)
{
  *_retval = nsnull;
  if (IsAnimValList()) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  index = NS_MIN(index, Length());
  if (index >= DOMSVGTransform::MaxListIndex()) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  nsCOMPtr<DOMSVGTransform> domItem = do_QueryInterface(newItem);
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

  
  MaybeInsertNullInAnimValListAt(index);

  InternalList().InsertItem(index, domItem->ToSVGTransform());
  mItems.InsertElementAt(index, domItem.get());

  
  
  
  domItem->InsertingIntoList(this, index, IsAnimValList());

  UpdateListIndicesFromIndex(mItems, index + 1);

  Element()->DidChangeTransformList(true);
  if (mAList->IsAnimating()) {
    Element()->AnimationNeedsResample();
  }
  *_retval = domItem.forget().get();
  return NS_OK;
}



NS_IMETHODIMP
DOMSVGTransformList::ReplaceItem(nsIDOMSVGTransform *newItem,
                                 PRUint32 index,
                                 nsIDOMSVGTransform **_retval)
{
  *_retval = nsnull;
  if (IsAnimValList()) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  nsCOMPtr<DOMSVGTransform> domItem = do_QueryInterface(newItem);
  if (!domItem) {
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;
  }
  if (index >= Length()) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }
  if (domItem->HasOwner()) {
    domItem = domItem->Clone(); 
  }

  if (mItems[index]) {
    
    
    mItems[index]->RemovingFromList();
  }

  InternalList()[index] = domItem->ToSVGTransform();
  mItems[index] = domItem;

  
  
  domItem->InsertingIntoList(this, index, IsAnimValList());

  Element()->DidChangeTransformList(true);
  if (mAList->IsAnimating()) {
    Element()->AnimationNeedsResample();
  }
  NS_ADDREF(*_retval = domItem.get());
  return NS_OK;
}


NS_IMETHODIMP
DOMSVGTransformList::RemoveItem(PRUint32 index, nsIDOMSVGTransform **_retval)
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

  
  
  mItems[index]->RemovingFromList();
  NS_ADDREF(*_retval = mItems[index]);

  InternalList().RemoveItem(index);
  mItems.RemoveElementAt(index);

  UpdateListIndicesFromIndex(mItems, index);

  Element()->DidChangeTransformList(true);
  if (mAList->IsAnimating()) {
    Element()->AnimationNeedsResample();
  }
  return NS_OK;
}


NS_IMETHODIMP
DOMSVGTransformList::AppendItem(nsIDOMSVGTransform *newItem,
                                nsIDOMSVGTransform **_retval)
{
  return InsertItemBefore(newItem, Length(), _retval);
}



NS_IMETHODIMP
DOMSVGTransformList::CreateSVGTransformFromMatrix(nsIDOMSVGMatrix *matrix,
                                                  nsIDOMSVGTransform **_retval)
{
  nsCOMPtr<DOMSVGMatrix> domItem = do_QueryInterface(matrix);
  if (!domItem) {
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;
  }

  NS_ADDREF(*_retval = new DOMSVGTransform(domItem->Matrix()));
  return NS_OK;
}


NS_IMETHODIMP
DOMSVGTransformList::Consolidate(nsIDOMSVGTransform **_retval)
{
  *_retval = nsnull;
  if (IsAnimValList()) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  if (Length() == 0)
    return NS_OK;

  
  
  
  

  
  gfxMatrix mx = InternalList().GetConsolidationMatrix();

  
  Clear();

  
  nsRefPtr<DOMSVGTransform> transform = new DOMSVGTransform(mx);
  return InsertItemBefore(transform, Length(), _retval);
}




void
DOMSVGTransformList::EnsureItemAt(PRUint32 aIndex)
{
  if (!mItems[aIndex]) {
    mItems[aIndex] = new DOMSVGTransform(this, aIndex, IsAnimValList());
  }
}

void
DOMSVGTransformList::MaybeInsertNullInAnimValListAt(PRUint32 aIndex)
{
  NS_ABORT_IF_FALSE(!IsAnimValList(), "call from baseVal to animVal");

  DOMSVGTransformList* animVal = mAList->mAnimVal;

  if (!animVal || mAList->IsAnimating()) {
    
    return;
  }

  NS_ABORT_IF_FALSE(animVal->mItems.Length() == mItems.Length(),
                    "animVal list not in sync!");

  animVal->mItems.InsertElementAt(aIndex,
                                  static_cast<DOMSVGTransform*>(nsnull));

  UpdateListIndicesFromIndex(animVal->mItems, aIndex + 1);
}

void
DOMSVGTransformList::MaybeRemoveItemFromAnimValListAt(PRUint32 aIndex)
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
