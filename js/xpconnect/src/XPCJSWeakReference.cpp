





#include "xpcprivate.h"
#include "XPCJSWeakReference.h"

#include "nsContentUtils.h"

using namespace JS;

xpcJSWeakReference::xpcJSWeakReference()
{
}

NS_IMPL_ISUPPORTS1(xpcJSWeakReference, xpcIJSWeakReference)

nsresult xpcJSWeakReference::Init(JSContext* cx, const JS::Value& object)
{
    if (!object.isObject())
        return NS_OK;

    JS::RootedObject obj(cx, &object.toObject());

    XPCCallContext ccx(NATIVE_CALLER, cx);

    
    nsISupports* supports =
        nsXPConnect::XPConnect()->GetNativeOfWrapper(cx, obj);
    nsCOMPtr<nsISupportsWeakReference> supportsWeakRef =
        do_QueryInterface(supports);
    if (supportsWeakRef) {
        supportsWeakRef->GetWeakReference(getter_AddRefs(mReferent));
        if (mReferent) {
            return NS_OK;
        }
    }
    
    

    
    nsRefPtr<nsXPCWrappedJS> wrapped;
    nsresult rv = nsXPCWrappedJS::GetNewOrUsed(obj,
                                               NS_GET_IID(nsISupports),
                                               getter_AddRefs(wrapped));
    if (!wrapped) {
        NS_ERROR("can't get nsISupportsWeakReference wrapper for obj");
        return rv;
    }

    return wrapped->GetWeakReference(getter_AddRefs(mReferent));
}

NS_IMETHODIMP
xpcJSWeakReference::Get(JSContext* aCx, MutableHandleValue aRetval)
{
    aRetval.setNull();

    if (!mReferent) {
        return NS_OK;
    }

    nsCOMPtr<nsISupports> supports = do_QueryReferent(mReferent);
    if (!supports) {
        return NS_OK;
    }

    nsCOMPtr<nsIXPConnectWrappedJS> wrappedObj = do_QueryInterface(supports);
    if (!wrappedObj) {
        
        
        return nsContentUtils::WrapNative(aCx, supports,
                                          &NS_GET_IID(nsISupports),
                                          aRetval);
    }

    JS::RootedObject obj(aCx, wrappedObj->GetJSObject());
    if (!obj) {
        return NS_OK;
    }

    
    
    
    
    
    if (!JS_WrapObject(aCx, &obj)) {
        return NS_ERROR_FAILURE;
    }

    aRetval.setObject(*obj);
    return NS_OK;
}
