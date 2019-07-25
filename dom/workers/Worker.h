




#ifndef mozilla_dom_workers_worker_h__
#define mozilla_dom_workers_worker_h__

#include "Workers.h"

#include "jspubtd.h"
#include "mozilla/dom/DOMJSClass.h"

BEGIN_WORKERS_NAMESPACE

extern mozilla::dom::NativePropertyHooks sNativePropertyHooks;

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
ClassIsWorker(JSClass* aClass);

END_WORKERS_NAMESPACE

#endif 
