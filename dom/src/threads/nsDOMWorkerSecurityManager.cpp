





































#include "nsDOMWorkerSecurityManager.h"


#include "nsIClassInfo.h"


#include "jsapi.h"


#include "nsDOMThreadService.h"

#define LOG(_args) PR_LOG(gDOMThreadsLog, PR_LOG_DEBUG, _args)

NS_IMPL_THREADSAFE_ISUPPORTS1(nsDOMWorkerSecurityManager,
                              nsIXPCSecurityManager)

NS_IMETHODIMP
nsDOMWorkerSecurityManager::CanCreateWrapper(JSContext* aJSContext,
                                             const nsIID& aIID,
                                             nsISupports* aObj,
                                             nsIClassInfo* aClassInfo,
                                             void** aPolicy)
{
  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerSecurityManager::CanCreateInstance(JSContext* aJSContext,
                                              const nsCID& aCID)
{
  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerSecurityManager::CanGetService(JSContext* aJSContext,
                                          const nsCID& aCID)
{
  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerSecurityManager::CanAccess(PRUint32 aAction,
                                      nsAXPCNativeCallContext* aCallContext,
                                      JSContext* aJSContext,
                                      JSObject* aJSObject,
                                      nsISupports* aObj,
                                      nsIClassInfo* aClassInfo,
                                      jsval aName,
                                      void** aPolicy)
{
  return NS_OK;
}
