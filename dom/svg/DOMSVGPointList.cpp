




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
UpdateListIndicesFromIndex(FallibleTArray<mozilla::nsISVGPoint*>& aItemsArray,
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

  static inline
nsSVGAttrTearoffTable<void, DOMSVGPointList>&
SVGPointListTearoffTable()
{
  static nsSVGAttrTearoffTable<void, DOMSVGPointList>
    sSVGPointListTearoffTable;
  return sSVGPointListTearoffTable;
}

NS_IMPL_CYCLE_COLLECTION_CLASS(DOMSVGPointList)

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





class MOZ_STACK_CLASS AutoChangePointListNotifier
{
public:
  explicit AutoChangePointListNotifier(DOMSVGPointList* aPointList MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
    : mPointList(aPointList)
  {
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    MOZ_ASSERT(mPointList, "Expecting non-null pointList");
    mEmptyOrOldValue =
      mPointList->Element()->WillChangePointList();
  }

  ~AutoChangePointListNotifier()
  {
    mPointList->Element()->DidChangePointList(mEmptyOrOldValue);
    if (mPointList->AttrIsAnimating()) {
      mPointList->Element()->AnimationNeedsResample();
    }
  }

private:
  DOMSVGPointList* const mPointList;
  nsAttrValue      mEmptyOrOldValue;
  MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};


 already_AddRefed<DOMSVGPointList>
DOMSVGPointList::GetDOMWrapper(void *aList,
                               nsSVGElement *aElement,
                               bool aIsAnimValList)
{
  nsRefPtr<DOMSVGPointList> wrapper =
    SVGPointListTearoffTable().GetTearoff(aList);
  if (!wrapper) {
    wrapper = new DOMSVGPointList(aElement, aIsAnimValList);
    SVGPointListTearoffTable().AddTearoff(aList, wrapper);
  }
  return wrapper.forget();
}

 DOMSVGPointList*
DOMSVGPointList::GetDOMWrapperIfExists(void *aList)
{
  return SVGPointListTearoffTable().GetTearoff(aList);
}

DOMSVGPointList::~DOMSVGPointList()
{
  
  
  void *key = mIsAnimValList ?
    InternalAList().GetAnimValKey() :
    InternalAList().GetBaseValKey();
  SVGPointListTearoffTable().RemoveTearoff(key);
}

JSObject*
DOMSVGPointList::WrapObject(JSContext *cx)
{
  return mozilla::dom::SVGPointListBinding::Wrap(cx, this);
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
    AutoChangePointListNotifier notifier(this);
    
    
    

    InternalListWillChangeTo(SVGPointList()); 

    if (!AttrIsAnimating()) {
      
      DOMSVGPointList *animList =
        GetDOMWrapperIfExists(InternalAList().GetAnimValKey());
      if (animList) {
        animList->InternalListWillChangeTo(SVGPointList()); 
      }
    }

    InternalList().Clear();
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
  if (domItem->HasOwner() || domItem->IsReadonly() ||
      domItem->IsTranslatePoint()) {
    domItem = domItem->Copy(); 
  }

  ErrorResult rv;
  Clear(rv);
  MOZ_ASSERT(!rv.Failed());
  return InsertItemBefore(*domItem, 0, aError);
}

already_AddRefed<nsISVGPoint>
DOMSVGPointList::GetItem(uint32_t index, ErrorResult& error)
{
  bool found;
  nsRefPtr<nsISVGPoint> item = IndexedGetter(index, found, error);
  if (!found) {
    error.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
  }
  return item.forget();
}

already_AddRefed<nsISVGPoint>
DOMSVGPointList::IndexedGetter(uint32_t aIndex, bool& aFound,
                               ErrorResult& aError)
{
  if (IsAnimValList()) {
    Element()->FlushAnimations();
  }
  aFound = aIndex < LengthNoFlush();
  if (aFound) {
    return GetItemAt(aIndex);
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
  if (domItem->HasOwner() || domItem->IsReadonly() ||
      domItem->IsTranslatePoint()) {
    domItem = domItem->Copy(); 
  }

  
  if (!mItems.SetCapacity(mItems.Length() + 1) ||
      !InternalList().SetCapacity(InternalList().Length() + 1)) {
    aError.Throw(NS_ERROR_OUT_OF_MEMORY);
    return nullptr;
  }

  AutoChangePointListNotifier notifier(this);
  
  MaybeInsertNullInAnimValListAt(aIndex);

  InternalList().InsertItem(aIndex, domItem->ToSVGPoint());
  mItems.InsertElementAt(aIndex, domItem);

  
  
  
  domItem->InsertingIntoList(this, aIndex, IsAnimValList());

  UpdateListIndicesFromIndex(mItems, aIndex + 1);

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
  if (domItem->HasOwner() || domItem->IsReadonly() ||
      domItem->IsTranslatePoint()) {
    domItem = domItem->Copy(); 
  }

  AutoChangePointListNotifier notifier(this);
  if (mItems[aIndex]) {
    
    
    mItems[aIndex]->RemovingFromList();
  }

  InternalList()[aIndex] = domItem->ToSVGPoint();
  mItems[aIndex] = domItem;

  
  
  domItem->InsertingIntoList(this, aIndex, IsAnimValList());

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

  AutoChangePointListNotifier notifier(this);
  
  
  
  MaybeRemoveItemFromAnimValListAt(aIndex);

  
  nsRefPtr<nsISVGPoint> result = GetItemAt(aIndex);

  
  
  mItems[aIndex]->RemovingFromList();

  InternalList().RemoveItem(aIndex);
  mItems.RemoveElementAt(aIndex);

  UpdateListIndicesFromIndex(mItems, aIndex);

  return result.forget();
}

already_AddRefed<nsISVGPoint>
DOMSVGPointList::GetItemAt(uint32_t aIndex)
{
  MOZ_ASSERT(aIndex < mItems.Length());

  if (!mItems[aIndex]) {
    mItems[aIndex] = new DOMSVGPoint(this, aIndex, IsAnimValList());
  }
  nsRefPtr<nsISVGPoint> result = mItems[aIndex];
  return result.forget();
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
