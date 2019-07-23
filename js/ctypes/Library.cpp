







































#include "Library.h"
#include "Function.h"
#include "nsServiceManagerUtils.h"
#include "nsString.h"
#include "nsIXPConnect.h"
#include "nsILocalFile.h"

namespace mozilla {
namespace ctypes {





static JSClass sLibraryClass = {
  "Library",
  JSCLASS_HAS_RESERVED_SLOTS(2),
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
  JS_EnumerateStub,JS_ResolveStub, JS_ConvertStub, Library::Finalize,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

static JSFunctionSpec sLibraryFunctions[] = {
  JS_FN("close",   Library::Close,   0, JSFUN_FAST_NATIVE | JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT),
  JS_FN("declare", Library::Declare, 0, JSFUN_FAST_NATIVE | JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT),
  JS_FS_END
};

JSObject*
Library::Create(JSContext* cx, jsval aPath)
{
  JSObject* libraryObj = JS_NewObject(cx, &sLibraryClass, NULL, NULL);
  if (!libraryObj)
    return NULL;

  
  if (!JS_DefineFunctions(cx, libraryObj, sLibraryFunctions))
    return NULL;

  nsresult rv;
  nsCOMPtr<nsILocalFile> localFile;

  
  
  if (JSVAL_IS_STRING(aPath)) {
    const jschar* path = JS_GetStringChars(JSVAL_TO_STRING(aPath));
    if (!path)
      return NULL;

    localFile = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID);
    if (!localFile)
      return NULL;

    rv = localFile->InitWithPath(nsDependentString(reinterpret_cast<const PRUnichar*>(path)));
    if (NS_FAILED(rv))
      return NULL;

  } else if (JSVAL_IS_OBJECT(aPath)) {
    nsCOMPtr<nsIXPConnect> xpc = do_GetService(nsIXPConnect::GetCID());

    nsISupports* file = xpc->GetNativeOfWrapper(cx, JSVAL_TO_OBJECT(aPath));
    localFile = do_QueryInterface(file);
    if (!localFile)
      return NULL;

  } else {
    
    return NULL;
  }

  PRLibrary* library;
  rv = localFile->Load(&library);
  if (NS_FAILED(rv))
    return NULL;

  
  if (!JS_SetReservedSlot(cx, libraryObj, 0, PRIVATE_TO_JSVAL(library)))
    return NULL;

  
  if (!JS_SetReservedSlot(cx, libraryObj, 1, PRIVATE_TO_JSVAL(NULL)))
    return NULL;

  return libraryObj;
}

PRLibrary*
Library::GetLibrary(JSContext* cx, JSObject* obj)
{
  JS_ASSERT(JS_GET_CLASS(cx, obj) == &sLibraryClass);

  jsval slot;
  JS_GetReservedSlot(cx, obj, 0, &slot);
  return static_cast<PRLibrary*>(JSVAL_TO_PRIVATE(slot));
}

static Function*
GetFunctionList(JSContext* cx, JSObject* obj)
{
  JS_ASSERT(JS_GET_CLASS(cx, obj) == &sLibraryClass);

  jsval slot;
  JS_GetReservedSlot(cx, obj, 1, &slot);
  return static_cast<Function*>(JSVAL_TO_PRIVATE(slot));
}

bool
Library::AddFunction(JSContext* cx, JSObject* aLibrary, Function* aFunction)
{
  
  aFunction->Next() = GetFunctionList(cx, aLibrary);
  return JS_SetReservedSlot(cx, aLibrary, 1, PRIVATE_TO_JSVAL(aFunction));
}

void
Library::Finalize(JSContext* cx, JSObject* obj)
{
  
  PRLibrary* library = GetLibrary(cx, obj);
  if (library)
    PR_UnloadLibrary(library);

  
  Function* current = GetFunctionList(cx, obj);
  while (current) {
    Function* next = current->Next();
    delete current;
    current = next;
  }
}

JSBool
Library::Open(JSContext* cx, uintN argc, jsval *vp)
{
  if (argc != 1) {
    JS_ReportError(cx, "open requires a single argument");
    return JS_FALSE;
  }

  JSObject* library = Create(cx, JS_ARGV(cx, vp)[0]);
  if (!library) {
    JS_ReportError(cx, "couldn't open library");
    return JS_FALSE;
  }

  JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(library));
  return JS_TRUE;
}

JSBool
Library::Close(JSContext* cx, uintN argc, jsval* vp)
{
  JSObject* obj = JS_THIS_OBJECT(cx, vp);
  if (JS_GET_CLASS(cx, obj) != &sLibraryClass) {
    JS_ReportError(cx, "not a library");
    return JS_FALSE;
  }

  if (argc != 0) {
    JS_ReportError(cx, "close doesn't take any arguments");
    return JS_FALSE;
  }

  
  Finalize(cx, obj);
  JS_SetReservedSlot(cx, obj, 0, PRIVATE_TO_JSVAL(NULL));
  JS_SetReservedSlot(cx, obj, 1, PRIVATE_TO_JSVAL(NULL));

  JS_SET_RVAL(cx, vp, JSVAL_VOID);
  return JS_TRUE;
}

JSBool
Library::Declare(JSContext* cx, uintN argc, jsval* vp)
{
  JSObject* obj = JS_THIS_OBJECT(cx, vp);
  if (JS_GET_CLASS(cx, obj) != &sLibraryClass) {
    JS_ReportError(cx, "not a library");
    return JS_FALSE;
  }

  PRLibrary* library = GetLibrary(cx, obj);
  if (!library) {
    JS_ReportError(cx, "library not open");
    return JS_FALSE;
  }

  
  if (argc < 3) {
    JS_ReportError(cx, "declare requires at least three arguments");
    return JS_FALSE;
  }

  jsval* argv = JS_ARGV(cx, vp);
  if (!JSVAL_IS_STRING(argv[0])) {
    JS_ReportError(cx, "first argument must be a string");
    return JS_FALSE;
  }

  const char* name = JS_GetStringBytes(JSVAL_TO_STRING(argv[0]));
  PRFuncPtr func = PR_FindFunctionSymbol(library, name);
  if (!func) {
    JS_ReportError(cx, "couldn't find function symbol in library");
    return JS_FALSE;
  }

  JSObject* fn = Function::Create(cx, obj, func, name, argv[1], argv[2], &argv[3], argc - 3);
  if (!fn)
    return JS_FALSE;

  JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(fn));
  return JS_TRUE;
}

}
}

