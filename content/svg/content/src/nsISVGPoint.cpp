




#include "nsISVGPoint.h"
#include "DOMSVGPointList.h"
#include "SVGPoint.h"
#include "nsSVGElement.h"
#include "nsError.h"
#include "nsContentUtils.h" 
#include "mozilla/dom/SVGPointBinding.h"



using namespace mozilla;





NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsISVGPoint)
  
  if (tmp->mList) {
    tmp->mList->mItems[tmp->mListIndex] = nullptr;
  }
NS_IMPL_CYCLE_COLLECTION_UNLINK(mList)
NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsISVGPoint)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mList)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsISVGPoint)
NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsISVGPoint)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsISVGPoint)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsISVGPoint)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISVGPoint)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

void
nsISVGPoint::InsertingIntoList(DOMSVGPointList *aList,
                               uint32_t aListIndex,
                               bool aIsAnimValItem)
{
  NS_ABORT_IF_FALSE(!HasOwner(), "Inserting item that already has an owner");

  mList = aList;
  mListIndex = aListIndex;
  mIsReadonly = false;
  mIsAnimValItem = aIsAnimValItem;

  NS_ABORT_IF_FALSE(IndexIsValid(), "Bad index for DOMSVGPoint!");
}

void
nsISVGPoint::RemovingFromList()
{
  mPt = InternalItem();
  mList = nullptr;
  NS_ABORT_IF_FALSE(!mIsReadonly, "mIsReadonly set for list");
  mIsAnimValItem = false;
}

SVGPoint&
nsISVGPoint::InternalItem()
{
  return mList->InternalList().mItems[mListIndex];
}

#ifdef DEBUG
bool
nsISVGPoint::IndexIsValid()
{
  return mListIndex < mList->InternalList().Length();
}
#endif

