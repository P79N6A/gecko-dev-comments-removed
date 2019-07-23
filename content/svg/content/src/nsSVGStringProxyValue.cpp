





































#include "nsSVGValue.h"
#include "nsWeakReference.h"


















class nsSVGStringProxyValue : public nsSVGValue,
                              public nsISVGValueObserver
{
protected:
  friend nsresult
  NS_CreateSVGStringProxyValue(nsISVGValue* proxiedValue, nsISVGValue** aResult);
  
  nsSVGStringProxyValue();
  virtual ~nsSVGStringProxyValue();
  PRBool Init(nsISVGValue* proxiedValue);
  
public:
  NS_DECL_ISUPPORTS

  
  NS_IMETHOD SetValueString(const nsAString& aValue);
  NS_IMETHOD GetValueString(nsAString& aValue);

  
  NS_IMETHOD WillModifySVGObservable(nsISVGValue* observable,
                                     modificationType aModType);
  NS_IMETHOD DidModifySVGObservable (nsISVGValue* observable,
                                     modificationType aModType);

  
  

protected:
  nsString mCachedValue;
  nsCOMPtr<nsISVGValue> mProxiedValue;
  PRPackedBool mUseCachedValue;
};



nsresult
NS_CreateSVGStringProxyValue(nsISVGValue* proxiedValue,
                             nsISVGValue** aResult)
{
  *aResult = nsnull;
  
  nsSVGStringProxyValue *sp = new nsSVGStringProxyValue();
  if(!sp) return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(sp);
  if (!sp->Init(proxiedValue)) {
    NS_RELEASE(sp);
    return NS_ERROR_FAILURE;
  }
  
  *aResult = sp;
  return NS_OK;
}

nsSVGStringProxyValue::nsSVGStringProxyValue()
{
#ifdef DEBUG
  printf("nsSVGStringProxyValue CTOR\n");
#endif
}

nsSVGStringProxyValue::~nsSVGStringProxyValue()
{
  mProxiedValue->RemoveObserver(this);
#ifdef DEBUG
  printf("nsSVGStringProxyValue DTOR\n");
#endif
}

PRBool nsSVGStringProxyValue::Init(nsISVGValue* proxiedValue)
{
  mProxiedValue = proxiedValue;
  mProxiedValue->AddObserver(this);
  return PR_TRUE;
}  




NS_IMPL_ADDREF(nsSVGStringProxyValue)
NS_IMPL_RELEASE(nsSVGStringProxyValue)

NS_INTERFACE_MAP_BEGIN(nsSVGStringProxyValue)
  NS_INTERFACE_MAP_ENTRY(nsISVGValue)
  NS_INTERFACE_MAP_ENTRY(nsISVGValueObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISVGValue)
NS_INTERFACE_MAP_END




NS_IMETHODIMP
nsSVGStringProxyValue::SetValueString(const nsAString& aValue)
{
#ifdef DEBUG
  printf("nsSVGStringProxyValue(%p)::SetValueString(%s)\n", this, NS_ConvertUTF16toUTF8(aValue).get());
#endif
  if (NS_FAILED(mProxiedValue->SetValueString(aValue))) {
#ifdef DEBUG
    printf("  -> call failed, now using cached value\n");
#endif
    mUseCachedValue = PR_TRUE; 
                               
    mCachedValue = aValue;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsSVGStringProxyValue::GetValueString(nsAString& aValue)
{
  if (!mUseCachedValue)
    return mProxiedValue->GetValueString(aValue);

  aValue = mCachedValue;
  return NS_OK;
}





NS_IMETHODIMP
nsSVGStringProxyValue::WillModifySVGObservable(nsISVGValue* observable,
                                               modificationType aModType)
{
  WillModify(aModType);
  return NS_OK;
}

NS_IMETHODIMP
nsSVGStringProxyValue::DidModifySVGObservable (nsISVGValue* observable,
                                               modificationType aModType)
{
  
  
  mUseCachedValue = PR_FALSE;
  
  DidModify(aModType);
  return NS_OK;
}
