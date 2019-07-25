





































#include "Principal.h"

#include "jsapi.h"

BEGIN_WORKERS_NAMESPACE

namespace {

JSPrincipals gPrincipal = {
  1
#ifdef DEBUG
  , kJSPrincipalsDebugToken
#endif
};

} 

JSPrincipals*
GetWorkerPrincipal()
{
  return &gPrincipal;
}

END_WORKERS_NAMESPACE
