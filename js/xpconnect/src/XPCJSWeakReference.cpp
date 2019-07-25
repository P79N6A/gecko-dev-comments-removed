




































#include "xpcprivate.h"
#include "XPCJSWeakReference.h"

xpcJSWeakReference::xpcJSWeakReference()
{
}

NS_IMPL_ISUPPORTS1(xpcJSWeakReference, xpcIJSWeakReference)

nsresult xpcJSWeakReference::Init(JSContext* cx, const JS::Value& object)
{
    JSAutoRequest ar(cx);

    if (!object.isObject())
        return NS_ERROR_FAILURE;

    JSObject& obj = object.toObject();

    XPCCallContext ccx(NATIVE_CALLER, cx);

    nsRefPtr<nsXPCWrappedJS> wrapped;
    nsresult rv = nsXPCWrappedJS::GetNewOrUsed(ccx,
                                               &obj,
                                               NS_GET_IID(nsISupports),
                                               nsnull,
                                               getter_AddRefs(wrapped));
    if (!wrapped) {
        NS_ERROR("can't get nsISupportsWeakReference wrapper for obj");
        return rv;
    }

    return wrapped->GetWeakReference(getter_AddRefs(mWrappedJSObject));
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
