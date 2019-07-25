




































#include "xpcprivate.h"
#include "XPCJSWeakReference.h"

xpcJSWeakReference::xpcJSWeakReference()
{
}

NS_IMPL_ISUPPORTS1(xpcJSWeakReference, xpcIJSWeakReference)

nsresult xpcJSWeakReference::Init()
{
    nsresult rv;

    nsXPConnect* xpc = nsXPConnect::GetXPConnect();
    if (!xpc) return NS_ERROR_UNEXPECTED;

    nsAXPCNativeCallContext *cc = nsnull;
    rv = xpc->GetCurrentNativeCallContext(&cc);
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

    JSAutoRequest ar(cx);

    if (JSVAL_IS_PRIMITIVE(argv[0])) return NS_ERROR_FAILURE;

    JSObject *obj = JSVAL_TO_OBJECT(argv[0]);

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
xpcJSWeakReference::Get(JSContext* aCx, JS::Value* aRetval)
{
    *aRetval = JSVAL_NULL;

    if (!mWrappedJSObject) {
        return NS_OK;
    }

    nsCOMPtr<nsIXPConnectWrappedJS> wrappedObj = do_QueryReferent(mWrappedJSObject);
    if (!wrappedObj) {
        return NS_OK;
    }

    JSObject *obj;
    wrappedObj->GetJSObject(&obj);
    if (!obj) {
        return NS_OK;
    }

    
    
    
    
    
    if (!JS_WrapObject(aCx, &obj)) {
        return NS_ERROR_FAILURE;
    }

    *aRetval = OBJECT_TO_JSVAL(obj);
    return NS_OK;
}
