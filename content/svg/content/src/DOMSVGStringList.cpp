




#include "DOMSVGStringList.h"

#include "mozilla/dom/SVGStringListBinding.h"
#include "mozilla/dom/SVGTests.h"
#include "nsError.h"
#include "nsCOMPtr.h"
#include "nsSVGAttrTearoffTable.h"
#include <algorithm>



namespace mozilla {

using namespace dom;

static nsSVGAttrTearoffTable<SVGStringList, DOMSVGStringList>
  sSVGStringListTearoffTable;

NS_SVG_VAL_IMPL_CYCLE_COLLECTION_WRAPPERCACHED(DOMSVGStringList, mElement)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(DOMSVGStringList, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(DOMSVGStringList, Release)


 already_AddRefed<DOMSVGStringList>
DOMSVGStringList::GetDOMWrapper(SVGStringList *aList,
                                nsSVGElement *aElement,
                                bool aIsConditionalProcessingAttribute,
                                uint8_t aAttrEnum)
{
  nsRefPtr<DOMSVGStringList> wrapper =
    sSVGStringListTearoffTable.GetTearoff(aList);
  if (!wrapper) {
    wrapper = new DOMSVGStringList(aElement, 
                                   aIsConditionalProcessingAttribute,
                                   aAttrEnum);
    sSVGStringListTearoffTable.AddTearoff(aList, wrapper);
  }
  return wrapper.forget();
}

DOMSVGStringList::~DOMSVGStringList()
{
  
  sSVGStringListTearoffTable.RemoveTearoff(&InternalList());
}

 JSObject*
DOMSVGStringList::WrapObject(JSContext* aCx, JSObject* aScope)
{
  return SVGStringListBinding::Wrap(aCx, aScope, this);
}




uint32_t
DOMSVGStringList::NumberOfItems() const
{
  return InternalList().Length();
}

uint32_t
DOMSVGStringList::Length() const
{
  return NumberOfItems();
}

void
DOMSVGStringList::Clear()
{
  if (InternalList().IsExplicitlySet()) {
    nsAttrValue emptyOrOldValue =
      mElement->WillChangeStringList(mIsConditionalProcessingAttribute,
                                     mAttrEnum);
    InternalList().Clear();
    mElement->DidChangeStringList(mIsConditionalProcessingAttribute,
                                  mAttrEnum, emptyOrOldValue);
  }
}

void
DOMSVGStringList::Initialize(const nsAString& aNewItem, nsAString& aRetval,
                             ErrorResult& aRv)
{
  if (InternalList().IsExplicitlySet()) {
    InternalList().Clear();
  }
  InsertItemBefore(aNewItem, 0, aRetval, aRv);
}

void
DOMSVGStringList::GetItem(uint32_t aIndex, nsAString& aRetval, ErrorResult& aRv)
{
  bool found;
  IndexedGetter(aIndex, found, aRetval);
  if (!found) {
    aRv.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
  }
}

void
DOMSVGStringList::IndexedGetter(uint32_t aIndex, bool& aFound,
                                nsAString& aRetval)
{
  aFound = aIndex < InternalList().Length();
  if (aFound) {
    aRetval = InternalList()[aIndex];
  }
}

void
DOMSVGStringList::InsertItemBefore(const nsAString& aNewItem, uint32_t aIndex,
                                   nsAString& aRetval, ErrorResult& aRv)
{
  if (aNewItem.IsEmpty()) {
    aRv.Throw(NS_ERROR_DOM_SYNTAX_ERR);
    return;
  }
  aIndex = std::min(aIndex, InternalList().Length());

  
  if (!InternalList().SetCapacity(InternalList().Length() + 1)) {
    aRv.Throw(NS_ERROR_OUT_OF_MEMORY);
    return;
  }

  nsAttrValue emptyOrOldValue =
    mElement->WillChangeStringList(mIsConditionalProcessingAttribute,
                                   mAttrEnum);
  InternalList().InsertItem(aIndex, aNewItem);

  mElement->DidChangeStringList(mIsConditionalProcessingAttribute, mAttrEnum,
                                emptyOrOldValue);
  aRetval = aNewItem;
}

void
DOMSVGStringList::ReplaceItem(const nsAString& aNewItem, uint32_t aIndex,
                              nsAString& aRetval, ErrorResult& aRv)
{
  if (aNewItem.IsEmpty()) {
    aRv.Throw(NS_ERROR_DOM_SYNTAX_ERR);
    return;
  }
  if (aIndex >= InternalList().Length()) {
    aRv.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return;
  }

  aRetval = InternalList()[aIndex];
  nsAttrValue emptyOrOldValue =
    mElement->WillChangeStringList(mIsConditionalProcessingAttribute,
                                   mAttrEnum);
  InternalList().ReplaceItem(aIndex, aNewItem);

  mElement->DidChangeStringList(mIsConditionalProcessingAttribute, mAttrEnum,
                                emptyOrOldValue);
}

void
DOMSVGStringList::RemoveItem(uint32_t aIndex, nsAString& aRetval,
                             ErrorResult& aRv)
{
  if (aIndex >= InternalList().Length()) {
    aRv.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return;
  }

  nsAttrValue emptyOrOldValue =
    mElement->WillChangeStringList(mIsConditionalProcessingAttribute,
                                   mAttrEnum);
  InternalList().RemoveItem(aIndex);

  mElement->DidChangeStringList(mIsConditionalProcessingAttribute, mAttrEnum,
                                emptyOrOldValue);
}

void
DOMSVGStringList::AppendItem(const nsAString& aNewItem, nsAString& aRetval,
                             ErrorResult& aRv)
{
  InsertItemBefore(aNewItem, InternalList().Length(), aRetval, aRv);
}

SVGStringList &
DOMSVGStringList::InternalList() const
{
  if (mIsConditionalProcessingAttribute) {
    nsCOMPtr<dom::SVGTests> tests = do_QueryObject(mElement.get());
    return tests->mStringListAttributes[mAttrEnum];
  }
  return mElement->GetStringListInfo().mStringLists[mAttrEnum];
}

} 
