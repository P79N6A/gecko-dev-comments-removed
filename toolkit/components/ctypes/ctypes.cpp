






































#include "ctypes.h"
#include "jsapi.h"
#include "mozilla/ModuleUtils.h"
#include "nsMemory.h"

#define JSCTYPES_CONTRACTID \
  "@mozilla.org/jsctypes;1"

#define JSCTYPES_CID \
{ 0xc797702, 0x1c60, 0x4051, { 0x9d, 0xd7, 0x4d, 0x74, 0x5, 0x60, 0x56, 0x42 } }

namespace mozilla {
namespace ctypes {

NS_GENERIC_FACTORY_CONSTRUCTOR(Module)

NS_IMPL_ISUPPORTS1(Module, nsIXPCScriptable)

Module::Module()
{
}

Module::~Module()
{
}

#define XPC_MAP_CLASSNAME Module
#define XPC_MAP_QUOTED_CLASSNAME "Module"
#define XPC_MAP_WANT_CALL
#define XPC_MAP_FLAGS nsIXPCScriptable::WANT_CALL
#include "xpc_map_end.h"

static JSBool
SealObjectAndPrototype(JSContext* cx, JSObject* parent, const char* name)
{
  jsval prop;
  if (!JS_GetProperty(cx, parent, name, &prop))
    return false;

  JSObject* obj = JSVAL_TO_OBJECT(prop);
  if (!JS_GetProperty(cx, obj, "prototype", &prop))
    return false;

  JSObject* prototype = JSVAL_TO_OBJECT(prop);
  return JS_SealObject(cx, obj, JS_FALSE) &&
         JS_SealObject(cx, prototype, JS_FALSE);
}

static JSBool
InitAndSealCTypesClass(JSContext* cx, JSObject* global)
{
  
  if (!JS_InitCTypesClass(cx, global))
    return false;

  
  
  if (!SealObjectAndPrototype(cx, global, "Object") ||
      !SealObjectAndPrototype(cx, global, "Function") ||
      !SealObjectAndPrototype(cx, global, "Array"))
    return false;

  
  
  return JS_SealObject(cx, global, JS_FALSE);
}

NS_IMETHODIMP
Module::Call(nsIXPConnectWrappedNative* wrapper,
             JSContext* cx,
             JSObject* obj,
             PRUint32 argc,
             jsval* argv,
             jsval* vp,
             PRBool* _retval)
{
  JSObject* global = JS_GetGlobalForScopeChain(cx);
  if (!global)
    return NS_ERROR_NOT_AVAILABLE;

  *_retval = InitAndSealCTypesClass(cx, global);
  return NS_OK;
}

}
}

NS_DEFINE_NAMED_CID(JSCTYPES_CID);

static const mozilla::Module::CIDEntry kCTypesCIDs[] = {
  { &kJSCTYPES_CID, false, NULL, mozilla::ctypes::ModuleConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kCTypesContracts[] = {
  { JSCTYPES_CONTRACTID, &kJSCTYPES_CID },
  { NULL }
};

static const mozilla::Module kCTypesModule = {
  mozilla::Module::kVersion,
  kCTypesCIDs,
  kCTypesContracts
};

NSMODULE_DEFN(jsctypes) = &kCTypesModule;
