





#include "DOMSVGTransform.h"
#include "DOMSVGMatrix.h"
#include "SVGAnimatedTransformList.h"
#include "nsError.h"
#include <math.h>
#include "nsContentUtils.h"
#include "nsAttrValueInlines.h"
#include "mozilla/dom/SVGTransformBinding.h"

namespace mozilla {








NS_IMPL_CYCLE_COLLECTION_CLASS(DOMSVGTransform)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(DOMSVGTransform)
  
  if (tmp->mList) {
    tmp->mList->mItems[tmp->mListIndex] = nullptr;
  }
NS_IMPL_CYCLE_COLLECTION_UNLINK(mList)
NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(DOMSVGTransform)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mList)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(DOMSVGTransform)
NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(DOMSVGTransform)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DOMSVGTransform)

} 
DOMCI_DATA(SVGTransform, mozilla::DOMSVGTransform)
namespace mozilla {

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMSVGTransform)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(mozilla::DOMSVGTransform)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGTransform)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMSVGTransform)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGTransform)
NS_INTERFACE_MAP_END


JSObject*
DOMSVGTransform::WrapObject(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap)
{
  return mozilla::dom::SVGTransformBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}




DOMSVGTransform::DOMSVGTransform(DOMSVGTransformList *aList,
                                 uint32_t aListIndex,
                                 bool aIsAnimValItem)
  : mList(aList)
  , mListIndex(aListIndex)
  , mIsAnimValItem(aIsAnimValItem)
  , mTransform(nullptr)
  , mMatrixTearoff(nullptr)
{
  SetIsDOMBinding();
  
  NS_ABORT_IF_FALSE(aList &&
                    aListIndex <= MaxListIndex(), "bad arg");

  NS_ABORT_IF_FALSE(IndexIsValid(), "Bad index for DOMSVGNumber!");
}

DOMSVGTransform::DOMSVGTransform()
  : mList(nullptr)
  , mListIndex(0)
  , mIsAnimValItem(false)
  , mTransform(new SVGTransform()) 
                                   
                                   
  , mMatrixTearoff(nullptr)
{
  SetIsDOMBinding();
}

DOMSVGTransform::DOMSVGTransform(const gfxMatrix &aMatrix)
  : mList(nullptr)
  , mListIndex(0)
  , mIsAnimValItem(false)
  , mTransform(new SVGTransform(aMatrix))
  , mMatrixTearoff(nullptr)
{
  SetIsDOMBinding();
}

DOMSVGTransform::DOMSVGTransform(const SVGTransform &aTransform)
  : mList(nullptr)
  , mListIndex(0)
  , mIsAnimValItem(false)
  , mTransform(new SVGTransform(aTransform))
  , mMatrixTearoff(nullptr)
{
  SetIsDOMBinding();
}






uint16_t
DOMSVGTransform::Type() const
{
  return Transform().Type();
}

NS_IMETHODIMP
DOMSVGTransform::GetType(uint16_t *aType)
{
  *aType = Type();
  return NS_OK;
}

already_AddRefed<DOMSVGMatrix>
DOMSVGTransform::Matrix()
{
  if (!mMatrixTearoff) {
    mMatrixTearoff = new DOMSVGMatrix(*this);
  }
  nsRefPtr<DOMSVGMatrix> matrix = mMatrixTearoff;
  return matrix.forget();
}


NS_IMETHODIMP
DOMSVGTransform::GetMatrix(nsIDOMSVGMatrix * *aMatrix)
{
  *aMatrix = Matrix().get();
  return NS_OK;
}


float
DOMSVGTransform::Angle() const
{
  return Transform().Angle();
}

NS_IMETHODIMP
DOMSVGTransform::GetAngle(float *aAngle)
{
  *aAngle = Angle();
  return NS_OK;
}

void
DOMSVGTransform::SetMatrix(DOMSVGMatrix& aMatrix, ErrorResult& rv)
{
  if (mIsAnimValItem) {
    rv.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return;
  }
  SetMatrix(aMatrix.Matrix());
}


NS_IMETHODIMP
DOMSVGTransform::SetMatrix(nsIDOMSVGMatrix *matrix)
{
  nsCOMPtr<DOMSVGMatrix> domMatrix = do_QueryInterface(matrix);
  if (!domMatrix)
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;

  ErrorResult rv;
  SetMatrix(*domMatrix, rv);
  return rv.ErrorCode();
}

void
DOMSVGTransform::SetTranslate(float tx, float ty, ErrorResult& rv)
{
  if (mIsAnimValItem) {
    rv.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  if (Transform().Type() == nsIDOMSVGTransform::SVG_TRANSFORM_TRANSLATE &&
      Matrixgfx().x0 == tx && Matrixgfx().y0 == ty) {
    return;
  }

  nsAttrValue emptyOrOldValue = NotifyElementWillChange();
  Transform().SetTranslate(tx, ty);
  NotifyElementDidChange(emptyOrOldValue);
}


NS_IMETHODIMP
DOMSVGTransform::SetTranslate(float tx, float ty)
{
  NS_ENSURE_FINITE2(tx, ty, NS_ERROR_ILLEGAL_VALUE);
  ErrorResult rv;
  SetTranslate(tx, ty, rv);
  return rv.ErrorCode();
}

void
DOMSVGTransform::SetScale(float sx, float sy, ErrorResult& rv)
{
  if (mIsAnimValItem) {
    rv.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  if (Transform().Type() == nsIDOMSVGTransform::SVG_TRANSFORM_SCALE &&
      Matrixgfx().xx == sx && Matrixgfx().yy == sy) {
    return;
  }
  nsAttrValue emptyOrOldValue = NotifyElementWillChange();
  Transform().SetScale(sx, sy);
  NotifyElementDidChange(emptyOrOldValue);
}


NS_IMETHODIMP
DOMSVGTransform::SetScale(float sx, float sy)
{
  NS_ENSURE_FINITE2(sx, sy, NS_ERROR_ILLEGAL_VALUE);
  ErrorResult rv;
  SetScale(sx, sy, rv);
  return rv.ErrorCode();
}

void
DOMSVGTransform::SetRotate(float angle, float cx, float cy, ErrorResult& rv)
{
  if (mIsAnimValItem) {
    rv.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  if (Transform().Type() == nsIDOMSVGTransform::SVG_TRANSFORM_ROTATE) {
    float currentCx, currentCy;
    Transform().GetRotationOrigin(currentCx, currentCy);
    if (Transform().Angle() == angle && currentCx == cx && currentCy == cy) {
      return;
    }
  }

  nsAttrValue emptyOrOldValue = NotifyElementWillChange();
  Transform().SetRotate(angle, cx, cy);
  NotifyElementDidChange(emptyOrOldValue);
}


NS_IMETHODIMP
DOMSVGTransform::SetRotate(float angle, float cx, float cy)
{
  NS_ENSURE_FINITE3(angle, cx, cy, NS_ERROR_ILLEGAL_VALUE);
  ErrorResult rv;
  SetRotate(angle, cx, cy, rv);
  return rv.ErrorCode();
}

void
DOMSVGTransform::SetSkewX(float angle, ErrorResult& rv)
{
  if (mIsAnimValItem) {
    rv.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  if (Transform().Type() == nsIDOMSVGTransform::SVG_TRANSFORM_SKEWX &&
      Transform().Angle() == angle) {
    return;
  }

  nsAttrValue emptyOrOldValue = NotifyElementWillChange();
  nsresult result = Transform().SetSkewX(angle);
  if (NS_FAILED(result)) {
    rv.Throw(result);
    return;
  }
  NotifyElementDidChange(emptyOrOldValue);
}


NS_IMETHODIMP
DOMSVGTransform::SetSkewX(float angle)
{
  NS_ENSURE_FINITE(angle, NS_ERROR_ILLEGAL_VALUE);
  ErrorResult rv;
  SetSkewX(angle, rv);
  return rv.ErrorCode();
}

void
DOMSVGTransform::SetSkewY(float angle, ErrorResult& rv)
{
  if (mIsAnimValItem) {
    rv.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  if (Transform().Type() == nsIDOMSVGTransform::SVG_TRANSFORM_SKEWY &&
      Transform().Angle() == angle) {
    return;
  }

  nsAttrValue emptyOrOldValue = NotifyElementWillChange();
  nsresult result = Transform().SetSkewY(angle);
  if (NS_FAILED(result)) {
    rv.Throw(result);
    return;
  }
  NotifyElementDidChange(emptyOrOldValue);
}


NS_IMETHODIMP
DOMSVGTransform::SetSkewY(float angle)
{
  NS_ENSURE_FINITE(angle, NS_ERROR_ILLEGAL_VALUE);
  ErrorResult rv;
  SetSkewY(angle, rv);
  return rv.ErrorCode();
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
      SVGTransform::MatricesEqual(Matrixgfx(), aMatrix)) {
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
