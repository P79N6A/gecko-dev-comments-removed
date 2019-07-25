





































#include "Principal.h"

#include "jsapi.h"

namespace {

void
PrincipalDestroy(JSContext*, JSPrincipals*)
{
  
}

JSBool
PrincipalSubsume(JSPrincipals*, JSPrincipals*)
{
  return JS_TRUE;
}

const char gPrincipalCodebase[] = "Web Worker";

JSPrincipals gPrincipal = {
  const_cast<char*>(gPrincipalCodebase),
  1, PrincipalDestroy, PrincipalSubsume
};

} 

BEGIN_WORKERS_NAMESPACE

JSPrincipals*
GetWorkerPrincipal()
{
  return &gPrincipal;
}

END_WORKERS_NAMESPACE
