





































#include "nsSVGValue.h"
#include "nsWeakReference.h"
#include "nsSVGAnimatedEnumeration.h"
#include "nsISVGEnum.h"
#include "nsContentUtils.h"




class nsSVGAnimatedEnumeration : public nsIDOMSVGAnimatedEnumeration,
                                 public nsSVGValue,
                                 public nsISVGValueObserver
{
protected:
  friend nsresult NS_NewSVGAnimatedEnumeration(nsIDOMSVGAnimatedEnumeration** result,
                                               nsISVGEnum* aBaseVal);
  nsSVGAnimatedEnumeration();
  ~nsSVGAnimatedEnumeration();
  nsresult Init(nsISVGEnum* aBaseVal);
  
public:
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGANIMATEDENUMERATION

  
  NS_IMETHOD SetValueString(const nsAString& aValue);
  NS_IMETHOD GetValueString(nsAString& aValue);

  
  NS_IMETHOD WillModifySVGObservable(nsISVGValue* observable,
                                     modificationType aModType);
  NS_IMETHOD DidModifySVGObservable (nsISVGValue* observable,
                                     modificationType aModType);
  
  
  
  
protected:
  nsCOMPtr<nsISVGEnum> mBaseVal;
};






nsSVGAnimatedEnumeration::nsSVGAnimatedEnumeration()
{
}

nsSVGAnimatedEnumeration::~nsSVGAnimatedEnumeration()
{
  nsCOMPtr<nsISVGValue> val = do_QueryInterface(mBaseVal);
  if (val) val->RemoveObserver(this);
}

nsresult
nsSVGAnimatedEnumeration::Init(nsISVGEnum* aBaseVal)
{
  mBaseVal = aBaseVal;
  if (!mBaseVal) return NS_ERROR_FAILURE;
  nsCOMPtr<nsISVGValue> val = do_QueryInterface(mBaseVal);
  NS_ASSERTION(val, "baseval needs to implement nsISVGValue interface");
  if (!val) return NS_ERROR_FAILURE;
  val->AddObserver(this);
  return NS_OK;
}




NS_IMPL_ADDREF(nsSVGAnimatedEnumeration)
NS_IMPL_RELEASE(nsSVGAnimatedEnumeration)


NS_INTERFACE_MAP_BEGIN(nsSVGAnimatedEnumeration)
  NS_INTERFACE_MAP_ENTRY(nsISVGValue)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedEnumeration)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsISVGValueObserver)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGAnimatedEnumeration)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISVGValue)
NS_INTERFACE_MAP_END
  



NS_IMETHODIMP
nsSVGAnimatedEnumeration::SetValueString(const nsAString& aValue)
{
  nsCOMPtr<nsISVGValue> value = do_QueryInterface(mBaseVal);
  NS_ASSERTION(value, "svg animated enumeration base value has wrong interface!");
  return value->SetValueString(aValue);
}

NS_IMETHODIMP
nsSVGAnimatedEnumeration::GetValueString(nsAString& aValue)
{
  nsCOMPtr<nsISVGValue> value = do_QueryInterface(mBaseVal);
  NS_ASSERTION(value, "svg animated enumeration base value has wrong interface!");
  return value->GetValueString(aValue);
}





NS_IMETHODIMP
nsSVGAnimatedEnumeration::GetBaseVal(PRUint16 *aBaseVal)
{
  mBaseVal->GetIntegerValue(*aBaseVal);
  return NS_OK;
}

NS_IMETHODIMP
nsSVGAnimatedEnumeration::SetBaseVal(PRUint16 aBaseVal)
{
  return mBaseVal->SetIntegerValue(aBaseVal);
}


NS_IMETHODIMP
nsSVGAnimatedEnumeration::GetAnimVal(PRUint16 *aAnimVal)
{
  mBaseVal->GetIntegerValue(*aAnimVal);
  return NS_OK;
}




NS_IMETHODIMP
nsSVGAnimatedEnumeration::WillModifySVGObservable(nsISVGValue* observable,
                                                  modificationType aModType)
{
  WillModify(aModType);
  return NS_OK;
}

NS_IMETHODIMP
nsSVGAnimatedEnumeration::DidModifySVGObservable (nsISVGValue* observable,
                                                  modificationType aModType)
{
  DidModify(aModType);
  return NS_OK;
}




nsresult
NS_NewSVGAnimatedEnumeration(nsIDOMSVGAnimatedEnumeration** aResult,
                             nsISVGEnum* aBaseVal)
{
  *aResult = nsnull;
  
  nsSVGAnimatedEnumeration* animatedEnum = new nsSVGAnimatedEnumeration();
  if (!animatedEnum) return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(animatedEnum);

  nsresult rv = animatedEnum->Init(aBaseVal);
  
  *aResult = (nsIDOMSVGAnimatedEnumeration*) animatedEnum;
  
  return rv;
}


