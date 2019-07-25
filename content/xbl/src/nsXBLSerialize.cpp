




































#include "nsXBLSerialize.h"
#include "nsDOMScriptObjectHolder.h"
#include "nsContentUtils.h"
#include "jsxdrapi.h"

nsresult
XBL_SerializeFunction(nsIScriptContext* aContext,
                      nsIObjectOutputStream* aStream,
                      JSObject* aFunctionObject)
{
  JSContext* cx = aContext->GetNativeContext();
  JSXDRState *xdr = ::JS_XDRNewMem(cx, JSXDR_ENCODE);
  if (!xdr)
    return NS_ERROR_OUT_OF_MEMORY;
  xdr->userdata = static_cast<void*>(aStream);

  JSAutoRequest ar(cx);
  nsresult rv;
  if (!JS_XDRFunctionObject(xdr, &aFunctionObject)) {
    rv = NS_ERROR_FAILURE;
  } else {
    uint32 size;
    const char* data = reinterpret_cast<const char*>
                                       (JS_XDRMemGetData(xdr, &size));
    NS_ASSERTION(data, "no decoded JSXDRState data!");

    rv = aStream->Write32(size);
    if (NS_SUCCEEDED(rv))
      rv = aStream->WriteBytes(data, size);
  }

  JS_XDRDestroy(xdr);

  return rv;
}


nsresult
XBL_DeserializeFunction(nsIScriptContext* aContext,
                        nsIObjectInputStream* aStream,
                        JSObject** aFunctionObject)
{
  *aFunctionObject = nsnull;

  PRUint32 size;
  nsresult rv = aStream->Read32(&size);
  if (NS_FAILED(rv))
    return rv;

  char* data;
  rv = aStream->ReadBytes(size, &data);
  if (NS_FAILED(rv))
    return rv;

  JSContext* cx = aContext->GetNativeContext();
  JSXDRState *xdr = JS_XDRNewMem(cx, JSXDR_DECODE);
  if (!xdr) {
    rv = NS_ERROR_OUT_OF_MEMORY;
  } else {
    xdr->userdata = static_cast<void*>(aStream);
    JSAutoRequest ar(cx);
    JS_XDRMemSetData(xdr, data, size);

    if (!JS_XDRFunctionObject(xdr, aFunctionObject)) {
      rv = NS_ERROR_FAILURE;
    }

    uint32 junk;
    data = static_cast<char*>(JS_XDRMemGetData(xdr, &junk));
    JS_XDRMemSetData(xdr, NULL, 0);
    JS_XDRDestroy(xdr);
  }

  nsMemory::Free(data);
  NS_ENSURE_SUCCESS(rv, rv);
  return rv;
}
