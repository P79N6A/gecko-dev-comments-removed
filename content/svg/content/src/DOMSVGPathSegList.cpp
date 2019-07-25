



































#include "nsSVGElement.h"
#include "DOMSVGPathSegList.h"
#include "DOMSVGPathSeg.h"
#include "nsDOMError.h"
#include "SVGAnimatedPathSegList.h"
#include "nsCOMPtr.h"
#include "nsSVGAttrTearoffTable.h"



namespace mozilla {

static nsSVGAttrTearoffTable<void, DOMSVGPathSegList>
  sSVGPathSegListTearoffTable;

NS_SVG_VAL_IMPL_CYCLE_COLLECTION(DOMSVGPathSegList, mElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(DOMSVGPathSegList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DOMSVGPathSegList)

} 
DOMCI_DATA(SVGPathSegList, mozilla::DOMSVGPathSegList)
namespace mozilla {

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMSVGPathSegList)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGPathSegList)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGPathSegList)
NS_INTERFACE_MAP_END


 already_AddRefed<DOMSVGPathSegList>
DOMSVGPathSegList::GetDOMWrapper(void *aList,
                                 nsSVGElement *aElement,
                                 PRBool aIsAnimValList)
{
  DOMSVGPathSegList *wrapper =
    sSVGPathSegListTearoffTable.GetTearoff(aList);
  if (!wrapper) {
    wrapper = new DOMSVGPathSegList(aElement, aIsAnimValList);
    sSVGPathSegListTearoffTable.AddTearoff(aList, wrapper);
  }
  NS_ADDREF(wrapper);
  return wrapper;
}

 DOMSVGPathSegList*
DOMSVGPathSegList::GetDOMWrapperIfExists(void *aList)
{
  return sSVGPathSegListTearoffTable.GetTearoff(aList);
}

DOMSVGPathSegList::~DOMSVGPathSegList()
{
  
  
  void *key = mIsAnimValList ?
    InternalAList().GetAnimValKey() :
    InternalAList().GetBaseValKey();
  sSVGPathSegListTearoffTable.RemoveTearoff(key);
}

nsIDOMSVGPathSeg*
DOMSVGPathSegList::GetItemWithoutAddRef(PRUint32 aIndex)
{
#ifdef MOZ_SMIL
  if (IsAnimValList()) {
    Element()->FlushAnimations();
  }
#endif
  if (aIndex < Length()) {
    EnsureItemAt(aIndex);
    return ItemAt(aIndex);
  }
  return nsnull;
}

void
DOMSVGPathSegList::InternalListWillChangeTo(const SVGPathData& aNewValue)
{
  
  
  
  
  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  PRUint32 length = mItems.Length();
  PRUint32 index = 0;

  PRUint32 dataLength = aNewValue.mData.Length();
  PRUint32 dataIndex = 0; 

  PRUint32 newSegType;

  nsRefPtr<DOMSVGPathSegList> kungFuDeathGrip;
  if (length && aNewValue.IsEmpty()) {
    
    
    kungFuDeathGrip = this;
  }

  while (index < length && dataIndex < dataLength) {
    newSegType = SVGPathSegUtils::DecodeType(aNewValue.mData[dataIndex]);
    if (ItemAt(index) && ItemAt(index)->Type() != newSegType) {
      ItemAt(index)->RemovingFromList();
      ItemAt(index) = nsnull;
    }
    
    mItems[index].mInternalDataIndex = dataIndex;
    ++index;
    dataIndex += 1 + SVGPathSegUtils::ArgCountForType(newSegType);
  }

  NS_ABORT_IF_FALSE((index == length && dataIndex <= dataLength) ||
                    (index <= length && dataIndex == dataLength),
                    "very bad - list corruption?");

  if (index < length) {
    

    PRUint32 newLength = index;

    
    for (; index < length; ++index) {
      if (ItemAt(index)) {
        ItemAt(index)->RemovingFromList();
        ItemAt(index) = nsnull;
      }
    }

    
    mItems.SetLength(newLength);

  } else if (dataIndex < dataLength) {
    

    
    while (dataIndex < dataLength) {
      if (mItems.Length() &&
          mItems.Length() - 1 > DOMSVGPathSeg::MaxListIndex()) {
        
        
        return;
      }
      if (!mItems.AppendElement(ItemProxy(nsnull, dataIndex))) {
        
        Clear();
        return;
      }
      dataIndex += 1 + SVGPathSegUtils::ArgCountForType(SVGPathSegUtils::DecodeType(aNewValue.mData[dataIndex]));
    }
  }

  NS_ABORT_IF_FALSE(dataIndex == dataLength, "Serious processing error");
  NS_ABORT_IF_FALSE(index == length, "Serious counting error");
}

PRBool
DOMSVGPathSegList::AttrIsAnimating() const
{
  return const_cast<DOMSVGPathSegList*>(this)->InternalAList().IsAnimating();
}

SVGPathData&
DOMSVGPathSegList::InternalList()
{
  SVGAnimatedPathSegList *alist = mElement->GetAnimPathSegList();
  return mIsAnimValList && alist->IsAnimating() ? *alist->mAnimVal : alist->mBaseVal;
}

SVGAnimatedPathSegList&
DOMSVGPathSegList::InternalAList()
{
  NS_ABORT_IF_FALSE(mElement->GetAnimPathSegList(), "Internal error");
  return *mElement->GetAnimPathSegList();
}




NS_IMETHODIMP
DOMSVGPathSegList::GetNumberOfItems(PRUint32 *aNumberOfItems)
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
DOMSVGPathSegList::Clear()
{
  if (IsAnimValList()) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  if (Length() > 0) {
    
    
    

    InternalListWillChangeTo(SVGPathData()); 

    if (!AttrIsAnimating()) {
      
      DOMSVGPathSegList *animList =
        GetDOMWrapperIfExists(InternalAList().GetAnimValKey());
      if (animList) {
        animList->InternalListWillChangeTo(SVGPathData()); 
      }
    }

    InternalList().Clear();
    Element()->DidChangePathSegList(PR_TRUE);
#ifdef MOZ_SMIL
    if (AttrIsAnimating()) {
      Element()->AnimationNeedsResample();
    }
#endif
  }
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGPathSegList::Initialize(nsIDOMSVGPathSeg *aNewItem,
                              nsIDOMSVGPathSeg **_retval)
{
  *_retval = nsnull;
  if (IsAnimValList()) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  
  
  
  
  
  
  

  nsCOMPtr<DOMSVGPathSeg> domItem = do_QueryInterface(aNewItem);
  if (!domItem) {
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;
  }
  if (domItem->HasOwner()) {
    aNewItem = domItem->Clone();
  }

  Clear();
  return InsertItemBefore(aNewItem, 0, _retval);
}

NS_IMETHODIMP
DOMSVGPathSegList::GetItem(PRUint32 aIndex,
                           nsIDOMSVGPathSeg **_retval)
{
  *_retval = GetItemWithoutAddRef(aIndex);
  if (!*_retval) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }
  NS_ADDREF(*_retval);
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGPathSegList::InsertItemBefore(nsIDOMSVGPathSeg *aNewItem,
                                    PRUint32 aIndex,
                                    nsIDOMSVGPathSeg **_retval)
{
  *_retval = nsnull;
  if (IsAnimValList()) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  PRUint32 internalIndex;
  if (aIndex < Length()) {
    internalIndex = mItems[aIndex].mInternalDataIndex;
  } else {
    aIndex = Length();
    internalIndex = InternalList().mData.Length();
  }
  if (aIndex >= DOMSVGPathSeg::MaxListIndex()) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  nsCOMPtr<DOMSVGPathSeg> domItem = do_QueryInterface(aNewItem);
  if (!domItem) {
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;
  }
  if (domItem->HasOwner()) {
    domItem = domItem->Clone(); 
  }

  PRUint32 argCount = SVGPathSegUtils::ArgCountForType(domItem->Type());

  
  if (!mItems.SetCapacity(mItems.Length() + 1) ||
      !InternalList().mData.SetCapacity(InternalList().mData.Length() + 1 + argCount)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  MaybeInsertNullInAnimValListAt(aIndex, internalIndex, argCount);

  float segAsRaw[1 + NS_SVG_PATH_SEG_MAX_ARGS];
  domItem->ToSVGPathSegEncodedData(segAsRaw);

  InternalList().mData.InsertElementsAt(internalIndex, segAsRaw, 1 + argCount);
  mItems.InsertElementAt(aIndex, ItemProxy(domItem.get(), internalIndex));

  
  
  
  domItem->InsertingIntoList(this, aIndex, IsAnimValList());

  UpdateListIndicesFromIndex(aIndex + 1, argCount + 1);

  Element()->DidChangePathSegList(PR_TRUE);
#ifdef MOZ_SMIL
  if (AttrIsAnimating()) {
    Element()->AnimationNeedsResample();
  }
#endif
  *_retval = domItem.forget().get();
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGPathSegList::ReplaceItem(nsIDOMSVGPathSeg *aNewItem,
                               PRUint32 aIndex,
                               nsIDOMSVGPathSeg **_retval)
{
  *_retval = nsnull;
  if (IsAnimValList()) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  nsCOMPtr<DOMSVGPathSeg> domItem = do_QueryInterface(aNewItem);
  if (!domItem) {
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;
  }
  if (aIndex >= Length()) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }
  if (domItem->HasOwner()) {
    domItem = domItem->Clone(); 
  }

  if (ItemAt(aIndex)) {
    
    
    ItemAt(aIndex)->RemovingFromList();
  }

  PRUint32 internalIndex = mItems[aIndex].mInternalDataIndex;
  
  
  PRUint32 oldType = SVGPathSegUtils::DecodeType(InternalList().mData[internalIndex]);
  PRUint32 oldArgCount = SVGPathSegUtils::ArgCountForType(oldType);
  PRUint32 newArgCount = SVGPathSegUtils::ArgCountForType(domItem->Type());

  float segAsRaw[1 + NS_SVG_PATH_SEG_MAX_ARGS];
  domItem->ToSVGPathSegEncodedData(segAsRaw);

  PRBool ok = !!InternalList().mData.ReplaceElementsAt(
                  internalIndex, 1 + oldArgCount,
                  segAsRaw, 1 + newArgCount);
  if (!ok) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  ItemAt(aIndex) = domItem;

  
  
  domItem->InsertingIntoList(this, aIndex, IsAnimValList());

  PRUint32 delta = newArgCount - oldArgCount;
  if (delta != 0) {
    for (PRUint32 i = aIndex + 1; i < Length(); ++i) {
      mItems[i].mInternalDataIndex += delta;
    }
  }

  Element()->DidChangePathSegList(PR_TRUE);
#ifdef MOZ_SMIL
  if (AttrIsAnimating()) {
    Element()->AnimationNeedsResample();
  }
#endif
  NS_ADDREF(*_retval = domItem.get());
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGPathSegList::RemoveItem(PRUint32 aIndex,
                              nsIDOMSVGPathSeg **_retval)
{
  *_retval = nsnull;
  if (IsAnimValList()) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  if (aIndex >= Length()) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }
  
  EnsureItemAt(aIndex);

  
  
  ItemAt(aIndex)->RemovingFromList();
  NS_ADDREF(*_retval = ItemAt(aIndex));

  PRUint32 internalIndex = mItems[aIndex].mInternalDataIndex;
  PRUint32 segType = SVGPathSegUtils::DecodeType(InternalList().mData[internalIndex]);
  PRUint32 argCount = SVGPathSegUtils::ArgCountForType(segType);

  
  
  
  MaybeRemoveItemFromAnimValListAt(aIndex, argCount);

  InternalList().mData.RemoveElementsAt(internalIndex, 1 + argCount);
  mItems.RemoveElementAt(aIndex);

  UpdateListIndicesFromIndex(aIndex, -(argCount + 1));

  Element()->DidChangePathSegList(PR_TRUE);
#ifdef MOZ_SMIL
  if (AttrIsAnimating()) {
    Element()->AnimationNeedsResample();
  }
#endif
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGPathSegList::AppendItem(nsIDOMSVGPathSeg *aNewItem,
                              nsIDOMSVGPathSeg **_retval)
{
  return InsertItemBefore(aNewItem, Length(), _retval);
}

NS_IMETHODIMP
DOMSVGPathSegList::GetLength(PRUint32 *aNumberOfItems)
{
  return GetNumberOfItems(aNumberOfItems);
}

void
DOMSVGPathSegList::EnsureItemAt(PRUint32 aIndex)
{
  if (!ItemAt(aIndex)) {
    ItemAt(aIndex) = DOMSVGPathSeg::CreateFor(this, aIndex, IsAnimValList());
  }
}

void
DOMSVGPathSegList::
  MaybeInsertNullInAnimValListAt(PRUint32 aIndex,
                                 PRUint32 aInternalIndex,
                                 PRUint32 aArgCountForItem)
{
  NS_ABORT_IF_FALSE(!IsAnimValList(), "call from baseVal to animVal");

  if (AttrIsAnimating()) {
    
    return;
  }

  
  DOMSVGPathSegList *animVal =
    GetDOMWrapperIfExists(InternalAList().GetAnimValKey());
  if (!animVal) {
    
    return;
  }

  NS_ABORT_IF_FALSE(animVal->mItems.Length() == mItems.Length(),
                    "animVal list not in sync!");

  animVal->mItems.InsertElementAt(aIndex, ItemProxy(nsnull, aInternalIndex));

  animVal->UpdateListIndicesFromIndex(aIndex + 1, 1 + aArgCountForItem);
}

void
DOMSVGPathSegList::
  MaybeRemoveItemFromAnimValListAt(PRUint32 aIndex,
                                   PRUint32 aArgCountForItem)
{
  NS_ABORT_IF_FALSE(!IsAnimValList(), "call from baseVal to animVal");

  if (AttrIsAnimating()) {
    
    return;
  }

  
  
  nsRefPtr<DOMSVGPathSegList> animVal =
    GetDOMWrapperIfExists(InternalAList().GetAnimValKey());
  if (!animVal) {
    
    return;
  }

  NS_ABORT_IF_FALSE(animVal->mItems.Length() == mItems.Length(),
                    "animVal list not in sync!");

  if (animVal->ItemAt(aIndex)) {
    animVal->ItemAt(aIndex)->RemovingFromList();
  }
  animVal->mItems.RemoveElementAt(aIndex);

  animVal->UpdateListIndicesFromIndex(aIndex, -(1 + aArgCountForItem));
}

void
DOMSVGPathSegList::UpdateListIndicesFromIndex(PRUint32 aStartingIndex,
                                              PRInt32  aInternalDataIndexDelta)
{
  PRUint32 length = mItems.Length();

  for (PRUint32 i = aStartingIndex; i < length; ++i) {
    mItems[i].mInternalDataIndex += aInternalDataIndexDelta;
    if (ItemAt(i)) {
      ItemAt(i)->UpdateListIndex(i);
    }
  }
}

} 
