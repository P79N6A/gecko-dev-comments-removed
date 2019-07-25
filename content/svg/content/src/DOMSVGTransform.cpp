





#include "DOMSVGTransform.h"
#include "DOMSVGMatrix.h"
#include "SVGAnimatedTransformList.h"
#include "nsError.h"
#include <math.h>
#include "nsContentUtils.h"

namespace mozilla {








NS_IMPL_CYCLE_COLLECTION_CLASS(DOMSVGTransform)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(DOMSVGTransform)
  
  if (tmp->mList) {
    tmp->mList->mItems[tmp->mListIndex] = nullptr;
  }
NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mList)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(DOMSVGTransform)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mList)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(DOMSVGTransform)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DOMSVGTransform)

} 
DOMCI_DATA(SVGTransform, mozilla::DOMSVGTransform)
namespace mozilla {

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMSVGTransform)
  NS_INTERFACE_MAP_ENTRY(mozilla::DOMSVGTransform)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGTransform)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMSVGTransform)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGTransform)
NS_INTERFACE_MAP_END





DOMSVGTransform::DOMSVGTransform(DOMSVGTransformList *aList,
                                 uint32_t aListIndex,
                                 bool aIsAnimValItem)
  : mList(aList)
  , mListIndex(aListIndex)
  , mIsAnimValItem(aIsAnimValItem)
  , mTransform(nullptr)
  , mMatrixTearoff(nullptr)
{
  
  NS_ABORT_IF_FALSE(aList &&
                    aListIndex <= MaxListIndex() &&
                    aIsAnimValItem < (1 << 1), "bad arg");

  NS_ABORT_IF_FALSE(IndexIsValid(), "Bad index for DOMSVGNumber!");
}

DOMSVGTransform::DOMSVGTransform()
  : mList(nullptr)
  , mListIndex(0)
  , mIsAnimValItem(false)
  , mTransform(new SVGTransform()) 
                                   
                                   
  , mMatrixTearoff(nullptr)
{
}

DOMSVGTransform::DOMSVGTransform(const gfxMatrix &aMatrix)
  : mList(nullptr)
  , mListIndex(0)
  , mIsAnimValItem(false)
  , mTransform(new SVGTransform(aMatrix))
  , mMatrixTearoff(nullptr)
{
}

DOMSVGTransform::DOMSVGTransform(const SVGTransform &aTransform)
  : mList(nullptr)
  , mListIndex(0)
  , mIsAnimValItem(false)
  , mTransform(new SVGTransform(aTransform))
  , mMatrixTearoff(nullptr)
{
}






NS_IMETHODIMP
DOMSVGTransform::GetType(uint16_t *aType)
{
  *aType = Transform().Type();
  return NS_OK;
}


NS_IMETHODIMP
DOMSVGTransform::GetMatrix(nsIDOMSVGMatrix * *aMatrix)
{
  if (!mMatrixTearoff) {
    mMatrixTearoff = new DOMSVGMatrix(*this);
  }

  NS_ADDREF(*aMatrix = mMatrixTearoff);
  return NS_OK;
}


NS_IMETHODIMP
DOMSVGTransform::GetAngle(float *aAngle)
{
  *aAngle = Transform().Angle();
  return NS_OK;
}


NS_IMETHODIMP
DOMSVGTransform::SetMatrix(nsIDOMSVGMatrix *matrix)
{
  if (mIsAnimValItem)
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;

  nsCOMPtr<DOMSVGMatrix> domMatrix = do_QueryInterface(matrix);
  if (!domMatrix)
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;

  SetMatrix(domMatrix->Matrix());
  return NS_OK;
}


NS_IMETHODIMP
DOMSVGTransform::SetTranslate(float tx, float ty)
{
  if (mIsAnimValItem) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }
  NS_ENSURE_FINITE2(tx, ty, NS_ERROR_ILLEGAL_VALUE);

  if (Transform().Type() == nsIDOMSVGTransform::SVG_TRANSFORM_TRANSLATE &&
      Matrix().x0 == tx && Matrix().y0 == ty) {
    return NS_OK;
  }

  nsAttrValue emptyOrOldValue = NotifyElementWillChange();
  Transform().SetTranslate(tx, ty);
  NotifyElementDidChange(emptyOrOldValue);

  return NS_OK;
}


NS_IMETHODIMP
DOMSVGTransform::SetScale(float sx, float sy)
{
  if (mIsAnimValItem) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }
  NS_ENSURE_FINITE2(sx, sy, NS_ERROR_ILLEGAL_VALUE);

  if (Transform().Type() == nsIDOMSVGTransform::SVG_TRANSFORM_SCALE &&
      Matrix().xx == sx && Matrix().yy == sy) {
    return NS_OK;
  }

  nsAttrValue emptyOrOldValue = NotifyElementWillChange();
  Transform().SetScale(sx, sy);
  NotifyElementDidChange(emptyOrOldValue);

  return NS_OK;
}


NS_IMETHODIMP
DOMSVGTransform::SetRotate(float angle, float cx, float cy)
{
  if (mIsAnimValItem) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }
  NS_ENSURE_FINITE3(angle, cx, cy, NS_ERROR_ILLEGAL_VALUE);

  if (Transform().Type() == nsIDOMSVGTransform::SVG_TRANSFORM_ROTATE) {
    float currentCx, currentCy;
    Transform().GetRotationOrigin(currentCx, currentCy);
    if (Transform().Angle() == angle && currentCx == cx && currentCy == cy) {
      return NS_OK;
    }
  }

  nsAttrValue emptyOrOldValue = NotifyElementWillChange();
  Transform().SetRotate(angle, cx, cy);
  NotifyElementDidChange(emptyOrOldValue);

  return NS_OK;
}


NS_IMETHODIMP
DOMSVGTransform::SetSkewX(float angle)
{
  if (mIsAnimValItem) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }
  NS_ENSURE_FINITE(angle, NS_ERROR_ILLEGAL_VALUE);

  if (Transform().Type() == nsIDOMSVGTransform::SVG_TRANSFORM_SKEWX &&
      Transform().Angle() == angle) {
    return NS_OK;
  }

  nsAttrValue emptyOrOldValue = NotifyElementWillChange();
  nsresult rv = Transform().SetSkewX(angle);
  if (NS_FAILED(rv))
    return rv;
  NotifyElementDidChange(emptyOrOldValue);

  return NS_OK;
}


NS_IMETHODIMP
DOMSVGTransform::SetSkewY(float angle)
{
  if (mIsAnimValItem) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }
  NS_ENSURE_FINITE(angle, NS_ERROR_ILLEGAL_VALUE);

  if (Transform().Type() == nsIDOMSVGTransform::SVG_TRANSFORM_SKEWY &&
      Transform().Angle() == angle) {
    return NS_OK;
  }

  nsAttrValue emptyOrOldValue = NotifyElementWillChange();
  nsresult rv = Transform().SetSkewY(angle);
  if (NS_FAILED(rv))
    return rv;
  NotifyElementDidChange(emptyOrOldValue);

  return NS_OK;
}





void
DOMSVGTransform::InsertingIntoList(DOMSVGTransformList *aList,
                                   uint32_t aListIndex,
                                   bool aIsAnimValItem)
{
  NS_ABORT_IF_FALSE(!HasOwner(), "Inserting item that is already in a list");

  mList = aList;
  mListIndex = aListIndex;
  mIsAnimValItem = aIsAnimValItem;
  mTransform = nullptr;

  NS_ABORT_IF_FALSE(IndexIsValid(), "Bad index for DOMSVGLength!");
}

void
DOMSVGTransform::RemovingFromList()
{
  NS_ABORT_IF_FALSE(!mTransform,
      "Item in list also has another non-list value associated with it");

  mTransform = new SVGTransform(InternalItem());
  mList = nullptr;
  mIsAnimValItem = false;
}

SVGTransform&
DOMSVGTransform::InternalItem()
{
  SVGAnimatedTransformList *alist = Element()->GetAnimatedTransformList();
  return mIsAnimValItem && alist->mAnimVal ?
    (*alist->mAnimVal)[mListIndex] :
    alist->mBaseVal[mListIndex];
}

const SVGTransform&
DOMSVGTransform::InternalItem() const
{
  return const_cast<DOMSVGTransform *>(this)->InternalItem();
}

#ifdef DEBUG
bool
DOMSVGTransform::IndexIsValid()
{
  SVGAnimatedTransformList *alist = Element()->GetAnimatedTransformList();
  return (mIsAnimValItem &&
          mListIndex < alist->GetAnimValue().Length()) ||
         (!mIsAnimValItem &&
          mListIndex < alist->GetBaseValue().Length());
}
#endif 





void
DOMSVGTransform::SetMatrix(const gfxMatrix& aMatrix)
{
  NS_ABORT_IF_FALSE(!mIsAnimValItem,
      "Attempting to modify read-only transform");

  if (Transform().Type() == nsIDOMSVGTransform::SVG_TRANSFORM_MATRIX &&
      SVGTransform::MatricesEqual(Matrix(), aMatrix)) {
    return;
  }

  nsAttrValue emptyOrOldValue = NotifyElementWillChange();
  Transform().SetMatrix(aMatrix);
  NotifyElementDidChange(emptyOrOldValue);
}

void
DOMSVGTransform::ClearMatrixTearoff(DOMSVGMatrix* aMatrix)
{
  NS_ABORT_IF_FALSE(mMatrixTearoff == aMatrix,
      "Unexpected matrix pointer to be cleared");
  mMatrixTearoff = nullptr;
}





void
DOMSVGTransform::NotifyElementDidChange(const nsAttrValue& aEmptyOrOldValue)
{
  if (HasOwner()) {
    Element()->DidChangeTransformList(aEmptyOrOldValue);
    if (mList->mAList->IsAnimating()) {
      Element()->AnimationNeedsResample();
    }
  }
}

} 
