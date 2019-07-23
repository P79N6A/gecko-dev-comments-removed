



































#include "xpcJSWeakReference.h"
#include "xpcprivate.h"

xpcJSWeakReference::xpcJSWeakReference()
{
}

NS_IMPL_ISUPPORTS1(xpcJSWeakReference, xpcIJSWeakReference)

nsresult xpcJSWeakReference::Init()
{
    nsresult rv;
    
    nsXPConnect* xpc = nsXPConnect::GetXPConnect();
    if (!xpc) return NS_ERROR_UNEXPECTED;
    
    nsCOMPtr<nsIXPCNativeCallContext> cc;
    rv = xpc->GetCurrentNativeCallContext(getter_AddRefs(cc));
    NS_ENSURE_SUCCESS(rv, rv);

    JSContext *cx = nsnull;
    rv = cc->GetJSContext(&cx);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 argc = 0;
    rv = cc->GetArgc(&argc);
    NS_ENSURE_SUCCESS(rv, rv);

    if (argc != 1) return NS_ERROR_FAILURE;
    
    jsval *argv = nsnull;
    rv = cc->GetArgvPtr(&argv);
    NS_ENSURE_SUCCESS(rv, rv);

    if (JSVAL_IS_NULL(argv[0])) return NS_ERROR_FAILURE;
    
    JSObject *obj;
    if (!JS_ValueToObject(cx, argv[0], &obj)) {
        cc->SetExceptionWasThrown(JS_TRUE);
        return NS_ERROR_FAILURE;
    }
    
    XPCCallContext ccx(NATIVE_CALLER, cx);
    
    nsRefPtr<nsXPCWrappedJS> wrapped;
    rv = nsXPCWrappedJS::GetNewOrUsed(ccx,
                                      obj,
                                      NS_GET_IID(nsISupports),
                                      nsnull,
                                      getter_AddRefs(wrapped));
    if (!wrapped) {
        NS_ERROR("can't get nsISupportsWeakReference wrapper for obj");
        return rv;
    }

    return static_cast<nsISupportsWeakReference*>(wrapped)->
        GetWeakReference(getter_AddRefs(mWrappedJSObject));
}

NS_IMETHODIMP
xpcJSWeakReference::Get()
{
    nsresult rv;
    
    nsXPConnect* xpc = nsXPConnect::GetXPConnect();
    if (!xpc) return NS_ERROR_UNEXPECTED;
    
    nsCOMPtr<nsIXPCNativeCallContext> cc;
    rv = xpc->GetCurrentNativeCallContext(getter_AddRefs(cc));
    NS_ENSURE_SUCCESS(rv, rv);

    jsval *retval = nsnull;
    cc->GetRetValPtr(&retval);
    if (!retval) return NS_ERROR_UNEXPECTED;
    *retval = JSVAL_NULL;
    
    nsCOMPtr<nsIXPConnectWrappedJS> wrappedObj;
    
    if (mWrappedJSObject &&
        NS_SUCCEEDED(mWrappedJSObject->QueryReferent(NS_GET_IID(nsIXPConnectWrappedJS), getter_AddRefs(wrappedObj))) &&
        wrappedObj) {
        JSObject *obj;
        wrappedObj->GetJSObject(&obj);
        if (obj)
            *retval = OBJECT_TO_JSVAL(obj);
    }
    
    return NS_OK;
}
