




#include "DOMSVGAnimatedLengthList.h"
#include "DOMSVGLengthList.h"
#include "SVGAnimatedLengthList.h"
#include "nsSVGElement.h"
#include "nsCOMPtr.h"
#include "nsSVGAttrTearoffTable.h"
#include "mozilla/dom/SVGAnimatedLengthListBinding.h"
#include "nsContentUtils.h"



namespace mozilla {

static nsSVGAttrTearoffTable<SVGAnimatedLengthList, DOMSVGAnimatedLengthList>
  sSVGAnimatedLengthListTearoffTable;

NS_SVG_VAL_IMPL_CYCLE_COLLECTION_WRAPPERCACHED(DOMSVGAnimatedLengthList, mElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(DOMSVGAnimatedLengthList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DOMSVGAnimatedLengthList)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMSVGAnimatedLengthList)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

JSObject*
DOMSVGAnimatedLengthList::WrapObject(JSContext* aCx, JSObject* aScope)
{
  return dom::SVGAnimatedLengthListBinding::Wrap(aCx, aScope, this);
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
    sSVGAnimatedLengthListTearoffTable.GetTearoff(aList);
  if (!wrapper) {
    wrapper = new DOMSVGAnimatedLengthList(aElement, aAttrEnum, aAxis);
    sSVGAnimatedLengthListTearoffTable.AddTearoff(aList, wrapper);
  }
  return wrapper.forget();
}

 DOMSVGAnimatedLengthList*
DOMSVGAnimatedLengthList::GetDOMWrapperIfExists(SVGAnimatedLengthList *aList)
{
  return sSVGAnimatedLengthListTearoffTable.GetTearoff(aList);
}

DOMSVGAnimatedLengthList::~DOMSVGAnimatedLengthList()
{
  
  
  sSVGAnimatedLengthListTearoffTable.RemoveTearoff(&InternalAList());
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
