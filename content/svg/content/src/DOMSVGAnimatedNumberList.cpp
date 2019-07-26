




#include "DOMSVGAnimatedNumberList.h"
#include "DOMSVGNumberList.h"
#include "SVGAnimatedNumberList.h"
#include "nsSVGElement.h"
#include "nsCOMPtr.h"
#include "nsSVGAttrTearoffTable.h"
#include "mozilla/dom/SVGAnimatedNumberListBinding.h"



namespace mozilla {

  static inline
nsSVGAttrTearoffTable<SVGAnimatedNumberList, DOMSVGAnimatedNumberList>&
SVGAnimatedNumberListTearoffTable()
{
  static nsSVGAttrTearoffTable<SVGAnimatedNumberList, DOMSVGAnimatedNumberList>
    sSVGAnimatedNumberListTearoffTable;
  return sSVGAnimatedNumberListTearoffTable;
}

NS_SVG_VAL_IMPL_CYCLE_COLLECTION_WRAPPERCACHED(DOMSVGAnimatedNumberList, mElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(DOMSVGAnimatedNumberList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DOMSVGAnimatedNumberList)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMSVGAnimatedNumberList)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

JSObject*
DOMSVGAnimatedNumberList::WrapObject(JSContext* aCx)
{
  return mozilla::dom::SVGAnimatedNumberListBinding::Wrap(aCx, this);
}

already_AddRefed<DOMSVGNumberList>
DOMSVGAnimatedNumberList::BaseVal()
{
  if (!mBaseVal) {
    mBaseVal = new DOMSVGNumberList(this, InternalAList().GetBaseValue());
  }
  nsRefPtr<DOMSVGNumberList> baseVal = mBaseVal;
  return baseVal.forget();
}

already_AddRefed<DOMSVGNumberList>
DOMSVGAnimatedNumberList::AnimVal()
{
  if (!mAnimVal) {
    mAnimVal = new DOMSVGNumberList(this, InternalAList().GetAnimValue());
  }
  nsRefPtr<DOMSVGNumberList> animVal = mAnimVal;
  return animVal.forget();
}

 already_AddRefed<DOMSVGAnimatedNumberList>
DOMSVGAnimatedNumberList::GetDOMWrapper(SVGAnimatedNumberList *aList,
                                        nsSVGElement *aElement,
                                        uint8_t aAttrEnum)
{
  nsRefPtr<DOMSVGAnimatedNumberList> wrapper =
    SVGAnimatedNumberListTearoffTable().GetTearoff(aList);
  if (!wrapper) {
    wrapper = new DOMSVGAnimatedNumberList(aElement, aAttrEnum);
    SVGAnimatedNumberListTearoffTable().AddTearoff(aList, wrapper);
  }
  return wrapper.forget();
}

 DOMSVGAnimatedNumberList*
DOMSVGAnimatedNumberList::GetDOMWrapperIfExists(SVGAnimatedNumberList *aList)
{
  return SVGAnimatedNumberListTearoffTable().GetTearoff(aList);
}

DOMSVGAnimatedNumberList::~DOMSVGAnimatedNumberList()
{
  
  
  SVGAnimatedNumberListTearoffTable().RemoveTearoff(&InternalAList());
}

void
DOMSVGAnimatedNumberList::InternalBaseValListWillChangeTo(const SVGNumberList& aNewValue)
{
  
  
  
  
  
  

  nsRefPtr<DOMSVGAnimatedNumberList> kungFuDeathGrip;
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
DOMSVGAnimatedNumberList::InternalAnimValListWillChangeTo(const SVGNumberList& aNewValue)
{
  if (mAnimVal) {
    mAnimVal->InternalListLengthWillChange(aNewValue.Length());
  }
}

bool
DOMSVGAnimatedNumberList::IsAnimating() const
{
  return InternalAList().IsAnimating();
}

SVGAnimatedNumberList&
DOMSVGAnimatedNumberList::InternalAList()
{
  return *mElement->GetAnimatedNumberList(mAttrEnum);
}

const SVGAnimatedNumberList&
DOMSVGAnimatedNumberList::InternalAList() const
{
  return *mElement->GetAnimatedNumberList(mAttrEnum);
}

} 
