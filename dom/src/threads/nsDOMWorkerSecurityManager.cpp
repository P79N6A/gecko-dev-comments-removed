





































#include "nsDOMWorkerSecurityManager.h"


#include "nsIClassInfo.h"


#include "jsapi.h"


#include "nsDOMThreadService.h"

#define LOG(_args) PR_LOG(gDOMThreadsLog, PR_LOG_DEBUG, _args)

class nsDOMWorkerPrincipal : public JSPrincipals
{
public:
  nsDOMWorkerPrincipal() {
    codebase = "domworkerthread";
    getPrincipalArray = NULL;
    globalPrivilegesEnabled = NULL;
    refcount = 1;
    destroy = nsDOMWorkerPrincipal::Destroy;
    subsume = nsDOMWorkerPrincipal::Subsume;
  }

  static void Destroy(JSContext*, JSPrincipals*) {
    
  }

  static JSBool Subsume(JSPrincipals*, JSPrincipals*) {
    return JS_TRUE;
  }
};

static nsDOMWorkerPrincipal gWorkerPrincipal;

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
  NS_NOTREACHED("Should not call this!");
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMWorkerSecurityManager::CanGetService(JSContext* aJSContext,
                                          const nsCID& aCID)
{
  NS_NOTREACHED("Should not call this!");
  return NS_ERROR_UNEXPECTED;
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

JSPrincipals*
nsDOMWorkerSecurityManager::WorkerPrincipal()
{
  return &gWorkerPrincipal;
}

JSBool
nsDOMWorkerSecurityManager::JSCheckAccess(JSContext* aCx,
                                          JSObject* aObj,
                                          jsval aId,
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
