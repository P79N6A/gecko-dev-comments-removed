







































#include "jscntxt.h"
#include "Library.h"
#include "CTypes.h"
#include "nsServiceManagerUtils.h"
#include "nsIXPConnect.h"
#include "nsILocalFile.h"
#include "nsNativeCharsetUtils.h"

namespace mozilla {
namespace ctypes {





namespace Library
{
  static void Finalize(JSContext* cx, JSObject* obj);

  static JSBool Close(JSContext* cx, uintN argc, jsval* vp);
  static JSBool Declare(JSContext* cx, uintN argc, jsval* vp);
}





static JSClass sLibraryClass = {
  "Library",
  JSCLASS_HAS_RESERVED_SLOTS(LIBRARY_SLOTS) | JSCLASS_MARK_IS_TRACE,
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
  JS_EnumerateStub,JS_ResolveStub, JS_ConvertStub, Library::Finalize,
  JSCLASS_NO_OPTIONAL_MEMBERS
};

#define CTYPESFN_FLAGS \
  (JSFUN_FAST_NATIVE | JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT)

static JSFunctionSpec sLibraryFunctions[] = {
  JS_FN("close",   Library::Close,   0, CTYPESFN_FLAGS),
  JS_FN("declare", Library::Declare, 0, CTYPESFN_FLAGS),
  JS_FS_END
};

JSObject*
Library::Create(JSContext* cx, jsval aPath)
{
  JSObject* libraryObj = JS_NewObject(cx, &sLibraryClass, NULL, NULL);
  if (!libraryObj)
    return NULL;
  JSAutoTempValueRooter root(cx, libraryObj);

  
  if (!JS_SetReservedSlot(cx, libraryObj, SLOT_LIBRARY, PRIVATE_TO_JSVAL(NULL)))
    return NULL;

  
  if (!JS_DefineFunctions(cx, libraryObj, sLibraryFunctions))
    return NULL;

  nsresult rv;
  PRLibrary* library;

  
  
  if (JSVAL_IS_STRING(aPath)) {
    const PRUnichar* path = reinterpret_cast<const PRUnichar*>(
      JS_GetStringCharsZ(cx, JSVAL_TO_STRING(aPath)));
    if (!path)
      return NULL;

    
    
    PRLibSpec libSpec;
#ifdef XP_WIN
    
    
    libSpec.value.pathname_u = path;
    libSpec.type = PR_LibSpec_PathnameU;
#else
    nsCAutoString nativePath;
    NS_CopyUnicodeToNative(nsDependentString(path), nativePath);
    libSpec.value.pathname = nativePath.get();
    libSpec.type = PR_LibSpec_Pathname;
#endif
    library = PR_LoadLibraryWithFlags(libSpec, 0);
    if (!library) {
      JS_ReportError(cx, "couldn't open library");
      return NULL;
    }

  } else if (!JSVAL_IS_PRIMITIVE(aPath)) {
    nsCOMPtr<nsIXPConnect> xpc = do_GetService(nsIXPConnect::GetCID());

    nsISupports* file = xpc->GetNativeOfWrapper(cx, JSVAL_TO_OBJECT(aPath));
    nsCOMPtr<nsILocalFile> localFile = do_QueryInterface(file);
    if (!localFile) {
      JS_ReportError(cx, "open takes a string or nsILocalFile argument");
      return NULL;
    }

    rv = localFile->Load(&library);
    if (NS_FAILED(rv)) {
      JS_ReportError(cx, "couldn't open library");
      return NULL;
    }

  } else {
    
    JS_ReportError(cx, "open takes a string or nsIFile argument");
    return NULL;
  }

  
  if (!JS_SetReservedSlot(cx, libraryObj, SLOT_LIBRARY,
         PRIVATE_TO_JSVAL(library)))
    return NULL;

  return libraryObj;
}

bool
Library::IsLibrary(JSContext* cx, JSObject* obj)
{
  return JS_GET_CLASS(cx, obj) == &sLibraryClass;
}

PRLibrary*
Library::GetLibrary(JSContext* cx, JSObject* obj)
{
  JS_ASSERT(IsLibrary(cx, obj));

  jsval slot;
  JS_GetReservedSlot(cx, obj, SLOT_LIBRARY, &slot);
  return static_cast<PRLibrary*>(JSVAL_TO_PRIVATE(slot));
}

void
Library::Finalize(JSContext* cx, JSObject* obj)
{
  
  PRLibrary* library = GetLibrary(cx, obj);
  if (library)
    PR_UnloadLibrary(library);
}

JSBool
Library::Open(JSContext* cx, uintN argc, jsval *vp)
{
  if (argc != 1 || JSVAL_IS_VOID(JS_ARGV(cx, vp)[0])) {
    JS_ReportError(cx, "open requires a single argument");
    return JS_FALSE;
  }

  JSObject* library = Create(cx, JS_ARGV(cx, vp)[0]);
  if (!library)
    return JS_FALSE;

  JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(library));
  return JS_TRUE;
}

JSBool
Library::Close(JSContext* cx, uintN argc, jsval* vp)
{
  JSObject* obj = JS_THIS_OBJECT(cx, vp);
  if (!IsLibrary(cx, obj)) {
    JS_ReportError(cx, "not a library");
    return JS_FALSE;
  }

  if (argc != 0) {
    JS_ReportError(cx, "close doesn't take any arguments");
    return JS_FALSE;
  }

  
  Finalize(cx, obj);
  JS_SetReservedSlot(cx, obj, SLOT_LIBRARY, PRIVATE_TO_JSVAL(NULL));

  JS_SET_RVAL(cx, vp, JSVAL_VOID);
  return JS_TRUE;
}

JSBool
Library::Declare(JSContext* cx, uintN argc, jsval* vp)
{
  JSObject* obj = JS_THIS_OBJECT(cx, vp);
  if (!IsLibrary(cx, obj)) {
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

  const char* name = JS_GetStringBytesZ(cx, JSVAL_TO_STRING(argv[0]));
  if (!name)
    return JS_FALSE;

  PRFuncPtr func = PR_FindFunctionSymbol(library, name);
  if (!func) {
    JS_ReportError(cx, "couldn't find function symbol in library");
    return JS_FALSE;
  }

  
  JSObject* typeObj = FunctionType::CreateInternal(cx,
                        argv[1], argv[2], &argv[3], argc - 3);
  if (!typeObj)
    return JS_FALSE;
  JSAutoTempValueRooter root(cx, typeObj);

  JSObject* fn = CData::Create(cx, typeObj, obj, &func, true);
  if (!fn)
    return JS_FALSE;

  JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(fn));

  
  
  
  
  
  
  return JS_SealObject(cx, fn, JS_FALSE);
}

}
}

