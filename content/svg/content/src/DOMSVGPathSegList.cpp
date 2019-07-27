




#include "nsSVGElement.h"
#include "DOMSVGPathSegList.h"
#include "DOMSVGPathSeg.h"
#include "nsError.h"
#include "SVGAnimatedPathSegList.h"
#include "nsCOMPtr.h"
#include "nsSVGAttrTearoffTable.h"
#include "SVGPathSegUtils.h"
#include "mozilla/dom/SVGPathSegListBinding.h"



namespace mozilla {

  static inline
nsSVGAttrTearoffTable<void, DOMSVGPathSegList>&
SVGPathSegListTearoffTable()
{
  static nsSVGAttrTearoffTable<void, DOMSVGPathSegList>
    sSVGPathSegListTearoffTable;
  return sSVGPathSegListTearoffTable;
}

NS_IMPL_CYCLE_COLLECTION_CLASS(DOMSVGPathSegList)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(DOMSVGPathSegList)
  
  
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(DOMSVGPathSegList)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mElement)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(DOMSVGPathSegList)
  NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(DOMSVGPathSegList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DOMSVGPathSegList)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMSVGPathSegList)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END






class MOZ_STACK_CLASS AutoChangePathSegListNotifier
{
public:
  explicit AutoChangePathSegListNotifier(DOMSVGPathSegList* aPathSegList MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
    : mPathSegList(aPathSegList)
  {
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    MOZ_ASSERT(mPathSegList, "Expecting non-null pathSegList");
    mEmptyOrOldValue =
      mPathSegList->Element()->WillChangePathSegList();
  }

  ~AutoChangePathSegListNotifier()
  {
    mPathSegList->Element()->DidChangePathSegList(mEmptyOrOldValue);
    if (mPathSegList->AttrIsAnimating()) {
      mPathSegList->Element()->AnimationNeedsResample();
    }
  }

private:
  DOMSVGPathSegList* const mPathSegList;
  nsAttrValue        mEmptyOrOldValue;
  MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

 already_AddRefed<DOMSVGPathSegList>
DOMSVGPathSegList::GetDOMWrapper(void *aList,
                                 nsSVGElement *aElement,
                                 bool aIsAnimValList)
{
  nsRefPtr<DOMSVGPathSegList> wrapper =
    SVGPathSegListTearoffTable().GetTearoff(aList);
  if (!wrapper) {
    wrapper = new DOMSVGPathSegList(aElement, aIsAnimValList);
    SVGPathSegListTearoffTable().AddTearoff(aList, wrapper);
  }
  return wrapper.forget();
}

 DOMSVGPathSegList*
DOMSVGPathSegList::GetDOMWrapperIfExists(void *aList)
{
  return SVGPathSegListTearoffTable().GetTearoff(aList);
}

DOMSVGPathSegList::~DOMSVGPathSegList()
{
  
  
  void *key = mIsAnimValList ?
    InternalAList().GetAnimValKey() :
    InternalAList().GetBaseValKey();
  SVGPathSegListTearoffTable().RemoveTearoff(key);
}

JSObject*
DOMSVGPathSegList::WrapObject(JSContext *cx)
{
  return mozilla::dom::SVGPathSegListBinding::Wrap(cx, this);
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
        
        ErrorResult rv;
        Clear(rv);
        MOZ_ASSERT(!rv.Failed());
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




void
DOMSVGPathSegList::Clear(ErrorResult& aError)
{
  if (IsAnimValList()) {
    aError.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  if (LengthNoFlush() > 0) {
    AutoChangePathSegListNotifier notifier(this);
    
    
    

    InternalListWillChangeTo(SVGPathData()); 

    if (!AttrIsAnimating()) {
      
      DOMSVGPathSegList *animList =
        GetDOMWrapperIfExists(InternalAList().GetAnimValKey());
      if (animList) {
        animList->InternalListWillChangeTo(SVGPathData()); 
      }
    }

    InternalList().Clear();
  }
}

already_AddRefed<DOMSVGPathSeg>
DOMSVGPathSegList::Initialize(DOMSVGPathSeg& aNewItem, ErrorResult& aError)
{
  if (IsAnimValList()) {
    aError.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return nullptr;
  }

  
  
  
  
  
  
  

  nsRefPtr<DOMSVGPathSeg> domItem = &aNewItem;
  if (aNewItem.HasOwner()) {
    domItem = aNewItem.Clone();
  }

  Clear(aError);
  MOZ_ASSERT(!aError.Failed(), "How could this fail?");
  return InsertItemBefore(*domItem, 0, aError);
}

already_AddRefed<DOMSVGPathSeg>
DOMSVGPathSegList::GetItem(uint32_t index, ErrorResult& error)
{
  bool found;
  nsRefPtr<DOMSVGPathSeg> item = IndexedGetter(index, found, error);
  if (!found) {
    error.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
  }
  return item.forget();
}

already_AddRefed<DOMSVGPathSeg>
DOMSVGPathSegList::IndexedGetter(uint32_t aIndex, bool& aFound,
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

already_AddRefed<DOMSVGPathSeg>
DOMSVGPathSegList::InsertItemBefore(DOMSVGPathSeg& aNewItem,
                                    uint32_t aIndex,
                                    ErrorResult& aError)
{
  if (IsAnimValList()) {
    aError.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return nullptr;
  }

  uint32_t internalIndex;
  if (aIndex < LengthNoFlush()) {
    internalIndex = mItems[aIndex].mInternalDataIndex;
  } else {
    aIndex = LengthNoFlush();
    internalIndex = InternalList().mData.Length();
  }
  if (aIndex >= DOMSVGPathSeg::MaxListIndex()) {
    aError.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return nullptr;
  }

  nsRefPtr<DOMSVGPathSeg> domItem = &aNewItem;
  if (domItem->HasOwner()) {
    domItem = domItem->Clone(); 
  }

  uint32_t argCount = SVGPathSegUtils::ArgCountForType(domItem->Type());

  
  if (!mItems.SetCapacity(mItems.Length() + 1) ||
      !InternalList().mData.SetCapacity(InternalList().mData.Length() + 1 + argCount)) {
    aError.Throw(NS_ERROR_OUT_OF_MEMORY);
    return nullptr;
  }

  AutoChangePathSegListNotifier notifier(this);
  
  MaybeInsertNullInAnimValListAt(aIndex, internalIndex, argCount);

  float segAsRaw[1 + NS_SVG_PATH_SEG_MAX_ARGS];
  domItem->ToSVGPathSegEncodedData(segAsRaw);

  InternalList().mData.InsertElementsAt(internalIndex, segAsRaw, 1 + argCount);
  InternalList().mCachedPath = nullptr;
  mItems.InsertElementAt(aIndex, ItemProxy(domItem.get(), internalIndex));

  
  
  
  domItem->InsertingIntoList(this, aIndex, IsAnimValList());

  UpdateListIndicesFromIndex(aIndex + 1, argCount + 1);

  return domItem.forget();
}

already_AddRefed<DOMSVGPathSeg>
DOMSVGPathSegList::ReplaceItem(DOMSVGPathSeg& aNewItem,
                               uint32_t aIndex,
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

  nsRefPtr<DOMSVGPathSeg> domItem = &aNewItem;
  if (domItem->HasOwner()) {
    domItem = domItem->Clone(); 
  }

  AutoChangePathSegListNotifier notifier(this);
  if (ItemAt(aIndex)) {
    
    
    ItemAt(aIndex)->RemovingFromList();
  }

  uint32_t internalIndex = mItems[aIndex].mInternalDataIndex;
  
  
  uint32_t oldType = SVGPathSegUtils::DecodeType(InternalList().mData[internalIndex]);

  
  
  
  int32_t oldArgCount = SVGPathSegUtils::ArgCountForType(oldType);
  int32_t newArgCount = SVGPathSegUtils::ArgCountForType(domItem->Type());

  float segAsRaw[1 + NS_SVG_PATH_SEG_MAX_ARGS];
  domItem->ToSVGPathSegEncodedData(segAsRaw);

  bool ok = !!InternalList().mData.ReplaceElementsAt(
                  internalIndex, 1 + oldArgCount,
                  segAsRaw, 1 + newArgCount);
  InternalList().mCachedPath = nullptr;
  if (!ok) {
    aError.Throw(NS_ERROR_OUT_OF_MEMORY);
    return nullptr;
  }
  ItemAt(aIndex) = domItem;

  
  
  domItem->InsertingIntoList(this, aIndex, IsAnimValList());

  int32_t delta = newArgCount - oldArgCount;
  if (delta != 0) {
    for (uint32_t i = aIndex + 1; i < LengthNoFlush(); ++i) {
      mItems[i].mInternalDataIndex += delta;
    }
  }

  return domItem.forget();
}

already_AddRefed<DOMSVGPathSeg>
DOMSVGPathSegList::RemoveItem(uint32_t aIndex,
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
  
  nsRefPtr<DOMSVGPathSeg> result = GetItemAt(aIndex);

  AutoChangePathSegListNotifier notifier(this);
  
  
  ItemAt(aIndex)->RemovingFromList();

  uint32_t internalIndex = mItems[aIndex].mInternalDataIndex;
  uint32_t segType = SVGPathSegUtils::DecodeType(InternalList().mData[internalIndex]);
  
  
  
  int32_t argCount = SVGPathSegUtils::ArgCountForType(segType);

  
  
  
  MaybeRemoveItemFromAnimValListAt(aIndex, argCount);

  InternalList().mData.RemoveElementsAt(internalIndex, 1 + argCount);
  InternalList().mCachedPath = nullptr;
  mItems.RemoveElementAt(aIndex);

  UpdateListIndicesFromIndex(aIndex, -(argCount + 1));

  return result.forget();
}

already_AddRefed<DOMSVGPathSeg>
DOMSVGPathSegList::GetItemAt(uint32_t aIndex)
{
  MOZ_ASSERT(aIndex < mItems.Length());

  if (!ItemAt(aIndex)) {
    ItemAt(aIndex) = DOMSVGPathSeg::CreateFor(this, aIndex, IsAnimValList());
  }
  nsRefPtr<DOMSVGPathSeg> result = ItemAt(aIndex);
  return result.forget();
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
                                   int32_t aArgCountForItem)
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
