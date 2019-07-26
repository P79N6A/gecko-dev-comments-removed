




#ifndef mozilla_dom_workers_chromeworkerscope_h__
#define mozilla_dom_workers_chromeworkerscope_h__

#include "Workers.h"

BEGIN_WORKERS_NAMESPACE

bool
DefineChromeWorkerFunctions(JSContext* aCx, JS::Handle<JSObject*> aGlobal);

END_WORKERS_NAMESPACE

#endif 
