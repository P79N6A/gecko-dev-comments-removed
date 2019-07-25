



































#include "DOMSVGPoint.h"
#include "DOMSVGPointList.h"
#include "SVGPoint.h"
#include "SVGAnimatedPointList.h"
#include "nsSVGElement.h"
#include "nsIDOMSVGPoint.h"
#include "nsDOMError.h"
#include "nsIDOMSVGMatrix.h"



using namespace mozilla;





NS_IMPL_CYCLE_COLLECTION_CLASS(DOMSVGPoint)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(DOMSVGPoint)
  
  if (tmp->mList) {
    tmp->mList->mItems[tmp->mListIndex] = nsnull;
  }
NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mList)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(DOMSVGPoint)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mList)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(DOMSVGPoint)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DOMSVGPoint)

DOMCI_DATA(SVGPoint, DOMSVGPoint)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMSVGPoint)
  NS_INTERFACE_MAP_ENTRY(DOMSVGPoint) 
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGPoint)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGPoint)
NS_INTERFACE_MAP_END


NS_IMETHODIMP
DOMSVGPoint::GetX(float* aX)
{
#ifdef MOZ_SMIL
  if (mIsAnimValItem && HasOwner()) {
    Element()->FlushAnimations(); 
  }
#endif
  *aX = HasOwner() ? InternalItem().mX : mPt.mX;
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGPoint::SetX(float aX)
{
  if (mIsAnimValItem || mIsReadonly) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  NS_ENSURE_FINITE(aX, NS_ERROR_ILLEGAL_VALUE);

  if (HasOwner()) {
    InternalItem().mX = aX;
    Element()->DidChangePointList(PR_TRUE);
#ifdef MOZ_SMIL
    if (mList->AttrIsAnimating()) {
      Element()->AnimationNeedsResample();
    }
#endif
    return NS_OK;
  }
  mPt.mX = aX;
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGPoint::GetY(float* aY)
{
#ifdef MOZ_SMIL
  if (mIsAnimValItem && HasOwner()) {
    Element()->FlushAnimations(); 
  }
#endif
  *aY = HasOwner() ? InternalItem().mY : mPt.mY;
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGPoint::SetY(float aY)
{
  if (mIsAnimValItem || mIsReadonly) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  NS_ENSURE_FINITE(aY, NS_ERROR_ILLEGAL_VALUE);

  if (HasOwner()) {
    InternalItem().mY = aY;
    Element()->DidChangePointList(PR_TRUE);
#ifdef MOZ_SMIL
    if (mList->AttrIsAnimating()) {
      Element()->AnimationNeedsResample();
    }
#endif
    return NS_OK;
  }
  mPt.mY = aY;
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGPoint::MatrixTransform(nsIDOMSVGMatrix *matrix,
                             nsIDOMSVGPoint **_retval)
{
  if (!matrix)
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;

  float a, b, c, d, e, f;
  matrix->GetA(&a);
  matrix->GetB(&b);
  matrix->GetC(&c);
  matrix->GetD(&d);
  matrix->GetE(&e);
  matrix->GetF(&f);

  float x = HasOwner() ? InternalItem().mX : mPt.mX;
  float y = HasOwner() ? InternalItem().mY : mPt.mY;

  NS_ADDREF(*_retval = new DOMSVGPoint(a*x + c*y + e, b*x + d*y + f));
  return NS_OK;
}

void
DOMSVGPoint::InsertingIntoList(DOMSVGPointList *aList,
                               PRUint32 aListIndex,
                               PRBool aIsAnimValItem)
{
  NS_ABORT_IF_FALSE(!HasOwner(), "Inserting item that already has an owner");

  mList = aList;
  mListIndex = aListIndex;
  mIsReadonly = PR_FALSE;
  mIsAnimValItem = aIsAnimValItem;

  NS_ABORT_IF_FALSE(IndexIsValid(), "Bad index for DOMSVGPoint!");
}

void
DOMSVGPoint::RemovingFromList()
{
  mPt = InternalItem();
  mList = nsnull;
  NS_ABORT_IF_FALSE(!mIsReadonly, "mIsReadonly set for list");
  mIsAnimValItem = PR_FALSE;
}

SVGPoint&
DOMSVGPoint::InternalItem()
{
  return mList->InternalList().mItems[mListIndex];
}

#ifdef DEBUG
PRBool
DOMSVGPoint::IndexIsValid()
{
  return mListIndex < mList->InternalList().Length();
}
#endif

