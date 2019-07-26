




#include "ChromeWorkerScope.h"

#include "jsapi.h"

#include "nsXPCOM.h"
#include "nsNativeCharsetUtils.h"
#include "nsStringGlue.h"

#include "WorkerPrivate.h"

using namespace mozilla::dom;
USING_WORKERS_NAMESPACE

namespace {

#ifdef BUILD_CTYPES

char*
UnicodeToNative(JSContext* aCx, const jschar* aSource, size_t aSourceLen)
{
  nsDependentString unicode(aSource, aSourceLen);

  nsAutoCString native;
  if (NS_FAILED(NS_CopyUnicodeToNative(unicode, native))) {
    JS_ReportError(aCx, "Could not convert string to native charset!");
    return nullptr;
  }

  char* result = static_cast<char*>(JS_malloc(aCx, native.Length() + 1));
  if (!result) {
    return nullptr;
  }

  memcpy(result, native.get(), native.Length());
  result[native.Length()] = 0;
  return result;
}

#endif 

} 

BEGIN_WORKERS_NAMESPACE

bool
DefineChromeWorkerFunctions(JSContext* aCx, JS::Handle<JSObject*> aGlobal)
{
  
#ifdef BUILD_CTYPES
  {
    jsval ctypes;
    if (!JS_InitCTypesClass(aCx, aGlobal) ||
        !JS_GetProperty(aCx, aGlobal, "ctypes", &ctypes)) {
      return false;
    }

    static JSCTypesCallbacks callbacks = {
      UnicodeToNative
    };

    JS_SetCTypesCallbacks(JSVAL_TO_OBJECT(ctypes), &callbacks);
  }
#endif 

  return true;
}

END_WORKERS_NAMESPACE
