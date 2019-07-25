





































#include "JSDebugger.h"
#include "nsIXPConnect.h"
#include "nsThreadUtils.h"
#include "jsapi.h"
#include "jsgc.h"
#include "jsfriendapi.h"
#include "jsdbgapi.h"
#include "mozilla/ModuleUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsMemory.h"

#define JSDEBUGGER_CONTRACTID \
  "@mozilla.org/jsdebugger;1"

#define JSDEBUGGER_CID \
{ 0x0365cbd5, 0xd46e, 0x4e94, { 0xa3, 0x9f, 0x83, 0xb6, 0x3c, 0xd1, 0xa9, 0x63 } }

namespace mozilla {
namespace jsdebugger {

NS_GENERIC_FACTORY_CONSTRUCTOR(JSDebugger)

NS_IMPL_ISUPPORTS1(JSDebugger, IJSDebugger)

JSDebugger::JSDebugger()
{
}

JSDebugger::~JSDebugger()
{
}

NS_IMETHODIMP
JSDebugger::AddClass(JSContext *cx)
{
  nsresult rv;
  nsCOMPtr<nsIXPConnect> xpc = do_GetService(nsIXPConnect::GetCID(), &rv);

  JSObject* global = JS_GetGlobalForScopeChain(cx);
  if (!global) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  if (!JS_DefineDebuggerObject(cx, global)) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

}
}

NS_DEFINE_NAMED_CID(JSDEBUGGER_CID);

static const mozilla::Module::CIDEntry kJSDebuggerCIDs[] = {
  { &kJSDEBUGGER_CID, false, NULL, mozilla::jsdebugger::JSDebuggerConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kJSDebuggerContracts[] = {
  { JSDEBUGGER_CONTRACTID, &kJSDEBUGGER_CID },
  { NULL }
};

static const mozilla::Module kJSDebuggerModule = {
  mozilla::Module::kVersion,
  kJSDebuggerCIDs,
  kJSDebuggerContracts
};

NSMODULE_DEFN(jsdebugger) = &kJSDebuggerModule;
