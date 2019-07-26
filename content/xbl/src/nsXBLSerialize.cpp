




#include "nsXBLSerialize.h"
#include "nsIXPConnect.h"
#include "nsContentUtils.h"
#include "jsdbgapi.h"

using namespace mozilla;

nsresult
XBL_SerializeFunction(nsIScriptContext* aContext,
                      nsIObjectOutputStream* aStream,
                      JS::Handle<JSObject*> aFunction)
{
  AutoPushJSContext cx(aContext->GetNativeContext());
  return nsContentUtils::XPConnect()->WriteFunction(aStream, cx, aFunction);
}

nsresult
XBL_DeserializeFunction(nsIScriptContext* aContext,
                        nsIObjectInputStream* aStream,
                        JS::MutableHandle<JSObject*> aFunctionObjectp)
{
  AutoPushJSContext cx(aContext->GetNativeContext());
  return nsContentUtils::XPConnect()->ReadFunction(aStream, cx,
                                                   aFunctionObjectp.address());
}
