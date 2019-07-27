





#include "SVGTransform.h"

#include "mozilla/dom/SVGTransform.h"
#include "mozilla/dom/SVGMatrix.h"
#include "mozilla/dom/SVGTransformBinding.h"
#include "nsError.h"
#include "nsSVGAnimatedTransformList.h"
#include "nsSVGAttrTearoffTable.h"
#include "mozilla/DebugOnly.h"

namespace {
  const double kRadPerDegree = 2.0 * M_PI / 360.0;
}

namespace mozilla {
namespace dom {

static nsSVGAttrTearoffTable<SVGTransform, SVGMatrix>&
SVGMatrixTearoffTable()
{
  static nsSVGAttrTearoffTable<SVGTransform, SVGMatrix> sSVGMatrixTearoffTable;
  return sSVGMatrixTearoffTable;
}







NS_IMPL_CYCLE_COLLECTION_CLASS(SVGTransform)

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
    SVGMatrixTearoffTable().GetTearoff(tmp);
  CycleCollectionNoteChild(cb, matrix, "matrix");
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(SVGTransform)
NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(SVGTransform, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(SVGTransform, Release)

JSObject*
SVGTransform::WrapObject(JSContext* aCx)
{
  return SVGTransformBinding::Wrap(aCx, this);
}





class MOZ_STACK_CLASS AutoChangeTransformNotifier
{
public:
  explicit AutoChangeTransformNotifier(SVGTransform* aTransform MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
    : mTransform(aTransform)
  {
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    MOZ_ASSERT(mTransform, "Expecting non-null transform");
    if (mTransform->HasOwner()) {
      mEmptyOrOldValue =
        mTransform->Element()->WillChangeTransformList();
    }
  }

  ~AutoChangeTransformNotifier()
  {
    if (mTransform->HasOwner()) {
      mTransform->Element()->DidChangeTransformList(mEmptyOrOldValue);
      if (mTransform->mList->IsAnimating()) {
        mTransform->Element()->AnimationNeedsResample();
      }
    }
  }

private:
  SVGTransform* const mTransform;
  nsAttrValue   mEmptyOrOldValue;
  MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};




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
  SVGMatrix* matrix = SVGMatrixTearoffTable().GetTearoff(this);
  if (matrix) {
    SVGMatrixTearoffTable().RemoveTearoff(this);
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
SVGTransform::GetMatrix()
{
  SVGMatrix* wrapper =
    SVGMatrixTearoffTable().GetTearoff(this);
  if (!wrapper) {
    NS_ADDREF(wrapper = new SVGMatrix(*this));
    SVGMatrixTearoffTable().AddTearoff(this, wrapper);
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
  SetMatrix(aMatrix.GetMatrix());
}

void
SVGTransform::SetTranslate(float tx, float ty, ErrorResult& rv)
{
  if (mIsAnimValItem) {
    rv.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  if (Transform().Type() == SVG_TRANSFORM_TRANSLATE &&
      Matrixgfx()._31 == tx && Matrixgfx()._32 == ty) {
    return;
  }

  AutoChangeTransformNotifier notifier(this);
  Transform().SetTranslate(tx, ty);
}

void
SVGTransform::SetScale(float sx, float sy, ErrorResult& rv)
{
  if (mIsAnimValItem) {
    rv.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  if (Transform().Type() == SVG_TRANSFORM_SCALE &&
      Matrixgfx()._11 == sx && Matrixgfx()._22 == sy) {
    return;
  }
  AutoChangeTransformNotifier notifier(this);
  Transform().SetScale(sx, sy);
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

  AutoChangeTransformNotifier notifier(this);
  Transform().SetRotate(angle, cx, cy);
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

  if (!NS_finite(tan(angle * kRadPerDegree))) {
    rv.Throw(NS_ERROR_RANGE_ERR);
    return;
  }

  AutoChangeTransformNotifier notifier(this);
  DebugOnly<nsresult> result = Transform().SetSkewX(angle);
  MOZ_ASSERT(NS_SUCCEEDED(result), "SetSkewX unexpectedly failed");
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

  if (!NS_finite(tan(angle * kRadPerDegree))) {
    rv.Throw(NS_ERROR_RANGE_ERR);
    return;
  }

  AutoChangeTransformNotifier notifier(this);
  DebugOnly<nsresult> result = Transform().SetSkewY(angle);
  MOZ_ASSERT(NS_SUCCEEDED(result), "SetSkewY unexpectedly failed");
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
  nsSVGAnimatedTransformList *alist = Element()->GetAnimatedTransformList();
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
  nsSVGAnimatedTransformList *alist = Element()->GetAnimatedTransformList();
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

  AutoChangeTransformNotifier notifier(this);
  Transform().SetMatrix(aMatrix);
}

} 
} 
