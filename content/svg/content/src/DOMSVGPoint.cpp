




#include "DOMSVGPoint.h"
#include "DOMSVGPointList.h"
#include "SVGPoint.h"
#include "nsSVGElement.h"
#include "nsError.h"
#include "nsContentUtils.h" 
#include "mozilla/dom/SVGMatrix.h"



using namespace mozilla;

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
