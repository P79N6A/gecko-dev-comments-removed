





































#include "nsSVGAnimatedPreserveAspectRatio.h"
#include "nsSVGPreserveAspectRatio.h"
#include "nsSVGValue.h"
#include "nsWeakReference.h"
#include "nsContentUtils.h"




class nsSVGAnimatedPreserveAspectRatio : public nsIDOMSVGAnimatedPreserveAspectRatio,
                                         public nsSVGValue,
                                         public nsISVGValueObserver
{
protected:
  friend nsresult NS_NewSVGAnimatedPreserveAspectRatio(
                                 nsIDOMSVGAnimatedPreserveAspectRatio** result,
                                 nsIDOMSVGPreserveAspectRatio* baseVal);

  nsSVGAnimatedPreserveAspectRatio();
  ~nsSVGAnimatedPreserveAspectRatio();
  void Init(nsIDOMSVGPreserveAspectRatio* baseVal);

public:
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGANIMATEDPRESERVEASPECTRATIO

  
  NS_IMETHOD SetValueString(const nsAString& aValue);
  NS_IMETHOD GetValueString(nsAString& aValue);

  
  NS_IMETHOD WillModifySVGObservable(nsISVGValue* observable,
                                     modificationType aModType);
  NS_IMETHOD DidModifySVGObservable (nsISVGValue* observable,
                                     modificationType aModType);

  
  

protected:
  nsCOMPtr<nsIDOMSVGPreserveAspectRatio> mBaseVal;
};





nsSVGAnimatedPreserveAspectRatio::nsSVGAnimatedPreserveAspectRatio()
{
}

nsSVGAnimatedPreserveAspectRatio::~nsSVGAnimatedPreserveAspectRatio()
{
  nsCOMPtr<nsISVGValue> val( do_QueryInterface(mBaseVal) );
  if (!val) return;
  val->RemoveObserver(this);
}

void
nsSVGAnimatedPreserveAspectRatio::Init(nsIDOMSVGPreserveAspectRatio* aBaseVal)
{
  mBaseVal = aBaseVal;
  nsCOMPtr<nsISVGValue> val( do_QueryInterface(mBaseVal) );
  NS_ASSERTION(val, "baseval needs to implement nsISVGValue interface");
  if (!val) return;
  val->AddObserver(this);
}




NS_IMPL_ADDREF(nsSVGAnimatedPreserveAspectRatio)
NS_IMPL_RELEASE(nsSVGAnimatedPreserveAspectRatio)

NS_INTERFACE_MAP_BEGIN(nsSVGAnimatedPreserveAspectRatio)
  NS_INTERFACE_MAP_ENTRY(nsISVGValue)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedPreserveAspectRatio)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsISVGValueObserver)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGAnimatedPreserveAspectRatio)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISVGValue)
NS_INTERFACE_MAP_END




NS_IMETHODIMP
nsSVGAnimatedPreserveAspectRatio::SetValueString(const nsAString& aValue)
{
  nsresult rv;
  nsCOMPtr<nsISVGValue> val( do_QueryInterface(mBaseVal, &rv) );
  return NS_FAILED(rv)? rv: val->SetValueString(aValue);
}

NS_IMETHODIMP
nsSVGAnimatedPreserveAspectRatio::GetValueString(nsAString& aValue)
{
  nsresult rv;
  nsCOMPtr<nsISVGValue> val( do_QueryInterface(mBaseVal, &rv) );
  return NS_FAILED(rv)? rv: val->GetValueString(aValue);
}





NS_IMETHODIMP
nsSVGAnimatedPreserveAspectRatio::GetBaseVal(nsIDOMSVGPreserveAspectRatio** aBaseVal)
{
  *aBaseVal = mBaseVal;
  NS_ADDREF(*aBaseVal);
  return NS_OK;
}


NS_IMETHODIMP
nsSVGAnimatedPreserveAspectRatio::GetAnimVal(nsIDOMSVGPreserveAspectRatio** aAnimVal)
{
  *aAnimVal = mBaseVal;
  NS_ADDREF(*aAnimVal);
  return NS_OK;
}




NS_IMETHODIMP
nsSVGAnimatedPreserveAspectRatio::WillModifySVGObservable(nsISVGValue* observable,
                                                          modificationType aModType)
{
  WillModify(aModType);
  return NS_OK;
}

NS_IMETHODIMP
nsSVGAnimatedPreserveAspectRatio::DidModifySVGObservable (nsISVGValue* observable,
                                                          modificationType aModType)
{
  DidModify(aModType);
  return NS_OK;
}




nsresult
NS_NewSVGAnimatedPreserveAspectRatio(nsIDOMSVGAnimatedPreserveAspectRatio** result,
                                     nsIDOMSVGPreserveAspectRatio* baseVal)
{
  *result = nsnull;

  if (!baseVal) {
    NS_ERROR("need baseVal");
    return NS_ERROR_FAILURE;
  }

  nsSVGAnimatedPreserveAspectRatio* animatedPreserveAspectRatio = new nsSVGAnimatedPreserveAspectRatio();
  if (!animatedPreserveAspectRatio)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(animatedPreserveAspectRatio);
  animatedPreserveAspectRatio->Init(baseVal);
  *result = (nsIDOMSVGAnimatedPreserveAspectRatio*) animatedPreserveAspectRatio;

  return NS_OK;
}
