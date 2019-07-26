




#include "nsSVGElement.h"
#include "DOMSVGPathSegList.h"
#include "DOMSVGPathSeg.h"
#include "nsError.h"
#include "SVGAnimatedPathSegList.h"
#include "nsCOMPtr.h"
#include "nsSVGAttrTearoffTable.h"
#include "SVGPathSegUtils.h"
#include "dombindings.h"
#include "nsContentUtils.h"



namespace mozilla {

static nsSVGAttrTearoffTable<void, DOMSVGPathSegList>
  sSVGPathSegListTearoffTable;

NS_IMPL_CYCLE_COLLECTION_CLASS(DOMSVGPathSegList)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(DOMSVGPathSegList)
  
  
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(DOMSVGPathSegList)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mElement, nsIContent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(DOMSVGPathSegList)
  NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(DOMSVGPathSegList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DOMSVGPathSegList)

} 
DOMCI_DATA(SVGPathSegList, mozilla::DOMSVGPathSegList)
namespace mozilla {

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMSVGPathSegList)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGPathSegList)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGPathSegList)
NS_INTERFACE_MAP_END


 already_AddRefed<DOMSVGPathSegList>
DOMSVGPathSegList::GetDOMWrapper(void *aList,
                                 nsSVGElement *aElement,
                                 bool aIsAnimValList)
{
  nsRefPtr<DOMSVGPathSegList> wrapper =
    sSVGPathSegListTearoffTable.GetTearoff(aList);
  if (!wrapper) {
    wrapper = new DOMSVGPathSegList(aElement, aIsAnimValList);
    sSVGPathSegListTearoffTable.AddTearoff(aList, wrapper);
  }
  return wrapper.forget();
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

JSObject*
DOMSVGPathSegList::WrapObject(JSContext *cx, JSObject *scope, bool *triedToWrap)
{
  return mozilla::dom::oldproxybindings::SVGPathSegList::create(cx, scope, this,
                                                       triedToWrap);
}

nsIDOMSVGPathSeg*
DOMSVGPathSegList::GetItemAt(uint32_t aIndex)
{
  if (IsAnimValList()) {
    Element()->FlushAnimations();
  }
  if (aIndex < LengthNoFlush()) {
    EnsureItemAt(aIndex);
    return ItemAt(aIndex);
  }
  return nullptr;
}

void
DOMSVGPathSegList::InternalListWillChangeTo(const SVGPathData& aNewValue)
{
  
  
  
  
  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  uint32_t length = mItems.Length();
  uint32_t index = 0;

  uint32_t dataLength = aNewValue.mData.Length();
  uint32_t dataIndex = 0; 

  uint32_t newSegType;

  nsRefPtr<DOMSVGPathSegList> kungFuDeathGrip;
  if (length) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    kungFuDeathGrip = this;
  }

  while (index < length && dataIndex < dataLength) {
    newSegType = SVGPathSegUtils::DecodeType(aNewValue.mData[dataIndex]);
    if (ItemAt(index) && ItemAt(index)->Type() != newSegType) {
      ItemAt(index)->RemovingFromList();
      ItemAt(index) = nullptr;
    }
    
    mItems[index].mInternalDataIndex = dataIndex;
    ++index;
    dataIndex += 1 + SVGPathSegUtils::ArgCountForType(newSegType);
  }

  NS_ABORT_IF_FALSE((index == length && dataIndex <= dataLength) ||
                    (index <= length && dataIndex == dataLength),
                    "very bad - list corruption?");

  if (index < length) {
    

    uint32_t newLength = index;

    
    for (; index < length; ++index) {
      if (ItemAt(index)) {
        ItemAt(index)->RemovingFromList();
        ItemAt(index) = nullptr;
      }
    }

    
    mItems.SetLength(newLength);

  } else if (dataIndex < dataLength) {
    

    
    while (dataIndex < dataLength) {
      if (mItems.Length() &&
          mItems.Length() - 1 > DOMSVGPathSeg::MaxListIndex()) {
        
        
        return;
      }
      if (!mItems.AppendElement(ItemProxy(nullptr, dataIndex))) {
        
        Clear();
        return;
      }
      dataIndex += 1 + SVGPathSegUtils::ArgCountForType(SVGPathSegUtils::DecodeType(aNewValue.mData[dataIndex]));
    }
  }

  NS_ABORT_IF_FALSE(dataIndex == dataLength, "Serious processing error");
  NS_ABORT_IF_FALSE(index == length, "Serious counting error");
}

bool
DOMSVGPathSegList::AttrIsAnimating() const
{
  return InternalAList().IsAnimating();
}

SVGPathData&
DOMSVGPathSegList::InternalList() const
{
  SVGAnimatedPathSegList *alist = mElement->GetAnimPathSegList();
  return mIsAnimValList && alist->IsAnimating() ? *alist->mAnimVal : alist->mBaseVal;
}

SVGAnimatedPathSegList&
DOMSVGPathSegList::InternalAList() const
{
  NS_ABORT_IF_FALSE(mElement->GetAnimPathSegList(), "Internal error");
  return *mElement->GetAnimPathSegList();
}




NS_IMETHODIMP
DOMSVGPathSegList::GetNumberOfItems(uint32_t *aNumberOfItems)
{
  if (IsAnimValList()) {
    Element()->FlushAnimations();
  }
  *aNumberOfItems = LengthNoFlush();
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGPathSegList::Clear()
{
  if (IsAnimValList()) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  if (LengthNoFlush() > 0) {
    nsAttrValue emptyOrOldValue = Element()->WillChangePathSegList();
    
    
    

    InternalListWillChangeTo(SVGPathData()); 

    if (!AttrIsAnimating()) {
      
      DOMSVGPathSegList *animList =
        GetDOMWrapperIfExists(InternalAList().GetAnimValKey());
      if (animList) {
        animList->InternalListWillChangeTo(SVGPathData()); 
      }
    }

    InternalList().Clear();
    Element()->DidChangePathSegList(emptyOrOldValue);
    if (AttrIsAnimating()) {
      Element()->AnimationNeedsResample();
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGPathSegList::Initialize(nsIDOMSVGPathSeg *aNewItem,
                              nsIDOMSVGPathSeg **_retval)
{
  *_retval = nullptr;
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
DOMSVGPathSegList::GetItem(uint32_t aIndex,
                           nsIDOMSVGPathSeg **_retval)
{
  *_retval = GetItemAt(aIndex);
  if (!*_retval) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }
  NS_ADDREF(*_retval);
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGPathSegList::InsertItemBefore(nsIDOMSVGPathSeg *aNewItem,
                                    uint32_t aIndex,
                                    nsIDOMSVGPathSeg **_retval)
{
  *_retval = nullptr;
  if (IsAnimValList()) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  uint32_t internalIndex;
  if (aIndex < LengthNoFlush()) {
    internalIndex = mItems[aIndex].mInternalDataIndex;
  } else {
    aIndex = LengthNoFlush();
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

  uint32_t argCount = SVGPathSegUtils::ArgCountForType(domItem->Type());

  
  if (!mItems.SetCapacity(mItems.Length() + 1) ||
      !InternalList().mData.SetCapacity(InternalList().mData.Length() + 1 + argCount)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsAttrValue emptyOrOldValue = Element()->WillChangePathSegList();
  
  MaybeInsertNullInAnimValListAt(aIndex, internalIndex, argCount);

  float segAsRaw[1 + NS_SVG_PATH_SEG_MAX_ARGS];
  domItem->ToSVGPathSegEncodedData(segAsRaw);

  InternalList().mData.InsertElementsAt(internalIndex, segAsRaw, 1 + argCount);
  mItems.InsertElementAt(aIndex, ItemProxy(domItem.get(), internalIndex));

  
  
  
  domItem->InsertingIntoList(this, aIndex, IsAnimValList());

  UpdateListIndicesFromIndex(aIndex + 1, argCount + 1);

  Element()->DidChangePathSegList(emptyOrOldValue);
  if (AttrIsAnimating()) {
    Element()->AnimationNeedsResample();
  }
  domItem.forget(_retval);
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGPathSegList::ReplaceItem(nsIDOMSVGPathSeg *aNewItem,
                               uint32_t aIndex,
                               nsIDOMSVGPathSeg **_retval)
{
  *_retval = nullptr;
  if (IsAnimValList()) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  nsCOMPtr<DOMSVGPathSeg> domItem = do_QueryInterface(aNewItem);
  if (!domItem) {
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;
  }
  if (aIndex >= LengthNoFlush()) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }
  if (domItem->HasOwner()) {
    domItem = domItem->Clone(); 
  }

  nsAttrValue emptyOrOldValue = Element()->WillChangePathSegList();
  if (ItemAt(aIndex)) {
    
    
    ItemAt(aIndex)->RemovingFromList();
  }

  uint32_t internalIndex = mItems[aIndex].mInternalDataIndex;
  
  
  uint32_t oldType = SVGPathSegUtils::DecodeType(InternalList().mData[internalIndex]);
  uint32_t oldArgCount = SVGPathSegUtils::ArgCountForType(oldType);
  uint32_t newArgCount = SVGPathSegUtils::ArgCountForType(domItem->Type());

  float segAsRaw[1 + NS_SVG_PATH_SEG_MAX_ARGS];
  domItem->ToSVGPathSegEncodedData(segAsRaw);

  bool ok = !!InternalList().mData.ReplaceElementsAt(
                  internalIndex, 1 + oldArgCount,
                  segAsRaw, 1 + newArgCount);
  if (!ok) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  ItemAt(aIndex) = domItem;

  
  
  domItem->InsertingIntoList(this, aIndex, IsAnimValList());

  uint32_t delta = newArgCount - oldArgCount;
  if (delta != 0) {
    for (uint32_t i = aIndex + 1; i < LengthNoFlush(); ++i) {
      mItems[i].mInternalDataIndex += delta;
    }
  }

  Element()->DidChangePathSegList(emptyOrOldValue);
  if (AttrIsAnimating()) {
    Element()->AnimationNeedsResample();
  }
  NS_ADDREF(*_retval = domItem.get());
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGPathSegList::RemoveItem(uint32_t aIndex,
                              nsIDOMSVGPathSeg **_retval)
{
  *_retval = nullptr;
  if (IsAnimValList()) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  if (aIndex >= LengthNoFlush()) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }
  
  EnsureItemAt(aIndex);

  nsAttrValue emptyOrOldValue = Element()->WillChangePathSegList();
  
  
  ItemAt(aIndex)->RemovingFromList();
  NS_ADDREF(*_retval = ItemAt(aIndex));

  uint32_t internalIndex = mItems[aIndex].mInternalDataIndex;
  uint32_t segType = SVGPathSegUtils::DecodeType(InternalList().mData[internalIndex]);
  uint32_t argCount = SVGPathSegUtils::ArgCountForType(segType);

  
  
  
  MaybeRemoveItemFromAnimValListAt(aIndex, argCount);

  InternalList().mData.RemoveElementsAt(internalIndex, 1 + argCount);
  mItems.RemoveElementAt(aIndex);

  UpdateListIndicesFromIndex(aIndex, -(argCount + 1));

  Element()->DidChangePathSegList(emptyOrOldValue);
  if (AttrIsAnimating()) {
    Element()->AnimationNeedsResample();
  }
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGPathSegList::AppendItem(nsIDOMSVGPathSeg *aNewItem,
                              nsIDOMSVGPathSeg **_retval)
{
  return InsertItemBefore(aNewItem, LengthNoFlush(), _retval);
}

NS_IMETHODIMP
DOMSVGPathSegList::GetLength(uint32_t *aNumberOfItems)
{
  return GetNumberOfItems(aNumberOfItems);
}

void
DOMSVGPathSegList::EnsureItemAt(uint32_t aIndex)
{
  if (!ItemAt(aIndex)) {
    ItemAt(aIndex) = DOMSVGPathSeg::CreateFor(this, aIndex, IsAnimValList());
  }
}

void
DOMSVGPathSegList::
  MaybeInsertNullInAnimValListAt(uint32_t aIndex,
                                 uint32_t aInternalIndex,
                                 uint32_t aArgCountForItem)
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

  animVal->mItems.InsertElementAt(aIndex, ItemProxy(nullptr, aInternalIndex));

  animVal->UpdateListIndicesFromIndex(aIndex + 1, 1 + aArgCountForItem);
}

void
DOMSVGPathSegList::
  MaybeRemoveItemFromAnimValListAt(uint32_t aIndex,
                                   uint32_t aArgCountForItem)
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
DOMSVGPathSegList::UpdateListIndicesFromIndex(uint32_t aStartingIndex,
                                              int32_t  aInternalDataIndexDelta)
{
  uint32_t length = mItems.Length();

  for (uint32_t i = aStartingIndex; i < length; ++i) {
    mItems[i].mInternalDataIndex += aInternalDataIndexDelta;
    if (ItemAt(i)) {
      ItemAt(i)->UpdateListIndex(i);
    }
  }
}

} 
