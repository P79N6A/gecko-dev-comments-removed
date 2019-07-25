



































#include "nsSVGElement.h"
#include "DOMSVGPointList.h"
#include "DOMSVGPoint.h"
#include "nsDOMError.h"
#include "SVGAnimatedPointList.h"
#include "nsCOMPtr.h"
#include "nsSVGAttrTearoffTable.h"



using namespace mozilla;

static nsSVGAttrTearoffTable<void, DOMSVGPointList>
  sSVGPointListTearoffTable;

NS_SVG_VAL_IMPL_CYCLE_COLLECTION(DOMSVGPointList, mElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(DOMSVGPointList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DOMSVGPointList)

DOMCI_DATA(SVGPointList, DOMSVGPointList)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMSVGPointList)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGPointList)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGPointList)
NS_INTERFACE_MAP_END


 already_AddRefed<DOMSVGPointList>
DOMSVGPointList::GetDOMWrapper(void *aList,
                               nsSVGElement *aElement,
                               PRBool aIsAnimValList)
{
  DOMSVGPointList *wrapper =
    sSVGPointListTearoffTable.GetTearoff(aList);
  if (!wrapper) {
    wrapper = new DOMSVGPointList(aElement, aIsAnimValList);
    sSVGPointListTearoffTable.AddTearoff(aList, wrapper);
  }
  NS_ADDREF(wrapper);
  return wrapper;
}

 DOMSVGPointList*
DOMSVGPointList::GetDOMWrapperIfExists(void *aList)
{
  return sSVGPointListTearoffTable.GetTearoff(aList);
}

DOMSVGPointList::~DOMSVGPointList()
{
  
  
  
  
  void *key = mIsAnimValList ?
    InternalAList().GetAnimValKey() :
    InternalAList().GetBaseValKey();
  sSVGPointListTearoffTable.RemoveTearoff(key);
}

void
DOMSVGPointList::InternalListWillChangeTo(const SVGPointList& aNewValue)
{
  
  
  

  PRUint32 oldLength = mItems.Length();
  PRUint32 newLength = aNewValue.Length();

  
  for (PRUint32 i = newLength; i < oldLength; ++i) {
    if (mItems[i]) {
      mItems[i]->RemovingFromList();
    }
  }

  if (!mItems.SetLength(newLength)) {
    
    
    mItems.Clear();
    return;
  }

  
  for (PRUint32 i = oldLength; i < newLength; ++i) {
    mItems[i] = nsnull;
  }
}

PRBool
DOMSVGPointList::AttrIsAnimating() const
{
  return const_cast<DOMSVGPointList*>(this)->InternalAList().IsAnimating();
}

SVGPointList&
DOMSVGPointList::InternalList()
{
  SVGAnimatedPointList *alist = mElement->GetAnimatedPointList();
  return mIsAnimValList && alist->IsAnimating() ? *alist->mAnimVal : alist->mBaseVal;
}

SVGAnimatedPointList&
DOMSVGPointList::InternalAList()
{
  NS_ABORT_IF_FALSE(mElement->GetAnimatedPointList(), "Internal error");
  return *mElement->GetAnimatedPointList();
}




NS_IMETHODIMP
DOMSVGPointList::GetNumberOfItems(PRUint32 *aNumberOfItems)
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
DOMSVGPointList::Clear()
{
  if (IsAnimValList()) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  if (Length() > 0) {
    
    
    

    InternalListWillChangeTo(SVGPointList()); 

    if (!AttrIsAnimating()) {
      
      DOMSVGPointList *animList =
        GetDOMWrapperIfExists(InternalAList().GetAnimValKey());
      if (animList) {
        animList->InternalListWillChangeTo(SVGPointList()); 
      }
    }

    InternalList().Clear();
    Element()->DidChangePointList(PR_TRUE);
#ifdef MOZ_SMIL
    if (AttrIsAnimating()) {
      Element()->AnimationNeedsResample();
    }
#endif
  }
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGPointList::Initialize(nsIDOMSVGPoint *aNewItem,
                            nsIDOMSVGPoint **_retval)
{
  *_retval = nsnull;
  if (IsAnimValList()) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  
  
  
  
  
  
  

  nsCOMPtr<DOMSVGPoint> domItem = do_QueryInterface(aNewItem);
  if (!domItem) {
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;
  }
  if (domItem->HasOwner() || domItem->IsReadonly()) {
    aNewItem = domItem->Clone();
  }

  Clear();
  return InsertItemBefore(aNewItem, 0, _retval);
}

NS_IMETHODIMP
DOMSVGPointList::GetItem(PRUint32 aIndex,
                         nsIDOMSVGPoint **_retval)
{
#ifdef MOZ_SMIL
  if (IsAnimValList()) {
    Element()->FlushAnimations();
  }
#endif
  if (aIndex < Length()) {
    EnsureItemAt(aIndex);
    NS_ADDREF(*_retval = mItems[aIndex]);
    return NS_OK;
  }
  *_retval = nsnull;
  return NS_ERROR_DOM_INDEX_SIZE_ERR;
}

NS_IMETHODIMP
DOMSVGPointList::InsertItemBefore(nsIDOMSVGPoint *aNewItem,
                                  PRUint32 aIndex,
                                  nsIDOMSVGPoint **_retval)
{
  *_retval = nsnull;
  if (IsAnimValList()) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  nsCOMPtr<DOMSVGPoint> domItem = do_QueryInterface(aNewItem);
  if (!domItem) {
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;
  }
  if (domItem->HasOwner() || domItem->IsReadonly()) {
    domItem = domItem->Clone(); 
  }
  aIndex = NS_MIN(aIndex, mItems.Length());

  
  if (!mItems.SetCapacity(mItems.Length() + 1) ||
      !InternalList().SetCapacity(InternalList().Length() + 1)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  InternalList().InsertItem(aIndex, domItem->ToSVGPoint());
  mItems.InsertElementAt(aIndex, domItem.get());

  
  
  domItem->InsertingIntoList(this, aIndex, IsAnimValList());

  for (PRUint32 i = aIndex + 1; i < Length(); ++i) {
    if (mItems[i]) {
      mItems[i]->UpdateListIndex(i);
    }
  }

  Element()->DidChangePointList(PR_TRUE);
#ifdef MOZ_SMIL
  if (AttrIsAnimating()) {
    Element()->AnimationNeedsResample();
  }
#endif
  *_retval = domItem.forget().get();
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGPointList::ReplaceItem(nsIDOMSVGPoint *aNewItem,
                             PRUint32 aIndex,
                             nsIDOMSVGPoint **_retval)
{
  *_retval = nsnull;
  if (IsAnimValList()) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  nsCOMPtr<DOMSVGPoint> domItem = do_QueryInterface(aNewItem);
  if (!domItem) {
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;
  }
  if (aIndex >= Length()) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }
  if (domItem->HasOwner() || domItem->IsReadonly()) {
    domItem = domItem->Clone(); 
  }

  if (mItems[aIndex]) {
    
    
    mItems[aIndex]->RemovingFromList();
  }

  InternalList()[aIndex] = domItem->ToSVGPoint();
  mItems[aIndex] = domItem;

  
  
  domItem->InsertingIntoList(this, aIndex, IsAnimValList());

  Element()->DidChangePointList(PR_TRUE);
#ifdef MOZ_SMIL
  if (AttrIsAnimating()) {
    Element()->AnimationNeedsResample();
  }
#endif
  NS_ADDREF(*_retval = domItem.get());
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGPointList::RemoveItem(PRUint32 aIndex,
                            nsIDOMSVGPoint **_retval)
{
  *_retval = nsnull;
  if (IsAnimValList()) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  if (aIndex >= Length()) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }
  
  EnsureItemAt(aIndex);

  
  
  mItems[aIndex]->RemovingFromList();
  NS_ADDREF(*_retval = mItems[aIndex]);

  InternalList().RemoveItem(aIndex);
  mItems.RemoveElementAt(aIndex);

  for (PRUint32 i = aIndex; i < Length(); ++i) {
    if (mItems[i]) {
      mItems[i]->UpdateListIndex(i);
    }
  }

  Element()->DidChangePointList(PR_TRUE);
#ifdef MOZ_SMIL
  if (AttrIsAnimating()) {
    Element()->AnimationNeedsResample();
  }
#endif
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGPointList::AppendItem(nsIDOMSVGPoint *aNewItem,
                            nsIDOMSVGPoint **_retval)
{
  return InsertItemBefore(aNewItem, Length(), _retval);
}

void
DOMSVGPointList::EnsureItemAt(PRUint32 aIndex)
{
  if (!mItems[aIndex]) {
    mItems[aIndex] = new DOMSVGPoint(this, aIndex, IsAnimValList());
  }
}

