




#include "DOMSVGAnimatedTransformList.h"
#include "DOMSVGTransformList.h"
#include "SVGAnimatedTransformList.h"
#include "nsSVGAttrTearoffTable.h"
#include "mozilla/dom/SVGAnimatedTransformListBinding.h"
#include "nsContentUtils.h"

namespace mozilla {

static
  nsSVGAttrTearoffTable<SVGAnimatedTransformList,DOMSVGAnimatedTransformList>
  sSVGAnimatedTransformListTearoffTable;

NS_SVG_VAL_IMPL_CYCLE_COLLECTION_WRAPPERCACHED(DOMSVGAnimatedTransformList, mElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(DOMSVGAnimatedTransformList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DOMSVGAnimatedTransformList)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMSVGAnimatedTransformList)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

JSObject*
DOMSVGAnimatedTransformList::WrapObject(JSContext* aCx, JSObject* aScope)
{
  return mozilla::dom::SVGAnimatedTransformListBinding::Wrap(aCx, aScope, this);
}


already_AddRefed<DOMSVGTransformList>
DOMSVGAnimatedTransformList::BaseVal()
{
  if (!mBaseVal) {
    mBaseVal = new DOMSVGTransformList(this, InternalAList().GetBaseValue());
  }
  nsRefPtr<DOMSVGTransformList> baseVal = mBaseVal;
  return baseVal.forget();
}

already_AddRefed<DOMSVGTransformList>
DOMSVGAnimatedTransformList::AnimVal()
{
  if (!mAnimVal) {
    mAnimVal = new DOMSVGTransformList(this, InternalAList().GetAnimValue());
  }
  nsRefPtr<DOMSVGTransformList> animVal = mAnimVal;
  return animVal.forget();
}

 already_AddRefed<DOMSVGAnimatedTransformList>
DOMSVGAnimatedTransformList::GetDOMWrapper(SVGAnimatedTransformList *aList,
                                           nsSVGElement *aElement)
{
  nsRefPtr<DOMSVGAnimatedTransformList> wrapper =
    sSVGAnimatedTransformListTearoffTable.GetTearoff(aList);
  if (!wrapper) {
    wrapper = new DOMSVGAnimatedTransformList(aElement);
    sSVGAnimatedTransformListTearoffTable.AddTearoff(aList, wrapper);
  }
  return wrapper.forget();
}

 DOMSVGAnimatedTransformList*
DOMSVGAnimatedTransformList::GetDOMWrapperIfExists(
  SVGAnimatedTransformList *aList)
{
  return sSVGAnimatedTransformListTearoffTable.GetTearoff(aList);
}

DOMSVGAnimatedTransformList::~DOMSVGAnimatedTransformList()
{
  
  
  sSVGAnimatedTransformListTearoffTable.RemoveTearoff(&InternalAList());
}

void
DOMSVGAnimatedTransformList::InternalBaseValListWillChangeLengthTo(
  uint32_t aNewLength)
{
  
  
  
  
  
  

  nsRefPtr<DOMSVGAnimatedTransformList> kungFuDeathGrip;
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
DOMSVGAnimatedTransformList::InternalAnimValListWillChangeLengthTo(
  uint32_t aNewLength)
{
  if (mAnimVal) {
    mAnimVal->InternalListLengthWillChange(aNewLength);
  }
}

bool
DOMSVGAnimatedTransformList::IsAnimating() const
{
  return InternalAList().IsAnimating();
}

SVGAnimatedTransformList&
DOMSVGAnimatedTransformList::InternalAList()
{
  return *mElement->GetAnimatedTransformList();
}

const SVGAnimatedTransformList&
DOMSVGAnimatedTransformList::InternalAList() const
{
  return *mElement->GetAnimatedTransformList();
}

} 
