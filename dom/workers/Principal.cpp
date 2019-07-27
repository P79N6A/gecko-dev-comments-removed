




#include "Principal.h"

#include "jsapi.h"
#include "mozilla/Assertions.h"

BEGIN_WORKERS_NAMESPACE

JSPrincipals*
GetWorkerPrincipal()
{
  static JSPrincipals sPrincipal;

  





  int32_t prevRefcount = sPrincipal.refcount++;
  if (prevRefcount > 0) {
    --sPrincipal.refcount;
  } else {
#ifdef DEBUG
    sPrincipal.debugToken = kJSPrincipalsDebugToken;
#endif
  }

  return &sPrincipal;
}

void
DestroyWorkerPrincipals(JSPrincipals* aPrincipals)
{
  MOZ_ASSERT_UNREACHABLE("Worker principals refcount should never fall below one");
}

END_WORKERS_NAMESPACE
