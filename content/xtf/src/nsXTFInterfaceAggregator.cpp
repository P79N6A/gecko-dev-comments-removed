





































#include "nsCOMPtr.h"
#include "nsXPTCUtils.h"
#include "nsIInterfaceInfo.h"
#include "nsIInterfaceInfoManager.h"
#include "nsServiceManagerUtils.h"
#include "nsAutoPtr.h"
#ifdef DEBUG
#include <stdio.h>
#endif




class nsXTFInterfaceAggregator : protected nsAutoXPTCStub
{
protected:
  friend nsresult
  NS_NewXTFInterfaceAggregator(const nsIID& iid,
                               nsISupports* inner,
                               nsISupports* outer,
                               void** result);

  nsXTFInterfaceAggregator(const nsIID& iid,
                           nsISupports* inner,
                           nsISupports* outer,
                           nsresult *rv);
  ~nsXTFInterfaceAggregator();
  
public:
  
  NS_DECL_ISUPPORTS
  
  NS_IMETHOD CallMethod(PRUint16 methodIndex,
                        const XPTMethodDescriptor* info,
                        nsXPTCMiniVariant* params);

private:
  nsISupports *mInner;
  nsISupports *mOuter;
  nsIID mIID;
};




nsXTFInterfaceAggregator::nsXTFInterfaceAggregator(const nsIID& iid,
                                                   nsISupports* inner,
                                                   nsISupports* outer,
                                                   nsresult *rv)
  : mInner(inner), mOuter(outer), mIID(iid)
{
#ifdef DEBUG

#endif
  mInner->AddRef();
  mOuter->AddRef();

  *rv = InitStub(iid);
}

nsXTFInterfaceAggregator::~nsXTFInterfaceAggregator()
{
#ifdef DEBUG

#endif
  mInner->Release();
  mOuter->Release();
}

nsresult
NS_NewXTFInterfaceAggregator(const nsIID& iid,
                             nsISupports* inner,
                             nsISupports* outer,
                             void** aResult){
  NS_PRECONDITION(aResult != nsnull, "null ptr");
  if (!aResult)
    return NS_ERROR_NULL_POINTER;

  nsresult rv;

  nsRefPtr<nsXTFInterfaceAggregator> result =
    new nsXTFInterfaceAggregator(iid, inner, outer, &rv);
  if (!result)
    return NS_ERROR_OUT_OF_MEMORY;

  if (NS_FAILED(rv))
    return rv;

  return result->QueryInterface(iid, aResult);
}




NS_IMPL_ADDREF(nsXTFInterfaceAggregator)
NS_IMPL_RELEASE(nsXTFInterfaceAggregator)

NS_IMETHODIMP
nsXTFInterfaceAggregator::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  NS_PRECONDITION(aInstancePtr, "null out param");

  if (aIID.Equals(mIID)) {
    *aInstancePtr = mXPTCStub;
    NS_ADDREF_THIS();
    return NS_OK;
  }

  return mOuter->QueryInterface(aIID, aInstancePtr);
}




NS_IMETHODIMP
nsXTFInterfaceAggregator::CallMethod(PRUint16 methodIndex,
                                     const XPTMethodDescriptor *info,
                                     nsXPTCMiniVariant* params)
{
  NS_ASSERTION(methodIndex >= 3,
               "huh? indirect nsISupports method call unexpected");

  
  int paramCount = info->num_args;
  nsXPTCVariant* fullPars;
  if (!paramCount) {
    fullPars = nsnull;
  }
  else {
    fullPars = new nsXPTCVariant[paramCount];
    if (!fullPars)
      return NS_ERROR_OUT_OF_MEMORY;
  }

  for (int i=0; i<paramCount; ++i) {
    const nsXPTParamInfo& paramInfo = info->params[i];
    PRUint8 flags = paramInfo.IsOut() ? nsXPTCVariant::PTR_IS_DATA : 0;
    fullPars[i].Init(params[i], paramInfo.GetType(), flags);
  }
  
  
  nsresult rv = NS_InvokeByIndex(mInner,
                                 methodIndex,
                                 paramCount,
                                 fullPars);
  if (fullPars)
    delete []fullPars;
  return rv;
}
