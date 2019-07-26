




#include "nsSVGElement.h"
#include "DOMSVGPointList.h"
#include "DOMSVGPoint.h"
#include "nsError.h"
#include "SVGAnimatedPointList.h"
#include "nsCOMPtr.h"
#include "nsSVGAttrTearoffTable.h"
#include "nsContentUtils.h"
#include "mozilla/dom/SVGPointListBinding.h"
#include <algorithm>




namespace {

void
UpdateListIndicesFromIndex(nsTArray<mozilla::nsISVGPoint*>& aItemsArray,
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

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(DOMSVGPointList)
  
  
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(DOMSVGPointList)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mElement)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(DOMSVGPointList)
  NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(DOMSVGPointList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DOMSVGPointList)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMSVGPointList)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END


 already_AddRefed<DOMSVGPointList>
DOMSVGPointList::GetDOMWrapper(void *aList,
                               nsSVGElement *aElement,
                               bool aIsAnimValList)
{
  nsRefPtr<DOMSVGPointList> wrapper =
    sSVGPointListTearoffTable.GetTearoff(aList);
  if (!wrapper) {
    wrapper = new DOMSVGPointList(aElement, aIsAnimValList);
    sSVGPointListTearoffTable.AddTearoff(aList, wrapper);
  }
  return wrapper.forget();
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
DOMSVGPointList::WrapObject(JSContext *cx, JSObject *scope)
{
  return mozilla::dom::SVGPointListBinding::Wrap(cx, scope, this);
}

void
DOMSVGPointList::InternalListWillChangeTo(const SVGPointList& aNewValue)
{
  
  
  

  uint32_t oldLength = mItems.Length();

  uint32_t newLength = aNewValue.Length();
  if (newLength > nsISVGPoint::MaxListIndex()) {
    
    
    newLength = nsISVGPoint::MaxListIndex();
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




void
DOMSVGPointList::Clear(ErrorResult& aError)
{
  if (IsAnimValList()) {
    aError.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  if (LengthNoFlush() > 0) {
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
}

already_AddRefed<nsISVGPoint>
DOMSVGPointList::Initialize(nsISVGPoint& aNewItem, ErrorResult& aError)
{
  if (IsAnimValList()) {
    aError.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return nullptr;
  }

  
  
  
  
  
  
  

  nsCOMPtr<nsISVGPoint> domItem = &aNewItem;
  if (domItem->HasOwner() || domItem->IsReadonly()) {
    domItem = domItem->Clone(); 
  }

  ErrorResult rv;
  Clear(rv);
  MOZ_ASSERT(!rv.Failed());
  return InsertItemBefore(*domItem, 0, aError);
}

nsISVGPoint*
DOMSVGPointList::IndexedGetter(uint32_t aIndex, bool& aFound,
                               ErrorResult& aError)
{
  if (IsAnimValList()) {
    Element()->FlushAnimations();
  }
  aFound = aIndex < LengthNoFlush();
  if (aFound) {
    EnsureItemAt(aIndex);
    return mItems[aIndex];
  }
  return nullptr;
}

already_AddRefed<nsISVGPoint>
DOMSVGPointList::InsertItemBefore(nsISVGPoint& aNewItem, uint32_t aIndex,
                                  ErrorResult& aError)
{
  if (IsAnimValList()) {
    aError.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return nullptr;
  }

  aIndex = std::min(aIndex, LengthNoFlush());
  if (aIndex >= nsISVGPoint::MaxListIndex()) {
    aError.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return nullptr;
  }

  nsCOMPtr<nsISVGPoint> domItem = &aNewItem;
  if (domItem->HasOwner() || domItem->IsReadonly()) {
    domItem = domItem->Clone(); 
  }

  
  if (!mItems.SetCapacity(mItems.Length() + 1) ||
      !InternalList().SetCapacity(InternalList().Length() + 1)) {
    aError.Throw(NS_ERROR_OUT_OF_MEMORY);
    return nullptr;
  }

  nsAttrValue emptyOrOldValue = Element()->WillChangePointList();
  
  MaybeInsertNullInAnimValListAt(aIndex);

  InternalList().InsertItem(aIndex, domItem->ToSVGPoint());
  mItems.InsertElementAt(aIndex, domItem);

  
  
  
  domItem->InsertingIntoList(this, aIndex, IsAnimValList());

  UpdateListIndicesFromIndex(mItems, aIndex + 1);

  Element()->DidChangePointList(emptyOrOldValue);
  if (AttrIsAnimating()) {
    Element()->AnimationNeedsResample();
  }
  return domItem.forget();
}

already_AddRefed<nsISVGPoint>
DOMSVGPointList::ReplaceItem(nsISVGPoint& aNewItem, uint32_t aIndex,
                             ErrorResult& aError)
{
  if (IsAnimValList()) {
    aError.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return nullptr;
  }

  if (aIndex >= LengthNoFlush()) {
    aError.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return nullptr;
  }

  nsCOMPtr<nsISVGPoint> domItem = &aNewItem;
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
  return domItem.forget();
}

already_AddRefed<nsISVGPoint>
DOMSVGPointList::RemoveItem(uint32_t aIndex, ErrorResult& aError)
{
  if (IsAnimValList()) {
    aError.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return nullptr;
  }

  if (aIndex >= LengthNoFlush()) {
    aError.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return nullptr;
  }

  nsAttrValue emptyOrOldValue = Element()->WillChangePointList();
  
  
  
  MaybeRemoveItemFromAnimValListAt(aIndex);

  
  EnsureItemAt(aIndex);

  
  
  mItems[aIndex]->RemovingFromList();
  nsCOMPtr<nsISVGPoint> result = mItems[aIndex];

  InternalList().RemoveItem(aIndex);
  mItems.RemoveElementAt(aIndex);

  UpdateListIndicesFromIndex(mItems, aIndex);

  Element()->DidChangePointList(emptyOrOldValue);
  if (AttrIsAnimating()) {
    Element()->AnimationNeedsResample();
  }
  return result.forget();
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

  animVal->mItems.InsertElementAt(aIndex, static_cast<nsISVGPoint*>(nullptr));

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
