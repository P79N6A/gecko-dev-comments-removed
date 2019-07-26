




#ifndef mozilla_dom_workers_worker_h__
#define mozilla_dom_workers_worker_h__

#include "Workers.h"


BEGIN_WORKERS_NAMESPACE

namespace worker {

JSObject*
InitClass(JSContext* aCx, JSObject* aGlobal, JSObject* aProto,
          bool aMainRuntime);

} 

namespace chromeworker {

bool
InitClass(JSContext* aCx, JSObject* aGlobal, JSObject* aProto,
          bool aMainRuntime);

} 

bool
ClassIsWorker(const JSClass* aClass);

END_WORKERS_NAMESPACE

#endif 