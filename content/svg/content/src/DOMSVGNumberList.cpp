



































#include "nsSVGElement.h"
#include "DOMSVGNumberList.h"
#include "DOMSVGNumber.h"
#include "nsDOMError.h"
#include "SVGAnimatedNumberList.h"
#include "nsCOMPtr.h"



using namespace mozilla;





NS_IMPL_CYCLE_COLLECTION_CLASS(DOMSVGNumberList)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(DOMSVGNumberList)
  
  ( tmp->IsAnimValList() ? tmp->mAList->mAnimVal : tmp->mAList->mBaseVal ) = nsnull;
NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mAList)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(DOMSVGNumberList)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mAList)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(DOMSVGNumberList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DOMSVGNumberList)

DOMCI_DATA(SVGNumberList, DOMSVGNumberList)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMSVGNumberList)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGNumberList)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGNumberList)
NS_INTERFACE_MAP_END


void
DOMSVGNumberList::InternalListLengthWillChange(PRUint32 aNewLength)
{
  PRUint32 oldLength = mItems.Length();

  
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
DOMSVGNumberList::InternalList()
{
  SVGAnimatedNumberList *alist = Element()->GetAnimatedNumberList(AttrEnum());
  return IsAnimValList() && alist->mAnimVal ? *alist->mAnimVal : alist->mBaseVal;
}




NS_IMETHODIMP
DOMSVGNumberList::GetNumberOfItems(PRUint32 *aNumberOfItems)
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
DOMSVGNumberList::Clear()
{
  if (IsAnimValList()) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  if (Length() > 0) {
    
    
    
    mAList->InternalBaseValListWillChangeTo(SVGNumberList());

    mItems.Clear();
    InternalList().Clear();
    Element()->DidChangeNumberList(AttrEnum(), PR_TRUE);
#ifdef MOZ_SMIL
    if (mAList->IsAnimating()) {
      Element()->AnimationNeedsResample();
    }
#endif
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
#ifdef MOZ_SMIL
  if (IsAnimValList()) {
    Element()->FlushAnimations();
  }
#endif
  if (index < Length()) {
    EnsureItemAt(index);
    NS_ADDREF(*_retval = mItems[index]);
    return NS_OK;
  }
  *_retval = nsnull;
  return NS_ERROR_DOM_INDEX_SIZE_ERR;
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

  nsCOMPtr<DOMSVGNumber> domItem = do_QueryInterface(newItem);
  if (!domItem) {
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;
  }
  index = NS_MIN(index, Length());
  float value = domItem->ToSVGNumber(); 
  if (domItem->HasOwner()) {
    domItem = new DOMSVGNumber();
  }
  PRBool ok = !!InternalList().InsertItem(index, value);
  if (!ok) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  domItem->InsertingIntoList(this, AttrEnum(), index, IsAnimValList());
  ok = !!mItems.InsertElementAt(index, domItem.get());
  if (!ok) {
    InternalList().RemoveItem(index);
    return NS_ERROR_OUT_OF_MEMORY;
  }
  for (PRUint32 i = index + 1; i < Length(); ++i) {
    if (mItems[i]) {
      mItems[i]->UpdateListIndex(i);
    }
  }
  Element()->DidChangeNumberList(AttrEnum(), PR_TRUE);
#ifdef MOZ_SMIL
  if (mAList->IsAnimating()) {
    Element()->AnimationNeedsResample();
  }
#endif
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
  float length = domItem->ToSVGNumber(); 
  if (domItem->HasOwner()) {
    domItem = new DOMSVGNumber();
  }
  if (mItems[index]) {
    
    
    mItems[index]->RemovingFromList();
  }
  InternalList()[index] = length;
  domItem->InsertingIntoList(this, AttrEnum(), index, IsAnimValList());
  mItems[index] = domItem;

  Element()->DidChangeNumberList(AttrEnum(), PR_TRUE);
#ifdef MOZ_SMIL
  if (mAList->IsAnimating()) {
    Element()->AnimationNeedsResample();
  }
#endif
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
  
  EnsureItemAt(index);

  
  
  mItems[index]->RemovingFromList();

  InternalList().RemoveItem(index);

  NS_ADDREF(*_retval = mItems[index]);
  mItems.RemoveElementAt(index);
  for (PRUint32 i = index; i < Length(); ++i) {
    if (mItems[i]) {
      mItems[i]->UpdateListIndex(i);
    }
  }
  Element()->DidChangeNumberList(AttrEnum(), PR_TRUE);
#ifdef MOZ_SMIL
  if (mAList->IsAnimating()) {
    Element()->AnimationNeedsResample();
  }
#endif
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGNumberList::AppendItem(nsIDOMSVGNumber *newItem,
                             nsIDOMSVGNumber **_retval)
{
  return InsertItemBefore(newItem, Length(), _retval);
}

void
DOMSVGNumberList::EnsureItemAt(PRUint32 aIndex)
{
  if (!mItems[aIndex]) {
    mItems[aIndex] = new DOMSVGNumber(this, AttrEnum(), aIndex, IsAnimValList());
  }
}
