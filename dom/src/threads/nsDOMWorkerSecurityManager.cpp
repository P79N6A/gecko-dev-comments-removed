





































#include "nsDOMWorkerSecurityManager.h"


#include "nsIClassInfo.h"


#include "jsapi.h"
#include "nsDOMError.h"
#include "nsThreadUtils.h"


#include "nsDOMThreadService.h"
#include "nsDOMWorker.h"

#define LOG(_args) PR_LOG(gDOMThreadsLog, PR_LOG_DEBUG, _args)

class nsDOMWorkerPrincipal
{
public:
  static void Destroy(JSContext*, JSPrincipals*) {
    
  }

  static JSBool Subsume(JSPrincipals*, JSPrincipals*) {
    return JS_TRUE;
  }
};

static JSPrincipals gWorkerPrincipal =
{ "domworkerthread" ,
  NULL ,
  NULL ,
  1 ,
  nsDOMWorkerPrincipal::Destroy ,
  nsDOMWorkerPrincipal::Subsume  };

NS_IMPL_THREADSAFE_ISUPPORTS1(nsDOMWorkerSecurityManager,
                              nsIXPCSecurityManager)

NS_IMETHODIMP
nsDOMWorkerSecurityManager::CanCreateWrapper(JSContext* aCx,
                                             const nsIID& aIID,
                                             nsISupports* aObj,
                                             nsIClassInfo* aClassInfo,
                                             void** aPolicy)
{
  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerSecurityManager::CanCreateInstance(JSContext* aCx,
                                              const nsCID& aCID)
{
  return CanGetService(aCx, aCID);
}

NS_IMETHODIMP
nsDOMWorkerSecurityManager::CanGetService(JSContext* aCx,
                                          const nsCID& aCID)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  nsDOMWorker* worker = static_cast<nsDOMWorker*>(JS_GetContextPrivate(aCx));
  NS_ASSERTION(worker, "This should be set by the DOM thread service!");

  return worker->IsPrivileged() ? NS_OK : NS_ERROR_DOM_XPCONNECT_ACCESS_DENIED;
}

NS_IMETHODIMP
nsDOMWorkerSecurityManager::CanAccess(PRUint32 aAction,
                                      nsAXPCNativeCallContext* aCallContext,
                                      JSContext* aJSContext,
                                      JSObject* aJSObject,
                                      nsISupports* aObj,
                                      nsIClassInfo* aClassInfo,
                                      jsid aName,
                                      void** aPolicy)
{
  return NS_OK;
}

JSPrincipals*
nsDOMWorkerSecurityManager::WorkerPrincipal()
{
  return &gWorkerPrincipal;
}

JSBool
nsDOMWorkerSecurityManager::JSCheckAccess(JSContext* aCx,
                                          JSObject* aObj,
                                          jsid aId,
                                          JSAccessMode aMode,
                                          jsval* aVp)
{
  return JS_TRUE;
}

JSPrincipals*
nsDOMWorkerSecurityManager::JSFindPrincipal(JSContext* aCx, JSObject* aObj)
{
  return WorkerPrincipal();
}

JSBool
nsDOMWorkerSecurityManager::JSTranscodePrincipals(JSXDRState* aXdr,
                                                 JSPrincipals** aJsprinp)
{
  NS_NOTREACHED("Shouldn't ever call this!");
  return JS_FALSE;
}
