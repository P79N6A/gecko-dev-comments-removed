




#include "DOMSVGAnimatedLengthList.h"
#include "DOMSVGLengthList.h"
#include "SVGAnimatedLengthList.h"
#include "nsSVGElement.h"
#include "nsCOMPtr.h"
#include "nsSVGAttrTearoffTable.h"
#include "mozilla/dom/SVGAnimatedLengthListBinding.h"



namespace mozilla {

static inline
nsSVGAttrTearoffTable<SVGAnimatedLengthList, DOMSVGAnimatedLengthList>&
SVGAnimatedLengthListTearoffTable()
{
  static nsSVGAttrTearoffTable<SVGAnimatedLengthList, DOMSVGAnimatedLengthList>
    sSVGAnimatedLengthListTearoffTable;
  return sSVGAnimatedLengthListTearoffTable;
}

NS_SVG_VAL_IMPL_CYCLE_COLLECTION_WRAPPERCACHED(DOMSVGAnimatedLengthList, mElement)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(DOMSVGAnimatedLengthList, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(DOMSVGAnimatedLengthList, Release)

JSObject*
DOMSVGAnimatedLengthList::WrapObject(JSContext* aCx)
{
  return dom::SVGAnimatedLengthListBinding::Wrap(aCx, this);
}

already_AddRefed<DOMSVGLengthList>
DOMSVGAnimatedLengthList::BaseVal()
{
  if (!mBaseVal) {
    mBaseVal = new DOMSVGLengthList(this, InternalAList().GetBaseValue());
  }
  nsRefPtr<DOMSVGLengthList> baseVal = mBaseVal;
  return baseVal.forget();
}

already_AddRefed<DOMSVGLengthList>
DOMSVGAnimatedLengthList::AnimVal()
{
  if (!mAnimVal) {
    mAnimVal = new DOMSVGLengthList(this, InternalAList().GetAnimValue());
  }
  nsRefPtr<DOMSVGLengthList> animVal = mAnimVal;
  return animVal.forget();
}

 already_AddRefed<DOMSVGAnimatedLengthList>
DOMSVGAnimatedLengthList::GetDOMWrapper(SVGAnimatedLengthList *aList,
                                        nsSVGElement *aElement,
                                        uint8_t aAttrEnum,
                                        uint8_t aAxis)
{
  nsRefPtr<DOMSVGAnimatedLengthList> wrapper =
    SVGAnimatedLengthListTearoffTable().GetTearoff(aList);
  if (!wrapper) {
    wrapper = new DOMSVGAnimatedLengthList(aElement, aAttrEnum, aAxis);
    SVGAnimatedLengthListTearoffTable().AddTearoff(aList, wrapper);
  }
  return wrapper.forget();
}

 DOMSVGAnimatedLengthList*
DOMSVGAnimatedLengthList::GetDOMWrapperIfExists(SVGAnimatedLengthList *aList)
{
  return SVGAnimatedLengthListTearoffTable().GetTearoff(aList);
}

DOMSVGAnimatedLengthList::~DOMSVGAnimatedLengthList()
{
  
  
  SVGAnimatedLengthListTearoffTable().RemoveTearoff(&InternalAList());
}

void
DOMSVGAnimatedLengthList::InternalBaseValListWillChangeTo(const SVGLengthList& aNewValue)
{
  
  
  
  
  
  

  nsRefPtr<DOMSVGAnimatedLengthList> kungFuDeathGrip;
  if (mBaseVal) {
    if (aNewValue.Length() < mBaseVal->LengthNoFlush()) {
      
      
      kungFuDeathGrip = this;
    }
    mBaseVal->InternalListLengthWillChange(aNewValue.Length());
  }

  
  
  
  

  if (!IsAnimating()) {
    InternalAnimValListWillChangeTo(aNewValue);
  }
}

void
DOMSVGAnimatedLengthList::InternalAnimValListWillChangeTo(const SVGLengthList& aNewValue)
{
  if (mAnimVal) {
    mAnimVal->InternalListLengthWillChange(aNewValue.Length());
  }
}

bool
DOMSVGAnimatedLengthList::IsAnimating() const
{
  return InternalAList().IsAnimating();
}

SVGAnimatedLengthList&
DOMSVGAnimatedLengthList::InternalAList()
{
  return *mElement->GetAnimatedLengthList(mAttrEnum);
}

const SVGAnimatedLengthList&
DOMSVGAnimatedLengthList::InternalAList() const
{
  return *mElement->GetAnimatedLengthList(mAttrEnum);
}

} 
