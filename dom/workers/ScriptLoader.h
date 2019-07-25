





































#ifndef mozilla_dom_workers_scriptloader_h__
#define mozilla_dom_workers_scriptloader_h__

#include "Workers.h"

#include "jsapi.h"

BEGIN_WORKERS_NAMESPACE

namespace scriptloader {

bool LoadWorkerScript(JSContext* aCx);

bool Load(JSContext* aCx, uintN aURLCount, jsval* aURLs);

} 

END_WORKERS_NAMESPACE

#endif 
