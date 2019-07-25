





































#include "nsSVGAnimatedTransformList.h"
#include "nsSVGTransformList.h"
#include "nsContentUtils.h"




nsSVGAnimatedTransformList::~nsSVGAnimatedTransformList()
{
  if (!mBaseVal) return;
  nsCOMPtr<nsISVGValue> val = do_QueryInterface(mBaseVal);
  if (!val) return;
  val->RemoveObserver(this);
}

void
nsSVGAnimatedTransformList::Init(nsIDOMSVGTransformList* baseVal)
{
  mBaseVal = baseVal;
  if (!mBaseVal) return;
  nsCOMPtr<nsISVGValue> val = do_QueryInterface(mBaseVal);
  if (!val) return;
  val->AddObserver(this);
}




NS_IMPL_ADDREF(nsSVGAnimatedTransformList)
NS_IMPL_RELEASE(nsSVGAnimatedTransformList)

DOMCI_DATA(SVGAnimatedTransformList, nsSVGAnimatedTransformList)

NS_INTERFACE_MAP_BEGIN(nsSVGAnimatedTransformList)
  NS_INTERFACE_MAP_ENTRY(nsISVGValue)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedTransformList)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsISVGValueObserver)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGAnimatedTransformList)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISVGValue)
NS_INTERFACE_MAP_END





NS_IMETHODIMP
nsSVGAnimatedTransformList::SetValueString(const nsAString& aValue)
{
  nsCOMPtr<nsISVGValue> value = do_QueryInterface(mBaseVal);
  return value->SetValueString(aValue);
}

NS_IMETHODIMP
nsSVGAnimatedTransformList::GetValueString(nsAString& aValue)
{
  nsCOMPtr<nsISVGValue> value = do_QueryInterface(mBaseVal);
  return value->GetValueString(aValue);
}





NS_IMETHODIMP
nsSVGAnimatedTransformList::GetBaseVal(nsIDOMSVGTransformList** aBaseVal)
{
  *aBaseVal = mBaseVal;
  NS_ADDREF(*aBaseVal);
  return NS_OK;
}


NS_IMETHODIMP
nsSVGAnimatedTransformList::GetAnimVal(nsIDOMSVGTransformList** aAnimVal)
{
  *aAnimVal = mAnimVal ? mAnimVal : mBaseVal;
  NS_ADDREF(*aAnimVal);
  return NS_OK;
}




NS_IMETHODIMP
nsSVGAnimatedTransformList::WillModifySVGObservable(nsISVGValue* observable,
                                                    modificationType aModType)
{
  WillModify(aModType);
  return NS_OK;
}

NS_IMETHODIMP
nsSVGAnimatedTransformList::DidModifySVGObservable (nsISVGValue* observable,
                                                    modificationType aModType)
{
  DidModify(aModType);
  return NS_OK;
}




PRBool
nsSVGAnimatedTransformList::IsExplicitlySet() const
{
  
  
  
  
  
  
  
  
  if (mAnimVal)
    return PR_TRUE;

  if (!mBaseVal)
    return PR_FALSE;

  PRUint32 numItems = 0;
  nsIDOMSVGTransformList *list = mBaseVal.get();
  list->GetNumberOfItems(&numItems);
  return numItems > 0;
}




nsresult
NS_NewSVGAnimatedTransformList(nsIDOMSVGAnimatedTransformList** result,
                               nsIDOMSVGTransformList* baseVal)
{
  *result = nsnull;
  
  nsSVGAnimatedTransformList* animatedTransformList = new nsSVGAnimatedTransformList();
  if(!animatedTransformList) return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(animatedTransformList);

  animatedTransformList->Init(baseVal);
  
  *result = (nsIDOMSVGAnimatedTransformList*) animatedTransformList;
  
  return NS_OK;
}

