




#include "Principal.h"

#include "jsapi.h"

BEGIN_WORKERS_NAMESPACE

JSPrincipals*
GetWorkerPrincipal()
{
  static Atomic<uint32_t> sInitialized(0);
  static JSPrincipals sPrincipal;

  if (!sInitialized.exchange(1)) {
    sPrincipal.refcount = 1;
#ifdef DEBUG
    sPrincipal.debugToken = kJSPrincipalsDebugToken;
#endif
  }
  return &sPrincipal;
}

END_WORKERS_NAMESPACE
