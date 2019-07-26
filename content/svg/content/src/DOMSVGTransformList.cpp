




#include "DOMSVGTransformList.h"
#include "mozilla/dom/SVGTransform.h"
#include "mozilla/dom/SVGMatrix.h"
#include "nsSVGAnimatedTransformList.h"
#include "nsSVGElement.h"
#include "nsContentUtils.h"
#include "mozilla/dom/SVGTransformListBinding.h"
#include "nsError.h"
#include <algorithm>


namespace {

void UpdateListIndicesFromIndex(
  FallibleTArray<mozilla::dom::SVGTransform*>& aItemsArray,
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

using namespace dom;





NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(DOMSVGTransformList)
  if (tmp->mAList) {
    if (tmp->IsAnimValList()) {
      tmp->mAList->mAnimVal = nullptr;
    } else {
      tmp->mAList->mBaseVal = nullptr;
    }
    NS_IMPL_CYCLE_COLLECTION_UNLINK(mAList)
  }
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(DOMSVGTransformList)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mAList)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(DOMSVGTransformList)
  NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(DOMSVGTransformList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DOMSVGTransformList)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMSVGTransformList)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END




JSObject*
DOMSVGTransformList::WrapObject(JSContext *cx, JS::Handle<JSObject*> scope)
{
  return mozilla::dom::SVGTransformListBinding::Wrap(cx, scope, this);
}

void
DOMSVGTransformList::InternalListLengthWillChange(uint32_t aNewLength)
{
  uint32_t oldLength = mItems.Length();

  if (aNewLength > SVGTransform::MaxListIndex()) {
    
    
    aNewLength = SVGTransform::MaxListIndex();
  }

  nsRefPtr<DOMSVGTransformList> kungFuDeathGrip;
  if (aNewLength < oldLength) {
    
    
    kungFuDeathGrip = this;
  }

  
  for (uint32_t i = aNewLength; i < oldLength; ++i) {
    if (mItems[i]) {
      mItems[i]->RemovingFromList();
    }
  }

  if (!mItems.SetLength(aNewLength)) {
    
    
    mItems.Clear();
    return;
  }

  
  for (uint32_t i = oldLength; i < aNewLength; ++i) {
    mItems[i] = nullptr;
  }
}

SVGTransformList&
DOMSVGTransformList::InternalList() const
{
  nsSVGAnimatedTransformList *alist = Element()->GetAnimatedTransformList();
  return IsAnimValList() && alist->mAnimVal ?
    *alist->mAnimVal :
    alist->mBaseVal;
}


void
DOMSVGTransformList::Clear(ErrorResult& error)
{
  if (IsAnimValList()) {
    error.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  if (LengthNoFlush() > 0) {
    nsAttrValue emptyOrOldValue = Element()->WillChangeTransformList();
    
    
    
    mAList->InternalBaseValListWillChangeLengthTo(0);

    mItems.Clear();
    InternalList().Clear();
    Element()->DidChangeTransformList(emptyOrOldValue);
    if (mAList->IsAnimating()) {
      Element()->AnimationNeedsResample();
    }
  }
}

already_AddRefed<SVGTransform>
DOMSVGTransformList::Initialize(SVGTransform& newItem, ErrorResult& error)
{
  if (IsAnimValList()) {
    error.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return nullptr;
  }

  
  
  
  
  
  
  

  nsRefPtr<SVGTransform> domItem = &newItem;
  if (domItem->HasOwner()) {
    domItem = newItem.Clone();
  }

  Clear(error);
  MOZ_ASSERT(!error.Failed(), "How could this fail?");
  return InsertItemBefore(*domItem, 0, error);
}

SVGTransform*
DOMSVGTransformList::IndexedGetter(uint32_t index, bool& found,
                                   ErrorResult& error)
{
  if (IsAnimValList()) {
    Element()->FlushAnimations();
  }
  found = index < LengthNoFlush();
  if (found) {
    EnsureItemAt(index);
    return mItems[index];
  }
  return nullptr;
}

already_AddRefed<SVGTransform>
DOMSVGTransformList::InsertItemBefore(SVGTransform& newItem,
                                      uint32_t index, ErrorResult& error)
{
  if (IsAnimValList()) {
    error.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return nullptr;
  }

  index = std::min(index, LengthNoFlush());
  if (index >= SVGTransform::MaxListIndex()) {
    error.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return nullptr;
  }

  nsRefPtr<SVGTransform> domItem = &newItem;
  if (newItem.HasOwner()) {
    domItem = newItem.Clone(); 
  }

  
  if (!mItems.SetCapacity(mItems.Length() + 1) ||
      !InternalList().SetCapacity(InternalList().Length() + 1)) {
    error.Throw(NS_ERROR_OUT_OF_MEMORY);
    return nullptr;
  }

  nsAttrValue emptyOrOldValue = Element()->WillChangeTransformList();
  
  MaybeInsertNullInAnimValListAt(index);

  InternalList().InsertItem(index, domItem->ToSVGTransform());
  mItems.InsertElementAt(index, domItem.get());

  
  
  
  domItem->InsertingIntoList(this, index, IsAnimValList());

  UpdateListIndicesFromIndex(mItems, index + 1);

  Element()->DidChangeTransformList(emptyOrOldValue);
  if (mAList->IsAnimating()) {
    Element()->AnimationNeedsResample();
  }
  return domItem.forget();
}

already_AddRefed<SVGTransform>
DOMSVGTransformList::ReplaceItem(SVGTransform& newItem,
                                 uint32_t index, ErrorResult& error)
{
  if (IsAnimValList()) {
    error.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return nullptr;
  }

  if (index >= LengthNoFlush()) {
    error.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return nullptr;
  }

  nsRefPtr<SVGTransform> domItem = &newItem;
  if (newItem.HasOwner()) {
    domItem = newItem.Clone(); 
  }

  nsAttrValue emptyOrOldValue = Element()->WillChangeTransformList();
  if (mItems[index]) {
    
    
    mItems[index]->RemovingFromList();
  }

  InternalList()[index] = domItem->ToSVGTransform();
  mItems[index] = domItem;

  
  
  domItem->InsertingIntoList(this, index, IsAnimValList());

  Element()->DidChangeTransformList(emptyOrOldValue);
  if (mAList->IsAnimating()) {
    Element()->AnimationNeedsResample();
  }
  return domItem.forget();
}

already_AddRefed<SVGTransform>
DOMSVGTransformList::RemoveItem(uint32_t index, ErrorResult& error)
{
  if (IsAnimValList()) {
    error.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return nullptr;
  }

  if (index >= LengthNoFlush()) {
    error.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return nullptr;
  }

  nsAttrValue emptyOrOldValue = Element()->WillChangeTransformList();
  
  
  
  MaybeRemoveItemFromAnimValListAt(index);

  
  EnsureItemAt(index);

  
  
  mItems[index]->RemovingFromList();
  nsRefPtr<SVGTransform> result = mItems[index];

  InternalList().RemoveItem(index);
  mItems.RemoveElementAt(index);

  UpdateListIndicesFromIndex(mItems, index);

  Element()->DidChangeTransformList(emptyOrOldValue);
  if (mAList->IsAnimating()) {
    Element()->AnimationNeedsResample();
  }
  return result.forget();
}

already_AddRefed<SVGTransform>
DOMSVGTransformList::CreateSVGTransformFromMatrix(dom::SVGMatrix& matrix)
{
  nsRefPtr<SVGTransform> result = new SVGTransform(matrix.Matrix());
  return result.forget();
}

already_AddRefed<SVGTransform>
DOMSVGTransformList::Consolidate(ErrorResult& error)
{
  if (IsAnimValList()) {
    error.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return nullptr;
  }

  if (LengthNoFlush() == 0) {
    return nullptr;
  }

  
  
  
  

  
  gfxMatrix mx = InternalList().GetConsolidationMatrix();

  
  Clear(error);
  MOZ_ASSERT(!error.Failed(), "How could this fail?");

  
  nsRefPtr<SVGTransform> transform = new SVGTransform(mx);
  return InsertItemBefore(*transform, LengthNoFlush(), error);
}




void
DOMSVGTransformList::EnsureItemAt(uint32_t aIndex)
{
  if (!mItems[aIndex]) {
    mItems[aIndex] = new SVGTransform(this, aIndex, IsAnimValList());
  }
}

void
DOMSVGTransformList::MaybeInsertNullInAnimValListAt(uint32_t aIndex)
{
  NS_ABORT_IF_FALSE(!IsAnimValList(), "call from baseVal to animVal");

  DOMSVGTransformList* animVal = mAList->mAnimVal;

  if (!animVal || mAList->IsAnimating()) {
    
    return;
  }

  NS_ABORT_IF_FALSE(animVal->mItems.Length() == mItems.Length(),
                    "animVal list not in sync!");

  animVal->mItems.InsertElementAt(aIndex,
                                  static_cast<SVGTransform*>(nullptr));

  UpdateListIndicesFromIndex(animVal->mItems, aIndex + 1);
}

void
DOMSVGTransformList::MaybeRemoveItemFromAnimValListAt(uint32_t aIndex)
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
