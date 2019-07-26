




#include "DOMSVGAnimatedTransformList.h"
#include "DOMSVGTransformList.h"
#include "SVGAnimatedTransformList.h"
#include "nsSVGAttrTearoffTable.h"
#include "mozilla/dom/SVGAnimatedTransformListBinding.h"

namespace mozilla {

static
  nsSVGAttrTearoffTable<SVGAnimatedTransformList,DOMSVGAnimatedTransformList>
  sSVGAnimatedTransformListTearoffTable;

NS_IMPL_CYCLE_COLLECTION_CLASS(DOMSVGAnimatedTransformList)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(DOMSVGAnimatedTransformList)


NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(DOMSVGAnimatedTransformList)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mElement)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(DOMSVGAnimatedTransformList)
NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(DOMSVGAnimatedTransformList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DOMSVGAnimatedTransformList)

} 
DOMCI_DATA(SVGAnimatedTransformList, mozilla::DOMSVGAnimatedTransformList)
namespace mozilla {

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMSVGAnimatedTransformList)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedTransformList)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGAnimatedTransformList)
NS_INTERFACE_MAP_END

JSObject*
DOMSVGAnimatedTransformList::WrapObject(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap)
{
  return mozilla::dom::SVGAnimatedTransformListBinding::Wrap(aCx, aScope, this, aTriedToWrap);
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


NS_IMETHODIMP
DOMSVGAnimatedTransformList::GetBaseVal(nsIDOMSVGTransformList** aBaseVal)
{
  *aBaseVal = BaseVal().get();
  return NS_OK;
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


NS_IMETHODIMP
DOMSVGAnimatedTransformList::GetAnimVal(nsIDOMSVGTransformList** aAnimVal)
{
  *aAnimVal = AnimVal().get();
  return NS_OK;
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
