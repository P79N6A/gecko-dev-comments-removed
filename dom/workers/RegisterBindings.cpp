




#include "WorkerPrivate.h"
#include "ChromeWorkerScope.h"
#include "File.h"
#include "RuntimeService.h"

#include "jsapi.h"
#include "js/OldDebugAPI.h"
#include "mozilla/dom/RegisterWorkerBindings.h"
#include "mozilla/OSFileConstants.h"

USING_WORKERS_NAMESPACE
using namespace mozilla::dom;

bool
WorkerPrivate::RegisterBindings(JSContext* aCx, JS::Handle<JSObject*> aGlobal)
{
  
  if (!RegisterWorkerBindings(aCx, aGlobal)) {
    return false;
  }

  if (IsChromeWorker()) {
    if (!DefineChromeWorkerFunctions(aCx, aGlobal) ||
        !DefineOSFileConstants(aCx, aGlobal)) {
      return false;
    }
  }

  
  if (!file::InitClasses(aCx, aGlobal)) {
    return false;
  }

  if (!JS_DefineProfilingFunctions(aCx, aGlobal)) {
    return false;
  }

  return true;
}
