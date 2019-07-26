




#include "nsXBLSerialize.h"
#include "nsIXPConnect.h"
#include "nsContentUtils.h"
#include "jsdbgapi.h"

using namespace mozilla;

nsresult
XBL_SerializeFunction(nsIScriptContext* aContext,
                      nsIObjectOutputStream* aStream,
                      JSObject* aFunctionObject)
{
  AutoPushJSContext cx(aContext->GetNativeContext());
  JS::RootedObject function(cx, aFunctionObject);
  return nsContentUtils::XPConnect()->WriteFunction(aStream, cx, function);
}

nsresult
XBL_DeserializeFunction(nsIScriptContext* aContext,
                        nsIObjectInputStream* aStream,
                        JS::MutableHandle<JSObject*> aFunctionObjectp)
{
  AutoPushJSContext cx(aContext->GetNativeContext());
  nsresult rv = nsContentUtils::XPConnect()->ReadFunction(aStream, cx,
                                                          aFunctionObjectp.address());
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  JSAutoRequest ar(cx);
  JSFunction* fun = JS_ValueToFunction(cx, JS::ObjectValue(*aFunctionObjectp));
  NS_ENSURE_TRUE(fun, NS_ERROR_UNEXPECTED);
  JS_SetScriptUserBit(JS_GetFunctionScript(cx, fun), true);
  return NS_OK;
}
