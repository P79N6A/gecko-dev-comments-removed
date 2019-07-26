





#include "mozilla/dom/SVGTransform.h"
#include "mozilla/dom/SVGMatrix.h"
#include "SVGAnimatedTransformList.h"
#include "nsError.h"
#include "nsContentUtils.h"
#include "nsAttrValueInlines.h"
#include "nsSVGAttrTearoffTable.h"
#include "mozilla/dom/SVGTransformBinding.h"

namespace mozilla {
namespace dom {

static nsSVGAttrTearoffTable<SVGTransform, SVGMatrix> sSVGMatrixTearoffTable;







NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(SVGTransform)
  
  if (tmp->mList) {
    tmp->mList->mItems[tmp->mListIndex] = nullptr;
  }
NS_IMPL_CYCLE_COLLECTION_UNLINK(mList)
NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(SVGTransform)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mList)
  SVGMatrix* matrix =
    sSVGMatrixTearoffTable.GetTearoff(tmp);
  CycleCollectionNoteChild(cb, matrix, "matrix");
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(SVGTransform)
NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(SVGTransform, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(SVGTransform, Release)

JSObject*
SVGTransform::WrapObject(JSContext* aCx, JSObject* aScope)
{
  return SVGTransformBinding::Wrap(aCx, aScope, this);
}




SVGTransform::SVGTransform(DOMSVGTransformList *aList,
                           uint32_t aListIndex,
                           bool aIsAnimValItem)
  : mList(aList)
  , mListIndex(aListIndex)
  , mIsAnimValItem(aIsAnimValItem)
  , mTransform(nullptr)
{
  SetIsDOMBinding();
  
  NS_ABORT_IF_FALSE(aList &&
                    aListIndex <= MaxListIndex(), "bad arg");

  NS_ABORT_IF_FALSE(IndexIsValid(), "Bad index for DOMSVGNumber!");
}

SVGTransform::SVGTransform()
  : mList(nullptr)
  , mListIndex(0)
  , mIsAnimValItem(false)
  , mTransform(new nsSVGTransform()) 
                                     
                                     
{
  SetIsDOMBinding();
}

SVGTransform::SVGTransform(const gfxMatrix &aMatrix)
  : mList(nullptr)
  , mListIndex(0)
  , mIsAnimValItem(false)
  , mTransform(new nsSVGTransform(aMatrix))
{
  SetIsDOMBinding();
}

SVGTransform::SVGTransform(const nsSVGTransform &aTransform)
  : mList(nullptr)
  , mListIndex(0)
  , mIsAnimValItem(false)
  , mTransform(new nsSVGTransform(aTransform))
{
  SetIsDOMBinding();
}

SVGTransform::~SVGTransform()
{
  SVGMatrix* matrix = sSVGMatrixTearoffTable.GetTearoff(this);
  if (matrix) {
    sSVGMatrixTearoffTable.RemoveTearoff(this);
    NS_RELEASE(matrix);
  }
  
  
  
  if (mList) {
    mList->mItems[mListIndex] = nullptr;
  }
}

uint16_t
SVGTransform::Type() const
{
  return Transform().Type();
}

SVGMatrix*
SVGTransform::Matrix()
{
  SVGMatrix* wrapper =
    sSVGMatrixTearoffTable.GetTearoff(this);
  if (!wrapper) {
    NS_ADDREF(wrapper = new SVGMatrix(*this));
    sSVGMatrixTearoffTable.AddTearoff(this, wrapper);
  }
  return wrapper;
}

float
SVGTransform::Angle() const
{
  return Transform().Angle();
}

void
SVGTransform::SetMatrix(SVGMatrix& aMatrix, ErrorResult& rv)
{
  if (mIsAnimValItem) {
    rv.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return;
  }
  SetMatrix(aMatrix.Matrix());
}

void
SVGTransform::SetTranslate(float tx, float ty, ErrorResult& rv)
{
  if (mIsAnimValItem) {
    rv.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  if (Transform().Type() == SVG_TRANSFORM_TRANSLATE &&
      Matrixgfx().x0 == tx && Matrixgfx().y0 == ty) {
    return;
  }

  nsAttrValue emptyOrOldValue = NotifyElementWillChange();
  Transform().SetTranslate(tx, ty);
  NotifyElementDidChange(emptyOrOldValue);
}

void
SVGTransform::SetScale(float sx, float sy, ErrorResult& rv)
{
  if (mIsAnimValItem) {
    rv.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  if (Transform().Type() == SVG_TRANSFORM_SCALE &&
      Matrixgfx().xx == sx && Matrixgfx().yy == sy) {
    return;
  }
  nsAttrValue emptyOrOldValue = NotifyElementWillChange();
  Transform().SetScale(sx, sy);
  NotifyElementDidChange(emptyOrOldValue);
}

void
SVGTransform::SetRotate(float angle, float cx, float cy, ErrorResult& rv)
{
  if (mIsAnimValItem) {
    rv.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  if (Transform().Type() == SVG_TRANSFORM_ROTATE) {
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

void
SVGTransform::SetSkewX(float angle, ErrorResult& rv)
{
  if (mIsAnimValItem) {
    rv.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  if (Transform().Type() == SVG_TRANSFORM_SKEWX &&
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

void
SVGTransform::SetSkewY(float angle, ErrorResult& rv)
{
  if (mIsAnimValItem) {
    rv.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  if (Transform().Type() == SVG_TRANSFORM_SKEWY &&
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




void
SVGTransform::InsertingIntoList(DOMSVGTransformList *aList,
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
SVGTransform::RemovingFromList()
{
  NS_ABORT_IF_FALSE(!mTransform,
      "Item in list also has another non-list value associated with it");

  mTransform = new nsSVGTransform(InternalItem());
  mList = nullptr;
  mIsAnimValItem = false;
}

nsSVGTransform&
SVGTransform::InternalItem()
{
  SVGAnimatedTransformList *alist = Element()->GetAnimatedTransformList();
  return mIsAnimValItem && alist->mAnimVal ?
    (*alist->mAnimVal)[mListIndex] :
    alist->mBaseVal[mListIndex];
}

const nsSVGTransform&
SVGTransform::InternalItem() const
{
  return const_cast<SVGTransform*>(this)->InternalItem();
}

#ifdef DEBUG
bool
SVGTransform::IndexIsValid()
{
  SVGAnimatedTransformList *alist = Element()->GetAnimatedTransformList();
  return (mIsAnimValItem &&
          mListIndex < alist->GetAnimValue().Length()) ||
         (!mIsAnimValItem &&
          mListIndex < alist->GetBaseValue().Length());
}
#endif 





void
SVGTransform::SetMatrix(const gfxMatrix& aMatrix)
{
  NS_ABORT_IF_FALSE(!mIsAnimValItem,
      "Attempting to modify read-only transform");

  if (Transform().Type() == SVG_TRANSFORM_MATRIX &&
      nsSVGTransform::MatricesEqual(Matrixgfx(), aMatrix)) {
    return;
  }

  nsAttrValue emptyOrOldValue = NotifyElementWillChange();
  Transform().SetMatrix(aMatrix);
  NotifyElementDidChange(emptyOrOldValue);
}




void
SVGTransform::NotifyElementDidChange(const nsAttrValue& aEmptyOrOldValue)
{
  if (HasOwner()) {
    Element()->DidChangeTransformList(aEmptyOrOldValue);
    if (mList->mAList->IsAnimating()) {
      Element()->AnimationNeedsResample();
    }
  }
}

} 
} 
