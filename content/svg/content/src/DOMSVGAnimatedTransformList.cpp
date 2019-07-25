





































#include "DOMSVGAnimatedTransformList.h"
#include "DOMSVGTransformList.h"
#include "SVGAnimatedTransformList.h"
#include "nsSVGAttrTearoffTable.h"

namespace mozilla {

static
  nsSVGAttrTearoffTable<SVGAnimatedTransformList,DOMSVGAnimatedTransformList>
  sSVGAnimatedTransformListTearoffTable;

NS_SVG_VAL_IMPL_CYCLE_COLLECTION(DOMSVGAnimatedTransformList, mElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(DOMSVGAnimatedTransformList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DOMSVGAnimatedTransformList)

} 
DOMCI_DATA(SVGAnimatedTransformList, mozilla::DOMSVGAnimatedTransformList)
namespace mozilla {

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMSVGAnimatedTransformList)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedTransformList)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGAnimatedTransformList)
NS_INTERFACE_MAP_END





NS_IMETHODIMP
DOMSVGAnimatedTransformList::GetBaseVal(nsIDOMSVGTransformList** aBaseVal)
{
  if (!mBaseVal) {
    mBaseVal = new DOMSVGTransformList(this, InternalAList().GetBaseValue());
  }
  NS_ADDREF(*aBaseVal = mBaseVal);
  return NS_OK;
}


NS_IMETHODIMP
DOMSVGAnimatedTransformList::GetAnimVal(nsIDOMSVGTransformList** aAnimVal)
{
  if (!mAnimVal) {
    mAnimVal = new DOMSVGTransformList(this, InternalAList().GetAnimValue());
  }
  NS_ADDREF(*aAnimVal = mAnimVal);
  return NS_OK;
}

 already_AddRefed<DOMSVGAnimatedTransformList>
DOMSVGAnimatedTransformList::GetDOMWrapper(SVGAnimatedTransformList *aList,
                                           nsSVGElement *aElement)
{
  DOMSVGAnimatedTransformList *wrapper =
    sSVGAnimatedTransformListTearoffTable.GetTearoff(aList);
  if (!wrapper) {
    wrapper = new DOMSVGAnimatedTransformList(aElement);
    sSVGAnimatedTransformListTearoffTable.AddTearoff(aList, wrapper);
  }
  NS_ADDREF(wrapper);
  return wrapper;
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
  PRUint32 aNewLength)
{
  
  
  
  
  
  

  nsRefPtr<DOMSVGAnimatedTransformList> kungFuDeathGrip;
  if (mBaseVal) {
    if (aNewLength < mBaseVal->Length()) {
      
      
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
  PRUint32 aNewLength)
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
