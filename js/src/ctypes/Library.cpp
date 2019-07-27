





#include "ctypes/Library.h"

#include "prerror.h"
#include "prlink.h"

#include "ctypes/CTypes.h"

namespace js {
namespace ctypes {





namespace Library
{
  static void Finalize(JSFreeOp* fop, JSObject* obj);

  static bool Close(JSContext* cx, unsigned argc, jsval* vp);
  static bool Declare(JSContext* cx, unsigned argc, jsval* vp);
}





typedef Rooted<JSFlatString*>    RootedFlatString;

static const JSClass sLibraryClass = {
  "Library",
  JSCLASS_HAS_RESERVED_SLOTS(LIBRARY_SLOTS),
  nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, Library::Finalize
};

#define CTYPESFN_FLAGS \
  (JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT)

static const JSFunctionSpec sLibraryFunctions[] = {
  JS_FN("close",   Library::Close,   0, CTYPESFN_FLAGS),
  JS_FN("declare", Library::Declare, 0, CTYPESFN_FLAGS),
  JS_FS_END
};

bool
Library::Name(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  if (args.length() != 1) {
    JS_ReportError(cx, "libraryName takes one argument");
    return false;
  }

  Value arg = args[0];
  JSString* str = nullptr;
  if (arg.isString()) {
    str = arg.toString();
  } else {
    JS_ReportError(cx, "name argument must be a string");
    return false;
  }

  AutoString resultString;
  AppendString(resultString, DLL_PREFIX);
  AppendString(resultString, str);
  AppendString(resultString, DLL_SUFFIX);

  JSString* result = JS_NewUCStringCopyN(cx, resultString.begin(),
                                         resultString.length());
  if (!result)
    return false;

  args.rval().setString(result);
  return true;
}

JSObject*
Library::Create(JSContext* cx, jsval path_, const JSCTypesCallbacks* callbacks)
{
  RootedValue path(cx, path_);
  RootedObject libraryObj(cx, JS_NewObject(cx, &sLibraryClass));
  if (!libraryObj)
    return nullptr;

  
  JS_SetReservedSlot(libraryObj, SLOT_LIBRARY, PRIVATE_TO_JSVAL(nullptr));

  
  if (!JS_DefineFunctions(cx, libraryObj, sLibraryFunctions))
    return nullptr;

  if (!path.isString()) {
    JS_ReportError(cx, "open takes a string argument");
    return nullptr;
  }

  PRLibSpec libSpec;
  RootedFlatString pathStr(cx, JS_FlattenString(cx, path.toString()));
  if (!pathStr)
    return nullptr;
  AutoStableStringChars pathStrChars(cx);
  if (!pathStrChars.initTwoByte(cx, pathStr))
    return nullptr;
#ifdef XP_WIN
  
  
  char16ptr_t pathChars = pathStrChars.twoByteChars();
  libSpec.value.pathname_u = pathChars;
  libSpec.type = PR_LibSpec_PathnameU;
#else
  
  
  char* pathBytes;
  if (callbacks && callbacks->unicodeToNative) {
    pathBytes =
      callbacks->unicodeToNative(cx, pathStrChars.twoByteChars(), pathStr->length());
    if (!pathBytes)
      return nullptr;

  } else {
    
    
    size_t nbytes =
      GetDeflatedUTF8StringLength(cx, pathStrChars.twoByteChars(), pathStr->length());
    if (nbytes == (size_t) -1)
      return nullptr;

    pathBytes = static_cast<char*>(JS_malloc(cx, nbytes + 1));
    if (!pathBytes)
      return nullptr;

    ASSERT_OK(DeflateStringToUTF8Buffer(cx, pathStrChars.twoByteChars(),
                pathStr->length(), pathBytes, &nbytes));
    pathBytes[nbytes] = 0;
  }

  libSpec.value.pathname = pathBytes;
  libSpec.type = PR_LibSpec_Pathname;
#endif

  PRLibrary* library = PR_LoadLibraryWithFlags(libSpec, 0);

  if (!library) {
    char* error = (char*) JS_malloc(cx, PR_GetErrorTextLength() + 1);
    if (error)
      PR_GetErrorText(error);

#ifdef XP_WIN
    JS_ReportError(cx, "couldn't open library %hs: %s", pathChars, error);
#else
    JS_ReportError(cx, "couldn't open library %s: %s", pathBytes, error);
    JS_free(cx, pathBytes);
#endif
    JS_free(cx, error);
    return nullptr;
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
  MOZ_ASSERT(IsLibrary(obj));

  jsval slot = JS_GetReservedSlot(obj, SLOT_LIBRARY);
  return static_cast<PRLibrary*>(slot.toPrivate());
}

static void
UnloadLibrary(JSObject* obj)
{
  PRLibrary* library = Library::GetLibrary(obj);
  if (library)
    PR_UnloadLibrary(library);
}

void
Library::Finalize(JSFreeOp* fop, JSObject* obj)
{
  UnloadLibrary(obj);
}

bool
Library::Open(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  JSObject* ctypesObj = JS_THIS_OBJECT(cx, vp);
  if (!ctypesObj)
    return false;
  if (!IsCTypesGlobal(ctypesObj)) {
    JS_ReportError(cx, "not a ctypes object");
    return false;
  }

  if (args.length() != 1 || args[0].isUndefined()) {
    JS_ReportError(cx, "open requires a single argument");
    return false;
  }

  JSObject* library = Create(cx, args[0], GetCallbacks(ctypesObj));
  if (!library)
    return false;

  args.rval().setObject(*library);
  return true;
}

bool
Library::Close(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  JSObject* obj = JS_THIS_OBJECT(cx, vp);
  if (!obj)
    return false;
  if (!IsLibrary(obj)) {
    JS_ReportError(cx, "not a library");
    return false;
  }

  if (args.length() != 0) {
    JS_ReportError(cx, "close doesn't take any arguments");
    return false;
  }

  
  UnloadLibrary(obj);
  JS_SetReservedSlot(obj, SLOT_LIBRARY, PRIVATE_TO_JSVAL(nullptr));

  args.rval().setUndefined();
  return true;
}

bool
Library::Declare(JSContext* cx, unsigned argc, jsval* vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  RootedObject obj(cx, JS_THIS_OBJECT(cx, vp));
  if (!obj)
    return false;
  if (!IsLibrary(obj)) {
    JS_ReportError(cx, "not a library");
    return false;
  }

  PRLibrary* library = GetLibrary(obj);
  if (!library) {
    JS_ReportError(cx, "library not open");
    return false;
  }

  
  
  
  
  
  
  
  
  
  
  if (args.length() < 2) {
    JS_ReportError(cx, "declare requires at least two arguments");
    return false;
  }

  if (!args[0].isString()) {
    JS_ReportError(cx, "first argument must be a string");
    return false;
  }

  RootedObject fnObj(cx, nullptr);
  RootedObject typeObj(cx);
  bool isFunction = args.length() > 2;
  if (isFunction) {
    
    
    fnObj = FunctionType::CreateInternal(cx, args[1], args[2],
                                         HandleValueArray::subarray(args, 3, args.length() - 3));
    if (!fnObj)
      return false;

    
    typeObj = PointerType::CreateInternal(cx, fnObj);
    if (!typeObj)
      return false;
  } else {
    
    if (args[1].isPrimitive() ||
        !CType::IsCType(args[1].toObjectOrNull()) ||
        !CType::IsSizeDefined(args[1].toObjectOrNull())) {
      JS_ReportError(cx, "second argument must be a type of defined size");
      return false;
    }

    typeObj = args[1].toObjectOrNull();
    if (CType::GetTypeCode(typeObj) == TYPE_pointer) {
      fnObj = PointerType::GetBaseType(typeObj);
      isFunction = fnObj && CType::GetTypeCode(fnObj) == TYPE_function;
    }
  }

  void* data;
  PRFuncPtr fnptr;
  RootedString nameStr(cx, args[0].toString());
  AutoCString symbol;
  if (isFunction) {
    
    FunctionType::BuildSymbolName(nameStr, fnObj, symbol);
    AppendString(symbol, "\0");

    
    fnptr = PR_FindFunctionSymbol(library, symbol.begin());
    if (!fnptr) {
      JS_ReportError(cx, "couldn't find function symbol in library");
      return false;
    }
    data = &fnptr;

  } else {
    
    AppendString(symbol, nameStr);
    AppendString(symbol, "\0");

    data = PR_FindSymbol(library, symbol.begin());
    if (!data) {
      JS_ReportError(cx, "couldn't find symbol in library");
      return false;
    }
  }

  RootedObject result(cx, CData::Create(cx, typeObj, obj, data, isFunction));
  if (!result)
    return false;

  if (isFunction)
    JS_SetReservedSlot(result, SLOT_FUNNAME, StringValue(nameStr));

  args.rval().setObject(*result);

  
  
  
  
  
  
  if (isFunction && !JS_FreezeObject(cx, result))
    return false;

  return true;
}

}
}

