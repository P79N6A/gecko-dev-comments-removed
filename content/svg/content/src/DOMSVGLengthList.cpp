



































#include "nsSVGElement.h"
#include "DOMSVGLengthList.h"
#include "DOMSVGLength.h"
#include "nsDOMError.h"
#include "SVGAnimatedLengthList.h"
#include "nsCOMPtr.h"




namespace {

using mozilla::DOMSVGLength;

void UpdateListIndicesFromIndex(nsTArray<DOMSVGLength*>& aItemsArray,
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





NS_IMPL_CYCLE_COLLECTION_CLASS(DOMSVGLengthList)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(DOMSVGLengthList)
  
  ( tmp->IsAnimValList() ? tmp->mAList->mAnimVal : tmp->mAList->mBaseVal ) = nsnull;
NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mAList)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(DOMSVGLengthList)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mAList)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(DOMSVGLengthList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DOMSVGLengthList)

} 
DOMCI_DATA(SVGLengthList, mozilla::DOMSVGLengthList)
namespace mozilla {

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMSVGLengthList)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGLengthList)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGLengthList)
NS_INTERFACE_MAP_END


nsIDOMSVGLength*
DOMSVGLengthList::GetItemWithoutAddRef(PRUint32 aIndex)
{
#ifdef MOZ_SMIL
  if (IsAnimValList()) {
    Element()->FlushAnimations();
  }
#endif
  if (aIndex < Length()) {
    EnsureItemAt(aIndex);
    return mItems[aIndex];
  }
  return nsnull;
}

void
DOMSVGLengthList::InternalListLengthWillChange(PRUint32 aNewLength)
{
  PRUint32 oldLength = mItems.Length();

  if (aNewLength > DOMSVGLength::MaxListIndex()) {
    
    
    aNewLength = DOMSVGLength::MaxListIndex();
  }

  nsRefPtr<DOMSVGLengthList> kungFuDeathGrip;
  if (oldLength && !aNewLength) {
    
    
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

SVGLengthList&
DOMSVGLengthList::InternalList()
{
  SVGAnimatedLengthList *alist = Element()->GetAnimatedLengthList(AttrEnum());
  return IsAnimValList() && alist->mAnimVal ? *alist->mAnimVal : alist->mBaseVal;
}




NS_IMETHODIMP
DOMSVGLengthList::GetNumberOfItems(PRUint32 *aNumberOfItems)
{
#ifdef MOZ_SMIL
  if (IsAnimValList()) {
    Element()->FlushAnimations();
  }
#endif
  *aNumberOfItems = Length();
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGLengthList::Clear()
{
  if (IsAnimValList()) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  if (Length() > 0) {
    
    
    
    mAList->InternalBaseValListWillChangeTo(SVGLengthList());

    mItems.Clear();
    InternalList().Clear();
    Element()->DidChangeLengthList(AttrEnum(), PR_TRUE);
#ifdef MOZ_SMIL
    if (mAList->IsAnimating()) {
      Element()->AnimationNeedsResample();
    }
#endif
  }
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGLengthList::Initialize(nsIDOMSVGLength *newItem,
                             nsIDOMSVGLength **_retval)
{
  *_retval = nsnull;
  if (IsAnimValList()) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  
  
  
  
  
  
  

  nsCOMPtr<DOMSVGLength> domItem = do_QueryInterface(newItem);
  if (!domItem) {
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;
  }
  if (domItem->HasOwner()) {
    newItem = domItem->Copy();
  }

  Clear();
  return InsertItemBefore(newItem, 0, _retval);
}

NS_IMETHODIMP
DOMSVGLengthList::GetItem(PRUint32 index,
                          nsIDOMSVGLength **_retval)
{
  *_retval = GetItemWithoutAddRef(index);
  if (!*_retval) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }
  NS_ADDREF(*_retval);
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGLengthList::InsertItemBefore(nsIDOMSVGLength *newItem,
                                   PRUint32 index,
                                   nsIDOMSVGLength **_retval)
{
  *_retval = nsnull;
  if (IsAnimValList()) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  index = NS_MIN(index, Length());
  if (index >= DOMSVGLength::MaxListIndex()) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  nsCOMPtr<DOMSVGLength> domItem = do_QueryInterface(newItem);
  if (!domItem) {
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;
  }
  if (domItem->HasOwner()) {
    domItem = domItem->Copy(); 
  }

  
  if (!mItems.SetCapacity(mItems.Length() + 1) ||
      !InternalList().SetCapacity(InternalList().Length() + 1)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  MaybeInsertNullInAnimValListAt(index);

  InternalList().InsertItem(index, domItem->ToSVGLength());
  mItems.InsertElementAt(index, domItem.get());

  
  
  
  domItem->InsertingIntoList(this, AttrEnum(), index, IsAnimValList());

  UpdateListIndicesFromIndex(mItems, index + 1);

  Element()->DidChangeLengthList(AttrEnum(), PR_TRUE);
#ifdef MOZ_SMIL
  if (mAList->IsAnimating()) {
    Element()->AnimationNeedsResample();
  }
#endif
  *_retval = domItem.forget().get();
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGLengthList::ReplaceItem(nsIDOMSVGLength *newItem,
                              PRUint32 index,
                              nsIDOMSVGLength **_retval)
{
  *_retval = nsnull;
  if (IsAnimValList()) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  nsCOMPtr<DOMSVGLength> domItem = do_QueryInterface(newItem);
  if (!domItem) {
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;
  }
  if (index >= Length()) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }
  if (domItem->HasOwner()) {
    domItem = domItem->Copy(); 
  }

  if (mItems[index]) {
    
    
    mItems[index]->RemovingFromList();
  }

  InternalList()[index] = domItem->ToSVGLength();
  mItems[index] = domItem;

  
  
  domItem->InsertingIntoList(this, AttrEnum(), index, IsAnimValList());

  Element()->DidChangeLengthList(AttrEnum(), PR_TRUE);
#ifdef MOZ_SMIL
  if (mAList->IsAnimating()) {
    Element()->AnimationNeedsResample();
  }
#endif
  NS_ADDREF(*_retval = domItem.get());
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGLengthList::RemoveItem(PRUint32 index,
                             nsIDOMSVGLength **_retval)
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

  Element()->DidChangeLengthList(AttrEnum(), PR_TRUE);
#ifdef MOZ_SMIL
  if (mAList->IsAnimating()) {
    Element()->AnimationNeedsResample();
  }
#endif
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGLengthList::AppendItem(nsIDOMSVGLength *newItem,
                             nsIDOMSVGLength **_retval)
{
  return InsertItemBefore(newItem, Length(), _retval);
}

NS_IMETHODIMP
DOMSVGLengthList::GetLength(PRUint32 *aNumberOfItems)
{
  return GetNumberOfItems(aNumberOfItems);
}

void
DOMSVGLengthList::EnsureItemAt(PRUint32 aIndex)
{
  if (!mItems[aIndex]) {
    mItems[aIndex] = new DOMSVGLength(this, AttrEnum(), aIndex, IsAnimValList());
  }
}

void
DOMSVGLengthList::MaybeInsertNullInAnimValListAt(PRUint32 aIndex)
{
  NS_ABORT_IF_FALSE(!IsAnimValList(), "call from baseVal to animVal");

  DOMSVGLengthList* animVal = mAList->mAnimVal;

  if (!animVal || mAList->IsAnimating()) {
    
    return;
  }

  NS_ABORT_IF_FALSE(animVal->mItems.Length() == mItems.Length(),
                    "animVal list not in sync!");

  animVal->mItems.InsertElementAt(aIndex, static_cast<DOMSVGLength*>(nsnull));

  UpdateListIndicesFromIndex(animVal->mItems, aIndex + 1);
}

void
DOMSVGLengthList::MaybeRemoveItemFromAnimValListAt(PRUint32 aIndex)
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
