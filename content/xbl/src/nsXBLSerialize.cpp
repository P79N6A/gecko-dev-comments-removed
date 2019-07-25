




































#include "nsXBLSerialize.h"
#include "nsDOMScriptObjectHolder.h"
#include "nsContentUtils.h"
#include "jsxdrapi.h"

nsresult
XBL_SerializeFunction(nsIScriptContext* aContext,
                      nsIObjectOutputStream* aStream,
                      JSObject* aFunctionObject,
                      PRUint32 aLineNumber)
{
  nsresult rv = aStream->Write32(aLineNumber);
  NS_ENSURE_SUCCESS(rv, rv);

  JSContext* cx = (JSContext*) aContext->GetNativeContext();
  JSXDRState *xdr = ::JS_XDRNewMem(cx, JSXDR_ENCODE);
  if (!xdr)
    return NS_ERROR_OUT_OF_MEMORY;
  xdr->userdata = (void*) aStream;

  jsval funval = OBJECT_TO_JSVAL(aFunctionObject);

  JSAutoRequest ar(cx);
  if (! ::JS_XDRFunctionObject(xdr, &aFunctionObject)) {
    rv = NS_ERROR_FAILURE;
  } else {
    uint32 size;
    const char* data = reinterpret_cast<const char*>
                                         (::JS_XDRMemGetData(xdr, &size));
    NS_ASSERTION(data, "no decoded JSXDRState data!");

    rv = aStream->Write32(size);
    if (NS_SUCCEEDED(rv))
      rv = aStream->WriteBytes(data, size);
  }

  ::JS_XDRDestroy(xdr);

  return rv;
}


nsresult
XBL_DeserializeFunction(nsIScriptContext* aContext,
                        nsIObjectInputStream* aStream,
                        void* aHolder,
                        PRUint32* aLineNumber,
                        void **aScriptObject)
{
  *aScriptObject = nsnull;

  nsresult rv = aStream->Read32(aLineNumber);
  if (NS_FAILED(rv))
    return rv;

  JSObject* functionObject = nsnull;

  PRUint32 size;
  rv = aStream->Read32(&size);
  if (NS_FAILED(rv))
    return rv;

  char* data;
  rv = aStream->ReadBytes(size, &data);
  if (NS_FAILED(rv))
    return rv;

  JSContext* cx = (JSContext*) aContext->GetNativeContext();
  JSXDRState *xdr = ::JS_XDRNewMem(cx, JSXDR_DECODE);
  if (!xdr) {
    rv = NS_ERROR_OUT_OF_MEMORY;
  } else {
    xdr->userdata = (void*) aStream;
    JSAutoRequest ar(cx);
    ::JS_XDRMemSetData(xdr, data, size);

    if (! ::JS_XDRFunctionObject(xdr, &functionObject)) {
      rv = NS_ERROR_FAILURE;
    }

    uint32 junk;
    data = (char*) ::JS_XDRMemGetData(xdr, &junk);
    ::JS_XDRMemSetData(xdr, NULL, 0);
    ::JS_XDRDestroy(xdr);
  }

  nsMemory::Free(data);
  NS_ENSURE_SUCCESS(rv, rv);

  *aScriptObject = functionObject;

  return rv;
}
