




#ifndef mozilla_dom_workers_workerscope_h__
#define mozilla_dom_workers_workerscope_h__

#include "Workers.h"

BEGIN_WORKERS_NAMESPACE

JSObject*
CreateDedicatedWorkerGlobalScope(JSContext* aCx);

bool
ClassIsWorkerGlobalScope(const JSClass* aClass);

END_WORKERS_NAMESPACE

#endif 
