





































#include "nsSVGAnimatedRect.h"
#include "nsSVGRect.h"
#include "nsSVGValue.h"
#include "nsWeakReference.h"
#include "nsContentUtils.h"




class nsSVGAnimatedRect : public nsIDOMSVGAnimatedRect,
                          public nsSVGValue,
                          public nsISVGValueObserver
{  
protected:
  friend nsresult NS_NewSVGAnimatedRect(nsIDOMSVGAnimatedRect** result,
                                        nsIDOMSVGRect* baseVal);

  nsSVGAnimatedRect();
  ~nsSVGAnimatedRect();
  void Init(nsIDOMSVGRect* baseVal);
  
public:
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGANIMATEDRECT

  
  NS_IMETHOD SetValueString(const nsAString& aValue);
  NS_IMETHOD GetValueString(nsAString& aValue);

  
  NS_IMETHOD WillModifySVGObservable(nsISVGValue* observable,
                                     modificationType aModType);
  NS_IMETHOD DidModifySVGObservable (nsISVGValue* observable,
                                     modificationType aModType);
  
  
  
  
protected:
  nsCOMPtr<nsIDOMSVGRect> mBaseVal;
};





nsSVGAnimatedRect::nsSVGAnimatedRect()
{
}

nsSVGAnimatedRect::~nsSVGAnimatedRect()
{
  if (!mBaseVal) return;
    nsCOMPtr<nsISVGValue> val = do_QueryInterface(mBaseVal);
  if (!val) return;
  val->RemoveObserver(this);
}

void
nsSVGAnimatedRect::Init(nsIDOMSVGRect* baseVal)
{
  mBaseVal = baseVal;
  if (!mBaseVal) return;
  nsCOMPtr<nsISVGValue> val = do_QueryInterface(mBaseVal);
  if (!val) return;
  val->AddObserver(this);
}




NS_IMPL_ADDREF(nsSVGAnimatedRect)
NS_IMPL_RELEASE(nsSVGAnimatedRect)

NS_INTERFACE_MAP_BEGIN(nsSVGAnimatedRect)
  NS_INTERFACE_MAP_ENTRY(nsISVGValue)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedRect)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsISVGValueObserver)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGAnimatedRect)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISVGValue)
NS_INTERFACE_MAP_END




NS_IMETHODIMP
nsSVGAnimatedRect::SetValueString(const nsAString& aValue)
{
  nsCOMPtr<nsISVGValue> value = do_QueryInterface(mBaseVal);
  return value->SetValueString(aValue);
}

NS_IMETHODIMP
nsSVGAnimatedRect::GetValueString(nsAString& aValue)
{
  nsCOMPtr<nsISVGValue> value = do_QueryInterface(mBaseVal);
  return value->GetValueString(aValue);
}





NS_IMETHODIMP
nsSVGAnimatedRect::GetBaseVal(nsIDOMSVGRect * *aBaseVal)
{
  *aBaseVal = mBaseVal;
  NS_ADDREF(*aBaseVal);
  return NS_OK;
}


NS_IMETHODIMP
nsSVGAnimatedRect::GetAnimVal(nsIDOMSVGRect * *aAnimVal)
{
  *aAnimVal = mBaseVal;
  NS_ADDREF(*aAnimVal);
  return NS_OK;
}




NS_IMETHODIMP
nsSVGAnimatedRect::WillModifySVGObservable(nsISVGValue* observable,
                                           modificationType aModType)
{
  WillModify(aModType);
  return NS_OK;
}

NS_IMETHODIMP
nsSVGAnimatedRect::DidModifySVGObservable (nsISVGValue* observable,
                                           modificationType aModType)
{
  DidModify(aModType);
  return NS_OK;
}





nsresult
NS_NewSVGAnimatedRect(nsIDOMSVGAnimatedRect** result,
                      nsIDOMSVGRect* baseVal)
{
  *result = nsnull;
  
  nsSVGAnimatedRect* animatedRect = new nsSVGAnimatedRect();
  if(!animatedRect) return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(animatedRect);

  animatedRect->Init(baseVal);
  
  *result = (nsIDOMSVGAnimatedRect*) animatedRect;
  
  return NS_OK;
}

