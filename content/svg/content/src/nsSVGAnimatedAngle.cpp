



































#include "nsSVGValue.h"
#include "nsWeakReference.h"
#include "nsSVGAnimatedAngle.h"
#include "nsSVGLength.h"
#include "nsContentUtils.h"





class nsSVGAnimatedAngle : public nsIDOMSVGAnimatedAngle,
                           public nsSVGValue,
                           public nsISVGValueObserver
{
protected:
  friend nsresult NS_NewSVGAnimatedAngle(nsIDOMSVGAnimatedAngle** result,
                                         nsIDOMSVGAngle* baseVal);
  nsSVGAnimatedAngle();
  ~nsSVGAnimatedAngle();
  void Init(nsIDOMSVGAngle* baseVal);
  
public:
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGANIMATEDANGLE

  
  NS_IMETHOD SetValueString(const nsAString& aValue);
  NS_IMETHOD GetValueString(nsAString& aValue);

  
  NS_IMETHOD WillModifySVGObservable(nsISVGValue* observable,
                                     modificationType aModType);
  NS_IMETHOD DidModifySVGObservable (nsISVGValue* observable,
                                     modificationType aModType);
  
  
  
  
protected:
  nsCOMPtr<nsIDOMSVGAngle> mBaseVal;
};






nsSVGAnimatedAngle::nsSVGAnimatedAngle()
{
}

nsSVGAnimatedAngle::~nsSVGAnimatedAngle()
{
  if (!mBaseVal) return;
  nsCOMPtr<nsISVGValue> val = do_QueryInterface(mBaseVal);
  if (!val) return;
  val->RemoveObserver(this);
}

void
nsSVGAnimatedAngle::Init(nsIDOMSVGAngle* baseVal)
{
  mBaseVal = baseVal;
  if (!mBaseVal) return;
  nsCOMPtr<nsISVGValue> val = do_QueryInterface(mBaseVal);
  NS_ASSERTION(val, "baseval needs to implement nsISVGValue interface");
  if (!val) return;
  val->AddObserver(this);
}




NS_IMPL_ADDREF(nsSVGAnimatedAngle)
NS_IMPL_RELEASE(nsSVGAnimatedAngle)


NS_INTERFACE_MAP_BEGIN(nsSVGAnimatedAngle)
  NS_INTERFACE_MAP_ENTRY(nsISVGValue)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedAngle)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsISVGValueObserver)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGAnimatedAngle)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISVGValue)
NS_INTERFACE_MAP_END
  



NS_IMETHODIMP
nsSVGAnimatedAngle::SetValueString(const nsAString& aValue)
{
  nsCOMPtr<nsISVGValue> value = do_QueryInterface(mBaseVal);
  return value->SetValueString(aValue);
}

NS_IMETHODIMP
nsSVGAnimatedAngle::GetValueString(nsAString& aValue)
{
  nsCOMPtr<nsISVGValue> value = do_QueryInterface(mBaseVal);
  return value->GetValueString(aValue);
}





NS_IMETHODIMP
nsSVGAnimatedAngle::GetBaseVal(nsIDOMSVGAngle * *aBaseVal)
{
  *aBaseVal = mBaseVal;
  NS_ADDREF(*aBaseVal);
  return NS_OK;
}


NS_IMETHODIMP
nsSVGAnimatedAngle::GetAnimVal(nsIDOMSVGAngle * *aAnimVal)
{
  *aAnimVal = mBaseVal;
  NS_ADDREF(*aAnimVal);
  return NS_OK;
}




NS_IMETHODIMP
nsSVGAnimatedAngle::WillModifySVGObservable(nsISVGValue* observable,
                                            modificationType aModType)
{
  WillModify(aModType);
  return NS_OK;
}

NS_IMETHODIMP
nsSVGAnimatedAngle::DidModifySVGObservable (nsISVGValue* observable,
                                            modificationType aModType)
{
  DidModify(aModType);
  return NS_OK;
}




nsresult
NS_NewSVGAnimatedAngle(nsIDOMSVGAnimatedAngle** aResult,
                       nsIDOMSVGAngle* baseVal)
{
  *aResult = nsnull;
  
  nsSVGAnimatedAngle* animatedLength = new nsSVGAnimatedAngle();
  if(!animatedLength) return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(animatedLength);

  animatedLength->Init(baseVal);
  
  *aResult = (nsIDOMSVGAnimatedAngle*) animatedLength;
  
  return NS_OK;
}
