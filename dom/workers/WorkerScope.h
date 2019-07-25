





































#ifndef mozilla_dom_workers_workerscope_h__
#define mozilla_dom_workers_workerscope_h__

#include "Workers.h"

#include "jspubtd.h"

BEGIN_WORKERS_NAMESPACE

JSObject*
CreateDedicatedWorkerGlobalScope(JSContext* aCx);

bool
ClassIsWorkerGlobalScope(JSClass* aClass);

END_WORKERS_NAMESPACE

#endif 
