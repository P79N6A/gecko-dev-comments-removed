




































#include "nsXBLSerialize.h"
#include "nsDOMScriptObjectHolder.h"
#include "nsContentUtils.h"

nsresult
XBL_SerializeFunction(nsIScriptContext* aContext,
                      nsIObjectOutputStream* aStream,
                      JSObject* aFunctionObject)
{
  JSContext* cx = aContext->GetNativeContext();
  return nsContentUtils::XPConnect()->WriteFunction(aStream, cx, aFunctionObject);
}

nsresult
XBL_DeserializeFunction(nsIScriptContext* aContext,
                        nsIObjectInputStream* aStream,
                        JSObject** aFunctionObjectp)
{
  JSContext* cx = aContext->GetNativeContext();
  return nsContentUtils::XPConnect()->ReadFunction(aStream, cx, aFunctionObjectp);
}
