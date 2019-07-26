




#include "DOMSVGPoint.h"
#include "DOMSVGPointList.h"
#include "SVGPoint.h"
#include "SVGAnimatedPointList.h"
#include "nsSVGElement.h"
#include "nsError.h"
#include "nsContentUtils.h" 
#include "mozilla/dom/SVGMatrix.h"
#include "mozilla/dom/SVGPointBinding.h"



using namespace mozilla;





NS_IMPL_CYCLE_COLLECTION_CLASS(DOMSVGPoint)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(DOMSVGPoint)
  
  if (tmp->mList) {
    tmp->mList->mItems[tmp->mListIndex] = nullptr;
  }
NS_IMPL_CYCLE_COLLECTION_UNLINK(mList)
NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(DOMSVGPoint)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mList)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(DOMSVGPoint)
NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(DOMSVGPoint)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DOMSVGPoint)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMSVGPoint)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(DOMSVGPoint) 
  NS_INTERFACE_MAP_ENTRY(nsISVGPoint)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

float
DOMSVGPoint::X()
{
  if (mIsAnimValItem && HasOwner()) {
    Element()->FlushAnimations(); 
  }
  return HasOwner() ? InternalItem().mX : mPt.mX;
}

void
DOMSVGPoint::SetX(float aX, ErrorResult& rv)
{
  if (mIsAnimValItem || mIsReadonly) {
    rv.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  if (HasOwner()) {
    if (InternalItem().mX == aX) {
      return;
    }
    nsAttrValue emptyOrOldValue = Element()->WillChangePointList();
    InternalItem().mX = aX;
    Element()->DidChangePointList(emptyOrOldValue);
    if (mList->AttrIsAnimating()) {
      Element()->AnimationNeedsResample();
    }
    return;
  }
  mPt.mX = aX;
}

float
DOMSVGPoint::Y()
{
  if (mIsAnimValItem && HasOwner()) {
    Element()->FlushAnimations(); 
  }
  return HasOwner() ? InternalItem().mY : mPt.mY;
}

void
DOMSVGPoint::SetY(float aY, ErrorResult& rv)
{
  if (mIsAnimValItem || mIsReadonly) {
    rv.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  if (HasOwner()) {
    if (InternalItem().mY == aY) {
      return;
    }
    nsAttrValue emptyOrOldValue = Element()->WillChangePointList();
    InternalItem().mY = aY;
    Element()->DidChangePointList(emptyOrOldValue);
    if (mList->AttrIsAnimating()) {
      Element()->AnimationNeedsResample();
    }
    return;
  }
  mPt.mY = aY;
}

already_AddRefed<nsISVGPoint>
DOMSVGPoint::MatrixTransform(dom::SVGMatrix& matrix)
{
  float x = HasOwner() ? InternalItem().mX : mPt.mX;
  float y = HasOwner() ? InternalItem().mY : mPt.mY;

  gfxPoint pt = matrix.Matrix().Transform(gfxPoint(x, y));
  nsCOMPtr<nsISVGPoint> newPoint = new DOMSVGPoint(pt);
  return newPoint.forget();
}

void
DOMSVGPoint::InsertingIntoList(DOMSVGPointList *aList,
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
DOMSVGPoint::RemovingFromList()
{
  mPt = InternalItem();
  mList = nullptr;
  NS_ABORT_IF_FALSE(!mIsReadonly, "mIsReadonly set for list");
  mIsAnimValItem = false;
}

SVGPoint&
DOMSVGPoint::InternalItem()
{
  return mList->InternalList().mItems[mListIndex];
}

#ifdef DEBUG
bool
DOMSVGPoint::IndexIsValid()
{
  return mListIndex < mList->InternalList().Length();
}
#endif

