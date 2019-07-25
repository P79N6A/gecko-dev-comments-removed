





































#ifndef mozilla_dom_workers_chromeworkerscope_h__
#define mozilla_dom_workers_chromeworkerscope_h__

#include "Workers.h"

#include "jspubtd.h"

BEGIN_WORKERS_NAMESPACE

namespace chromeworker {

bool
DefineChromeWorkerFunctions(JSContext* aCx, JSObject* aGlobal);

} 

END_WORKERS_NAMESPACE

#endif 
