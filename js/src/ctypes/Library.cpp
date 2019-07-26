




#include "jscntxt.h"
#include "jsstr.h"
#include "Library.h"
#include "CTypes.h"
#include "prlink.h"

namespace js {
namespace ctypes {





namespace Library
{
  static void Finalize(JSFreeOp *fop, JSObject* obj);

  static JSBool Close(JSContext* cx, unsigned argc, jsval* vp);
  static JSBool Declare(JSContext* cx, unsigned argc, jsval* vp);
}





typedef Rooted<JSFlatString*>    RootedFlatString;

static JSClass sLibraryClass = {
  "Library",
  JSCLASS_HAS_RESERVED_SLOTS(LIBRARY_SLOTS),
  JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
  JS_EnumerateStub,JS_ResolveStub, JS_ConvertStub, Library::Finalize
};

#define CTYPESFN_FLAGS \
  (JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT)

static JSFunctionSpec sLibraryFunctions[] = {
  JS_FN("close",   Library::Close,   0, CTYPESFN_FLAGS),
  JS_FN("declare", Library::Declare, 0, CTYPESFN_FLAGS),
  JS_FS_END
};

JSBool
Library::Name(JSContext* cx, unsigned argc, jsval *vp)
{
  if (argc != 1) {
    JS_ReportError(cx, "libraryName takes one argument");
    return JS_FALSE;
  }

  jsval arg = JS_ARGV(cx, vp)[0];
  JSString* str = NULL;
  if (JSVAL_IS_STRING(arg)) {
    str = JSVAL_TO_STRING(arg);
  }
  else {
    JS_ReportError(cx, "name argument must be a string");
      return JS_FALSE;
  }

  AutoString resultString;
  AppendString(resultString, DLL_PREFIX);
  AppendString(resultString, str);
  AppendString(resultString, DLL_SUFFIX);

  JSString *result = JS_NewUCStringCopyN(cx, resultString.begin(),
                                         resultString.length());
  if (!result)
    return JS_FALSE;

  JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(result));
  return JS_TRUE;
}

JSObject*
Library::Create(JSContext* cx, jsval path_, JSCTypesCallbacks* callbacks)
{
  RootedValue path(cx, path_);
  RootedObject libraryObj(cx, JS_NewObject(cx, &sLibraryClass, NULL, NULL));
  if (!libraryObj)
    return NULL;

  
  JS_SetReservedSlot(libraryObj, SLOT_LIBRARY, PRIVATE_TO_JSVAL(NULL));

  
  if (!JS_DefineFunctions(cx, libraryObj, sLibraryFunctions))
    return NULL;

  if (!JSVAL_IS_STRING(path)) {
    JS_ReportError(cx, "open takes a string argument");
    return NULL;
  }

  PRLibSpec libSpec;
  RootedFlatString pathStr(cx, JS_FlattenString(cx, JSVAL_TO_STRING(path)));
  if (!pathStr)
    return NULL;
#ifdef XP_WIN
  
  
  const PRUnichar* pathChars = JS_GetFlatStringChars(pathStr);
  if (!pathChars)
    return NULL;

  libSpec.value.pathname_u = pathChars;
  libSpec.type = PR_LibSpec_PathnameU;
#else
  
  
  char* pathBytes;
  if (callbacks && callbacks->unicodeToNative) {
    pathBytes = 
      callbacks->unicodeToNative(cx, pathStr->chars(), pathStr->length());
    if (!pathBytes)
      return NULL;

  } else {
    
    
    size_t nbytes =
      GetDeflatedUTF8StringLength(cx, pathStr->chars(), pathStr->length());
    if (nbytes == (size_t) -1)
      return NULL;

    pathBytes = static_cast<char*>(JS_malloc(cx, nbytes + 1));
    if (!pathBytes)
      return NULL;

    ASSERT_OK(DeflateStringToUTF8Buffer(cx, pathStr->chars(),
                pathStr->length(), pathBytes, &nbytes));
    pathBytes[nbytes] = 0;
  }

  libSpec.value.pathname = pathBytes;
  libSpec.type = PR_LibSpec_Pathname;
#endif

  PRLibrary* library = PR_LoadLibraryWithFlags(libSpec, 0);

  if (!library) {
#ifdef XP_WIN
    JS_ReportError(cx, "couldn't open library %hs", pathChars);
#else
    JS_ReportError(cx, "couldn't open library %s", pathBytes);
    JS_free(cx, pathBytes);
#endif
    return NULL;
  }

#ifndef XP_WIN
  JS_free(cx, pathBytes);
#endif

  
  JS_SetReservedSlot(libraryObj, SLOT_LIBRARY, PRIVATE_TO_JSVAL(library));

  return libraryObj;
}

bool
Library::IsLibrary(JSObject* obj)
{
  return JS_GetClass(obj) == &sLibraryClass;
}

PRLibrary*
Library::GetLibrary(JSObject* obj)
{
  JS_ASSERT(IsLibrary(obj));

  jsval slot = JS_GetReservedSlot(obj, SLOT_LIBRARY);
  return static_cast<PRLibrary*>(JSVAL_TO_PRIVATE(slot));
}

static void
UnloadLibrary(JSObject* obj)
{
  PRLibrary* library = Library::GetLibrary(obj);
  if (library)
    PR_UnloadLibrary(library);
}

void
Library::Finalize(JSFreeOp *fop, JSObject* obj)
{
  UnloadLibrary(obj);
}

JSBool
Library::Open(JSContext* cx, unsigned argc, jsval *vp)
{
  JSObject* ctypesObj = JS_THIS_OBJECT(cx, vp);
  if (!ctypesObj)
    return JS_FALSE;
  if (!IsCTypesGlobal(ctypesObj)) {
    JS_ReportError(cx, "not a ctypes object");
    return JS_FALSE;
  }

  if (argc != 1 || JSVAL_IS_VOID(JS_ARGV(cx, vp)[0])) {
    JS_ReportError(cx, "open requires a single argument");
    return JS_FALSE;
  }

  JSObject* library = Create(cx, JS_ARGV(cx, vp)[0], GetCallbacks(ctypesObj));
  if (!library)
    return JS_FALSE;

  JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(library));
  return JS_TRUE;
}

JSBool
Library::Close(JSContext* cx, unsigned argc, jsval* vp)
{
  JSObject* obj = JS_THIS_OBJECT(cx, vp);
  if (!obj)
    return JS_FALSE;
  if (!IsLibrary(obj)) {
    JS_ReportError(cx, "not a library");
    return JS_FALSE;
  }

  if (argc != 0) {
    JS_ReportError(cx, "close doesn't take any arguments");
    return JS_FALSE;
  }

  
  UnloadLibrary(obj);
  JS_SetReservedSlot(obj, SLOT_LIBRARY, PRIVATE_TO_JSVAL(NULL));

  JS_SET_RVAL(cx, vp, JSVAL_VOID);
  return JS_TRUE;
}

JSBool
Library::Declare(JSContext* cx, unsigned argc, jsval* vp)
{
  RootedObject obj(cx, JS_THIS_OBJECT(cx, vp));
  if (!obj)
    return JS_FALSE;
  if (!IsLibrary(obj)) {
    JS_ReportError(cx, "not a library");
    return JS_FALSE;
  }

  PRLibrary* library = GetLibrary(obj);
  if (!library) {
    JS_ReportError(cx, "library not open");
    return JS_FALSE;
  }

  
  
  
  
  
  
  
  
  
  
  if (argc < 2) {
    JS_ReportError(cx, "declare requires at least two arguments");
    return JS_FALSE;
  }

  jsval* argv = JS_ARGV(cx, vp);
  if (!JSVAL_IS_STRING(argv[0])) {
    JS_ReportError(cx, "first argument must be a string");
    return JS_FALSE;
  }

  RootedObject fnObj(cx, NULL);
  RootedObject typeObj(cx);
  bool isFunction = argc > 2;
  if (isFunction) {
    
    
    fnObj = FunctionType::CreateInternal(cx,
              argv[1], argv[2], &argv[3], argc - 3);
    if (!fnObj)
      return JS_FALSE;

    
    typeObj = PointerType::CreateInternal(cx, fnObj);
    if (!typeObj)
      return JS_FALSE;
  } else {
    
    if (JSVAL_IS_PRIMITIVE(argv[1]) ||
        !CType::IsCType(JSVAL_TO_OBJECT(argv[1])) ||
        !CType::IsSizeDefined(JSVAL_TO_OBJECT(argv[1]))) {
      JS_ReportError(cx, "second argument must be a type of defined size");
      return JS_FALSE;
    }

    typeObj = JSVAL_TO_OBJECT(argv[1]);
    if (CType::GetTypeCode(typeObj) == TYPE_pointer) {
      fnObj = PointerType::GetBaseType(typeObj);
      isFunction = fnObj && CType::GetTypeCode(fnObj) == TYPE_function;
    }
  }

  void* data;
  PRFuncPtr fnptr;
  JSString* nameStr = JSVAL_TO_STRING(argv[0]);
  AutoCString symbol;
  if (isFunction) {
    
    FunctionType::BuildSymbolName(nameStr, fnObj, symbol);
    AppendString(symbol, "\0");

    
    fnptr = PR_FindFunctionSymbol(library, symbol.begin());
    if (!fnptr) {
      JS_ReportError(cx, "couldn't find function symbol in library");
      return JS_FALSE;
    }
    data = &fnptr;

  } else {
    
    AppendString(symbol, nameStr);
    AppendString(symbol, "\0");

    data = PR_FindSymbol(library, symbol.begin());
    if (!data) {
      JS_ReportError(cx, "couldn't find symbol in library");
      return JS_FALSE;
    }
  }

  RootedObject result(cx, CData::Create(cx, typeObj, obj, data, isFunction));
  if (!result)
    return JS_FALSE;

  JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(result));

  
  
  
  
  
  
  if (isFunction && !JS_FreezeObject(cx, result))
    return JS_FALSE;

  return JS_TRUE;
}

}
}

