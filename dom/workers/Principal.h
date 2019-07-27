




#ifndef mozilla_dom_workers_principal_h__
#define mozilla_dom_workers_principal_h__

#include "Workers.h"

BEGIN_WORKERS_NAMESPACE

JSPrincipals*
GetWorkerPrincipal();

void
DestroyWorkerPrincipals(JSPrincipals* aPrincipals);

END_WORKERS_NAMESPACE

#endif 
