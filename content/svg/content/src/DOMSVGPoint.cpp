




#include "DOMSVGPoint.h"
#include "DOMSVGPointList.h"
#include "SVGPoint.h"
#include "SVGAnimatedPointList.h"
#include "nsSVGElement.h"
#include "nsIDOMSVGPoint.h"
#include "nsError.h"
#include "nsIDOMSVGMatrix.h"
#include "nsContentUtils.h" 
#include "DOMSVGMatrix.h"
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

DOMCI_DATA(SVGPoint, DOMSVGPoint)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMSVGPoint)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(DOMSVGPoint) 
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGPoint)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGPoint)
NS_INTERFACE_MAP_END

float
DOMSVGPoint::X()
{
  if (mIsAnimValItem && HasOwner()) {
    Element()->FlushAnimations(); 
  }
  return HasOwner() ? InternalItem().mX : mPt.mX;
}

NS_IMETHODIMP
DOMSVGPoint::GetX(float* aX)
{
  *aX = X();
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGPoint::SetX(float aX)
{
  if (!NS_finite(aX)) {
    return NS_ERROR_ILLEGAL_VALUE;
  }

  ErrorResult rv;
  SetX(aX, rv);
  return rv.ErrorCode();
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

NS_IMETHODIMP
DOMSVGPoint::GetY(float* aY)
{
  *aY = Y();
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGPoint::SetY(float aY)
{
  if (!NS_finite(aY)) {
    return NS_ERROR_ILLEGAL_VALUE;
  }

  ErrorResult rv;
  SetY(aY, rv);
  return rv.ErrorCode();
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

NS_IMETHODIMP
DOMSVGPoint::MatrixTransform(nsIDOMSVGMatrix *matrix,
                             nsIDOMSVGPoint **_retval)
{
  *_retval = MatrixTransform(matrix).get();
  return NS_OK;
}

already_AddRefed<DOMSVGPoint>
DOMSVGPoint::MatrixTransform(nsIDOMSVGMatrix* matrix)
{
  nsCOMPtr<DOMSVGMatrix> domMatrix = do_QueryInterface(matrix);

  float x = HasOwner() ? InternalItem().mX : mPt.mX;
  float y = HasOwner() ? InternalItem().mY : mPt.mY;

  gfxPoint pt = domMatrix->Matrix().Transform(gfxPoint(x, y));
  nsRefPtr<DOMSVGPoint> newPoint = new DOMSVGPoint(pt);
  return newPoint.forget();
}

JSObject*
DOMSVGPoint::WrapObject(JSContext *cx, JSObject *scope, bool *triedToWrap)
{
  return mozilla::dom::SVGPointBinding::Wrap(cx, scope, this, triedToWrap);
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

