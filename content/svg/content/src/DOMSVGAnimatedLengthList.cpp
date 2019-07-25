



































#include "DOMSVGAnimatedLengthList.h"
#include "DOMSVGLengthList.h"
#include "SVGAnimatedLengthList.h"
#include "nsSVGElement.h"
#include "nsCOMPtr.h"
#include "nsSVGAttrTearoffTable.h"



namespace mozilla {

static nsSVGAttrTearoffTable<SVGAnimatedLengthList, DOMSVGAnimatedLengthList>
  sSVGAnimatedLengthListTearoffTable;

NS_SVG_VAL_IMPL_CYCLE_COLLECTION(DOMSVGAnimatedLengthList, mElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(DOMSVGAnimatedLengthList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DOMSVGAnimatedLengthList)

} 
DOMCI_DATA(SVGAnimatedLengthList, mozilla::DOMSVGAnimatedLengthList)
namespace mozilla {

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMSVGAnimatedLengthList)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedLengthList)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGAnimatedLengthList)
NS_INTERFACE_MAP_END

NS_IMETHODIMP
DOMSVGAnimatedLengthList::GetBaseVal(nsIDOMSVGLengthList **_retval)
{
  if (!mBaseVal) {
    mBaseVal = new DOMSVGLengthList(this, InternalAList().GetBaseValue());
  }
  NS_ADDREF(*_retval = mBaseVal);
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGAnimatedLengthList::GetAnimVal(nsIDOMSVGLengthList **_retval)
{
  if (!mAnimVal) {
    mAnimVal = new DOMSVGLengthList(this, InternalAList().GetAnimValue());
  }
  NS_ADDREF(*_retval = mAnimVal);
  return NS_OK;
}

 already_AddRefed<DOMSVGAnimatedLengthList>
DOMSVGAnimatedLengthList::GetDOMWrapper(SVGAnimatedLengthList *aList,
                                        nsSVGElement *aElement,
                                        PRUint8 aAttrEnum,
                                        PRUint8 aAxis)
{
  DOMSVGAnimatedLengthList *wrapper =
    sSVGAnimatedLengthListTearoffTable.GetTearoff(aList);
  if (!wrapper) {
    wrapper = new DOMSVGAnimatedLengthList(aElement, aAttrEnum, aAxis);
    sSVGAnimatedLengthListTearoffTable.AddTearoff(aList, wrapper);
  }
  NS_ADDREF(wrapper);
  return wrapper;
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
    if (aNewValue.Length() < mBaseVal->Length()) {
      
      
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
