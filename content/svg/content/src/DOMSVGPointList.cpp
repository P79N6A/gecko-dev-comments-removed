




#include "nsSVGElement.h"
#include "DOMSVGPointList.h"
#include "DOMSVGPoint.h"
#include "nsError.h"
#include "SVGAnimatedPointList.h"
#include "nsCOMPtr.h"
#include "nsSVGAttrTearoffTable.h"
#include "nsContentUtils.h"
#include "dombindings.h"




namespace {

using mozilla::DOMSVGPoint;

void
UpdateListIndicesFromIndex(nsTArray<DOMSVGPoint*>& aItemsArray,
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

static nsSVGAttrTearoffTable<void, DOMSVGPointList>
  sSVGPointListTearoffTable;

NS_IMPL_CYCLE_COLLECTION_CLASS(DOMSVGPointList)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(DOMSVGPointList)
  
  
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(DOMSVGPointList)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mElement, nsIContent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(DOMSVGPointList)
  NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(DOMSVGPointList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DOMSVGPointList)

} 
DOMCI_DATA(SVGPointList, mozilla::DOMSVGPointList)
namespace mozilla {

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMSVGPointList)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGPointList)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGPointList)
NS_INTERFACE_MAP_END


 already_AddRefed<DOMSVGPointList>
DOMSVGPointList::GetDOMWrapper(void *aList,
                               nsSVGElement *aElement,
                               bool aIsAnimValList)
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

JSObject*
DOMSVGPointList::WrapObject(JSContext *cx, JSObject *scope, bool *triedToWrap)
{
  return mozilla::dom::binding::SVGPointList::create(cx, scope, this,
                                                     triedToWrap);
}

nsIDOMSVGPoint*
DOMSVGPointList::GetItemAt(uint32_t aIndex)
{
  if (IsAnimValList()) {
    Element()->FlushAnimations();
  }
  if (aIndex < Length()) {
    EnsureItemAt(aIndex);
    return mItems[aIndex];
  }
  return nullptr;
}

void
DOMSVGPointList::InternalListWillChangeTo(const SVGPointList& aNewValue)
{
  
  
  

  uint32_t oldLength = mItems.Length();

  uint32_t newLength = aNewValue.Length();
  if (newLength > DOMSVGPoint::MaxListIndex()) {
    
    
    newLength = DOMSVGPoint::MaxListIndex();
  }

  nsRefPtr<DOMSVGPointList> kungFuDeathGrip;
  if (newLength < oldLength) {
    
    
    kungFuDeathGrip = this;
  }

  
  for (uint32_t i = newLength; i < oldLength; ++i) {
    if (mItems[i]) {
      mItems[i]->RemovingFromList();
    }
  }

  if (!mItems.SetLength(newLength)) {
    
    
    mItems.Clear();
    return;
  }

  
  for (uint32_t i = oldLength; i < newLength; ++i) {
    mItems[i] = nullptr;
  }
}

bool
DOMSVGPointList::AttrIsAnimating() const
{
  return InternalAList().IsAnimating();
}

SVGPointList&
DOMSVGPointList::InternalList() const
{
  SVGAnimatedPointList *alist = mElement->GetAnimatedPointList();
  return mIsAnimValList && alist->IsAnimating() ? *alist->mAnimVal : alist->mBaseVal;
}

SVGAnimatedPointList&
DOMSVGPointList::InternalAList() const
{
  NS_ABORT_IF_FALSE(mElement->GetAnimatedPointList(), "Internal error");
  return *mElement->GetAnimatedPointList();
}




NS_IMETHODIMP
DOMSVGPointList::GetNumberOfItems(uint32_t *aNumberOfItems)
{
  if (IsAnimValList()) {
    Element()->FlushAnimations();
  }
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
    nsAttrValue emptyOrOldValue = Element()->WillChangePointList();
    
    
    

    InternalListWillChangeTo(SVGPointList()); 

    if (!AttrIsAnimating()) {
      
      DOMSVGPointList *animList =
        GetDOMWrapperIfExists(InternalAList().GetAnimValKey());
      if (animList) {
        animList->InternalListWillChangeTo(SVGPointList()); 
      }
    }

    InternalList().Clear();
    Element()->DidChangePointList(emptyOrOldValue);
    if (AttrIsAnimating()) {
      Element()->AnimationNeedsResample();
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGPointList::Initialize(nsIDOMSVGPoint *aNewItem,
                            nsIDOMSVGPoint **_retval)
{
  *_retval = nullptr;
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
DOMSVGPointList::GetItem(uint32_t aIndex,
                         nsIDOMSVGPoint **_retval)
{
  *_retval = GetItemAt(aIndex);
  if (!*_retval) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }
  NS_ADDREF(*_retval);
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGPointList::InsertItemBefore(nsIDOMSVGPoint *aNewItem,
                                  uint32_t aIndex,
                                  nsIDOMSVGPoint **_retval)
{
  *_retval = nullptr;
  if (IsAnimValList()) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  aIndex = NS_MIN(aIndex, Length());
  if (aIndex >= DOMSVGPoint::MaxListIndex()) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  nsCOMPtr<DOMSVGPoint> domItem = do_QueryInterface(aNewItem);
  if (!domItem) {
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;
  }
  if (domItem->HasOwner() || domItem->IsReadonly()) {
    domItem = domItem->Clone(); 
  }

  
  if (!mItems.SetCapacity(mItems.Length() + 1) ||
      !InternalList().SetCapacity(InternalList().Length() + 1)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsAttrValue emptyOrOldValue = Element()->WillChangePointList();
  
  MaybeInsertNullInAnimValListAt(aIndex);

  InternalList().InsertItem(aIndex, domItem->ToSVGPoint());
  mItems.InsertElementAt(aIndex, domItem.get());

  
  
  
  domItem->InsertingIntoList(this, aIndex, IsAnimValList());

  UpdateListIndicesFromIndex(mItems, aIndex + 1);

  Element()->DidChangePointList(emptyOrOldValue);
  if (AttrIsAnimating()) {
    Element()->AnimationNeedsResample();
  }
  domItem.forget(_retval);
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGPointList::ReplaceItem(nsIDOMSVGPoint *aNewItem,
                             uint32_t aIndex,
                             nsIDOMSVGPoint **_retval)
{
  *_retval = nullptr;
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

  nsAttrValue emptyOrOldValue = Element()->WillChangePointList();
  if (mItems[aIndex]) {
    
    
    mItems[aIndex]->RemovingFromList();
  }

  InternalList()[aIndex] = domItem->ToSVGPoint();
  mItems[aIndex] = domItem;

  
  
  domItem->InsertingIntoList(this, aIndex, IsAnimValList());

  Element()->DidChangePointList(emptyOrOldValue);
  if (AttrIsAnimating()) {
    Element()->AnimationNeedsResample();
  }
  NS_ADDREF(*_retval = domItem.get());
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGPointList::RemoveItem(uint32_t aIndex,
                            nsIDOMSVGPoint **_retval)
{
  *_retval = nullptr;
  if (IsAnimValList()) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  if (aIndex >= Length()) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  nsAttrValue emptyOrOldValue = Element()->WillChangePointList();
  
  
  
  MaybeRemoveItemFromAnimValListAt(aIndex);

  
  EnsureItemAt(aIndex);

  
  
  mItems[aIndex]->RemovingFromList();
  NS_ADDREF(*_retval = mItems[aIndex]);

  InternalList().RemoveItem(aIndex);
  mItems.RemoveElementAt(aIndex);

  UpdateListIndicesFromIndex(mItems, aIndex);

  Element()->DidChangePointList(emptyOrOldValue);
  if (AttrIsAnimating()) {
    Element()->AnimationNeedsResample();
  }
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGPointList::AppendItem(nsIDOMSVGPoint *aNewItem,
                            nsIDOMSVGPoint **_retval)
{
  return InsertItemBefore(aNewItem, Length(), _retval);
}

NS_IMETHODIMP
DOMSVGPointList::GetLength(uint32_t *aNumberOfItems)
{
  return GetNumberOfItems(aNumberOfItems);
}

void
DOMSVGPointList::EnsureItemAt(uint32_t aIndex)
{
  if (!mItems[aIndex]) {
    mItems[aIndex] = new DOMSVGPoint(this, aIndex, IsAnimValList());
  }
}

void
DOMSVGPointList::MaybeInsertNullInAnimValListAt(uint32_t aIndex)
{
  NS_ABORT_IF_FALSE(!IsAnimValList(), "call from baseVal to animVal");

  if (AttrIsAnimating()) {
    
    return;
  }

  
  DOMSVGPointList *animVal =
    GetDOMWrapperIfExists(InternalAList().GetAnimValKey());
  if (!animVal) {
    
    return;
  }

  NS_ABORT_IF_FALSE(animVal->mItems.Length() == mItems.Length(),
                    "animVal list not in sync!");

  animVal->mItems.InsertElementAt(aIndex, static_cast<DOMSVGPoint*>(nullptr));

  UpdateListIndicesFromIndex(animVal->mItems, aIndex + 1);
}

void
DOMSVGPointList::MaybeRemoveItemFromAnimValListAt(uint32_t aIndex)
{
  NS_ABORT_IF_FALSE(!IsAnimValList(), "call from baseVal to animVal");

  if (AttrIsAnimating()) {
    
    return;
  }

  
  
  nsRefPtr<DOMSVGPointList> animVal =
    GetDOMWrapperIfExists(InternalAList().GetAnimValKey());
  if (!animVal) {
    
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
