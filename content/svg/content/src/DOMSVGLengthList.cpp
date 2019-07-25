



































#include "nsSVGElement.h"
#include "DOMSVGLengthList.h"
#include "DOMSVGLength.h"
#include "nsDOMError.h"
#include "SVGAnimatedLengthList.h"
#include "nsCOMPtr.h"



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


void
DOMSVGLengthList::InternalListLengthWillChange(PRUint32 aNewLength)
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
DOMSVGLengthList::InsertItemBefore(nsIDOMSVGLength *newItem,
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
  index = NS_MIN(index, Length());
  SVGLength length = domItem->ToSVGLength(); 
  if (domItem->HasOwner()) {
    domItem = new DOMSVGLength();
  }
  PRBool ok = !!InternalList().InsertItem(index, length);
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
  SVGLength length = domItem->ToSVGLength(); 
  if (domItem->HasOwner()) {
    domItem = new DOMSVGLength();
  }
  if (mItems[index]) {
    
    
    mItems[index]->RemovingFromList();
  }
  InternalList()[index] = length;
  domItem->InsertingIntoList(this, AttrEnum(), index, IsAnimValList());
  mItems[index] = domItem;

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

void
DOMSVGLengthList::EnsureItemAt(PRUint32 aIndex)
{
  if (!mItems[aIndex]) {
    mItems[aIndex] = new DOMSVGLength(this, AttrEnum(), aIndex, IsAnimValList());
  }
}

} 
