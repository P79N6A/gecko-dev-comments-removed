





































#ifndef mozilla_dom_workers_worker_h__
#define mozilla_dom_workers_worker_h__

#include "Workers.h"

#include "jspubtd.h"

BEGIN_WORKERS_NAMESPACE

namespace worker {

JSObject*
InitClass(JSContext* aCx, JSObject* aGlobal, JSObject* aProto,
          bool aMainRuntime);

void
ClearPrivateSlot(JSContext* aCx, JSObject* aObj, bool aSaveEventHandlers);

} 

namespace chromeworker {

bool
InitClass(JSContext* aCx, JSObject* aGlobal, JSObject* aProto,
          bool aMainRuntime);

} 

END_WORKERS_NAMESPACE

#endif 
