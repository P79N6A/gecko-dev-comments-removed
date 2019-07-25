




































#include "xpcprivate.h"
#include "XPCJSWeakReference.h"

#include "nsContentUtils.h"

xpcJSWeakReference::xpcJSWeakReference()
{
}

NS_IMPL_ISUPPORTS1(xpcJSWeakReference, xpcIJSWeakReference)

nsresult xpcJSWeakReference::Init(JSContext* cx, const JS::Value& object)
{
    JSAutoRequest ar(cx);

    if (!object.isObject())
        return NS_OK;

    JSObject& obj = object.toObject();

    XPCCallContext ccx(NATIVE_CALLER, cx);

    
    nsISupports* supports =
        nsXPConnect::GetXPConnect()->GetNativeOfWrapper(cx, &obj);
    if (supports) {
        mReferent = do_GetWeakReference(supports);
        if (mReferent) {
            return NS_OK;
        }
    }
    
    

    
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

    return wrapped->GetWeakReference(getter_AddRefs(mReferent));
}

NS_IMETHODIMP
xpcJSWeakReference::Get(JSContext* aCx, JS::Value* aRetval)
{
    *aRetval = JSVAL_NULL;

    if (!mReferent) {
        return NS_OK;
    }

    nsCOMPtr<nsISupports> supports = do_QueryReferent(mReferent);
    if (!supports) {
        return NS_OK;
    }

    nsCOMPtr<nsIXPConnectWrappedJS> wrappedObj = do_QueryInterface(supports);
    if (!wrappedObj) {
        
        
        return nsContentUtils::WrapNative(aCx, JS_GetGlobalForScopeChain(aCx),
                                          supports, &NS_GET_IID(nsISupports),
                                          aRetval);
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
