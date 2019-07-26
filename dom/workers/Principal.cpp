




#include "Principal.h"

#include "jsapi.h"

BEGIN_WORKERS_NAMESPACE

JSPrincipals*
GetWorkerPrincipal()
{
  static Atomic<bool> sInitialized(false);
  static JSPrincipals sPrincipal;

  bool isInitialized = sInitialized.exchange(true);
  if (!isInitialized) {
    sPrincipal.refcount = 1;
#ifdef DEBUG
    sPrincipal.debugToken = kJSPrincipalsDebugToken;
#endif
  }
  return &sPrincipal;
}

END_WORKERS_NAMESPACE
