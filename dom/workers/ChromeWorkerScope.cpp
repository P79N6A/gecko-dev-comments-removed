





































#include "ChromeWorkerScope.h"

#include "jsapi.h"
#include "jscntxt.h"

#include "nsXPCOM.h"
#include "nsNativeCharsetUtils.h"
#include "nsStringGlue.h"

USING_WORKERS_NAMESPACE

namespace {

#ifdef BUILD_CTYPES
char*
UnicodeToNative(JSContext* aCx, const jschar* aSource, size_t aSourceLen)
{
  nsDependentString unicode(aSource, aSourceLen);

  nsCAutoString native;
  if (NS_FAILED(NS_CopyUnicodeToNative(unicode, native))) {
    JS_ReportError(aCx, "Could not convert string to native charset!");
    return nsnull;
  }

  char* result = static_cast<char*>(JS_malloc(aCx, native.Length() + 1));
  if (!result) {
    return nsnull;
  }

  memcpy(result, native.get(), native.Length());
  result[native.Length()] = 0;
  return result;
}

JSCTypesCallbacks gCTypesCallbacks = {
  UnicodeToNative
};
#endif

} 

BEGIN_WORKERS_NAMESPACE

namespace chromeworker {

bool
DefineChromeWorkerFunctions(JSContext* aCx, JSObject* aGlobal)
{
#ifdef BUILD_CTYPES
  jsval ctypes;
  if (!JS_InitCTypesClass(aCx, aGlobal) ||
      !JS_GetProperty(aCx, aGlobal, "ctypes", &ctypes) ||
      !JS_SetCTypesCallbacks(aCx, JSVAL_TO_OBJECT(ctypes), &gCTypesCallbacks)) {
    return false;
  }
#endif

  return true;
}

} 

END_WORKERS_NAMESPACE
