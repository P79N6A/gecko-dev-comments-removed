






































#include "Module.h"
#include "Library.h"
#include "CTypes.h"
#include "nsIGenericFactory.h"
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

NS_IMETHODIMP
Module::Call(nsIXPConnectWrappedNative* wrapper,
             JSContext* cx,
             JSObject* obj,
             PRUint32 argc,
             jsval* argv,
             jsval* vp,
             PRBool* _retval)
{
  JSObject* global = JS_GetGlobalObject(cx);
  *_retval = Init(cx, global);
  return NS_OK;
}

#define CTYPESFN_FLAGS \
  (JSFUN_FAST_NATIVE | JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT)

static JSFunctionSpec sModuleFunctions[] = {
  JS_FN("open", Library::Open, 1, CTYPESFN_FLAGS),
  JS_FN("cast", CData::Cast, 2, CTYPESFN_FLAGS),
  JS_FS_END
};

bool
Module::Init(JSContext* cx, JSObject* aGlobal)
{
  
  JSObject* ctypes = JS_NewObject(cx, NULL, NULL, NULL);
  if (!ctypes)
    return false;

  if (!JS_DefineProperty(cx, aGlobal, "ctypes", OBJECT_TO_JSVAL(ctypes),
         NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
    return false;

  if (!InitTypeClasses(cx, ctypes))
    return false;

  
  if (!JS_DefineFunctions(cx, ctypes, sModuleFunctions))
    return false;

  
  
  return JS_SealObject(cx, ctypes, JS_FALSE) != JS_FALSE;
}

}
}

static nsModuleComponentInfo components[] =
{
  {
    "jsctypes",
    JSCTYPES_CID,
    JSCTYPES_CONTRACTID,
    mozilla::ctypes::ModuleConstructor,
  }
};

NS_IMPL_NSGETMODULE(jsctypes, components)

