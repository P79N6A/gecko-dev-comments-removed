




#include "nsCOMPtr.h"
#include "nsXPTCUtils.h"
#include "nsIInterfaceInfo.h"
#include "nsIInterfaceInfoManager.h"
#include "nsServiceManagerUtils.h"
#include "nsAutoPtr.h"
#include "mozilla/Attributes.h"
#ifdef DEBUG
#include <stdio.h>
#endif




class nsXTFWeakTearoff MOZ_FINAL : protected nsAutoXPTCStub
{
protected:
  ~nsXTFWeakTearoff();
  
public:
  nsXTFWeakTearoff(const nsIID& iid,
                   nsISupports* obj,
                   nsresult *rv);

  
  NS_DECL_ISUPPORTS
  
  NS_IMETHOD CallMethod(PRUint16 methodIndex,
                        const XPTMethodDescriptor* info,
                        nsXPTCMiniVariant* params);

private:
  nsISupports *mObj;
  nsIID mIID;
};




nsXTFWeakTearoff::nsXTFWeakTearoff(const nsIID& iid,
                                   nsISupports* obj,
                                   nsresult *rv)
  : mObj(obj), mIID(iid)
{
  MOZ_COUNT_CTOR(nsXTFWeakTearoff);

  *rv = InitStub(iid);
}

nsXTFWeakTearoff::~nsXTFWeakTearoff()
{
  MOZ_COUNT_DTOR(nsXTFWeakTearoff);
}

nsresult
NS_NewXTFWeakTearoff(const nsIID& iid,
                     nsISupports* obj,
                     nsISupports** aResult){
  NS_PRECONDITION(aResult != nullptr, "null ptr");
  if (!aResult)
    return NS_ERROR_NULL_POINTER;

  nsresult rv;

  nsRefPtr<nsXTFWeakTearoff> result =
    new nsXTFWeakTearoff(iid, obj, &rv);
  if (!result)
    return NS_ERROR_OUT_OF_MEMORY;

  if (NS_FAILED(rv))
    return rv;

  return result->QueryInterface(iid, (void**) aResult);
}




NS_IMPL_ADDREF(nsXTFWeakTearoff)
NS_IMPL_RELEASE(nsXTFWeakTearoff)

NS_IMETHODIMP
nsXTFWeakTearoff::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  NS_PRECONDITION(aInstancePtr, "null out param");

  if (aIID.Equals(mIID) || aIID.Equals(NS_GET_IID(nsISupports))) {
    *aInstancePtr = mXPTCStub;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  
  
  
  *aInstancePtr = nullptr;
  return NS_ERROR_NO_INTERFACE;
}

NS_IMETHODIMP
nsXTFWeakTearoff::CallMethod(PRUint16 methodIndex,
                             const XPTMethodDescriptor* info,
                             nsXPTCMiniVariant* params)
{
  NS_ASSERTION(methodIndex >= 3,
               "huh? indirect nsISupports method call unexpected");

  
  int paramCount = info->num_args;
  nsXPTCVariant* fullPars;
  if (!paramCount) {
    fullPars = nullptr;
  }
  else {
    fullPars = new nsXPTCVariant[paramCount];
    if (!fullPars)
      return NS_ERROR_OUT_OF_MEMORY;
  }

  for (int i=0; i<paramCount; ++i) {
    const nsXPTParamInfo& paramInfo = info->params[i];
    uint8 flags = paramInfo.IsOut() ? nsXPTCVariant::PTR_IS_DATA : 0;
    fullPars[i].Init(params[i], paramInfo.GetType(), flags);
  }
  
  
  nsresult rv = NS_InvokeByIndex(mObj, methodIndex, paramCount, fullPars);
  if (fullPars)
    delete []fullPars;
  return rv;
}
