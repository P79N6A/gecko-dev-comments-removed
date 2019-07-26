




#include "mozilla/dom/SVGAnimatedTransformList.h"
#include "DOMSVGTransformList.h"
#include "nsSVGAnimatedTransformList.h"
#include "nsSVGAttrTearoffTable.h"
#include "mozilla/dom/SVGAnimatedTransformListBinding.h"

namespace mozilla {
namespace dom {

static
  nsSVGAttrTearoffTable<nsSVGAnimatedTransformList, SVGAnimatedTransformList>
  sSVGAnimatedTransformListTearoffTable;

NS_SVG_VAL_IMPL_CYCLE_COLLECTION_WRAPPERCACHED(SVGAnimatedTransformList, mElement)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(SVGAnimatedTransformList, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(SVGAnimatedTransformList, Release)

JSObject*
SVGAnimatedTransformList::WrapObject(JSContext* aCx)
{
  return SVGAnimatedTransformListBinding::Wrap(aCx, this);
}


already_AddRefed<DOMSVGTransformList>
SVGAnimatedTransformList::BaseVal()
{
  if (!mBaseVal) {
    mBaseVal = new DOMSVGTransformList(this, InternalAList().GetBaseValue());
  }
  nsRefPtr<DOMSVGTransformList> baseVal = mBaseVal;
  return baseVal.forget();
}

already_AddRefed<DOMSVGTransformList>
SVGAnimatedTransformList::AnimVal()
{
  if (!mAnimVal) {
    mAnimVal = new DOMSVGTransformList(this, InternalAList().GetAnimValue());
  }
  nsRefPtr<DOMSVGTransformList> animVal = mAnimVal;
  return animVal.forget();
}

 already_AddRefed<SVGAnimatedTransformList>
SVGAnimatedTransformList::GetDOMWrapper(nsSVGAnimatedTransformList *aList,
                                        nsSVGElement *aElement)
{
  nsRefPtr<SVGAnimatedTransformList> wrapper =
    sSVGAnimatedTransformListTearoffTable.GetTearoff(aList);
  if (!wrapper) {
    wrapper = new SVGAnimatedTransformList(aElement);
    sSVGAnimatedTransformListTearoffTable.AddTearoff(aList, wrapper);
  }
  return wrapper.forget();
}

 SVGAnimatedTransformList*
SVGAnimatedTransformList::GetDOMWrapperIfExists(
  nsSVGAnimatedTransformList *aList)
{
  return sSVGAnimatedTransformListTearoffTable.GetTearoff(aList);
}

SVGAnimatedTransformList::~SVGAnimatedTransformList()
{
  
  
  sSVGAnimatedTransformListTearoffTable.RemoveTearoff(&InternalAList());
}

void
SVGAnimatedTransformList::InternalBaseValListWillChangeLengthTo(
  uint32_t aNewLength)
{
  
  
  
  
  
  

  nsRefPtr<SVGAnimatedTransformList> kungFuDeathGrip;
  if (mBaseVal) {
    if (aNewLength < mBaseVal->LengthNoFlush()) {
      
      
      kungFuDeathGrip = this;
    }
    mBaseVal->InternalListLengthWillChange(aNewLength);
  }

  
  
  
  

  if (!IsAnimating()) {
    InternalAnimValListWillChangeLengthTo(aNewLength);
  }
}

void
SVGAnimatedTransformList::InternalAnimValListWillChangeLengthTo(
  uint32_t aNewLength)
{
  if (mAnimVal) {
    mAnimVal->InternalListLengthWillChange(aNewLength);
  }
}

bool
SVGAnimatedTransformList::IsAnimating() const
{
  return InternalAList().IsAnimating();
}

nsSVGAnimatedTransformList&
SVGAnimatedTransformList::InternalAList()
{
  return *mElement->GetAnimatedTransformList();
}

const nsSVGAnimatedTransformList&
SVGAnimatedTransformList::InternalAList() const
{
  return *mElement->GetAnimatedTransformList();
}

} 
} 
