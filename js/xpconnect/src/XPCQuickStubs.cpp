





#include "jsapi.h"
#include "jsfriendapi.h"
#include "jsprf.h"
#include "nsCOMPtr.h"
#include "AccessCheck.h"
#include "WrapperFactory.h"
#include "xpcprivate.h"
#include "XPCInlines.h"
#include "XPCQuickStubs.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/Exceptions.h"

using namespace mozilla;
using namespace JS;

static nsresult
getNative(nsISupports *idobj,
          HandleObject obj,
          const nsIID &iid,
          void **ppThis,
          nsISupports **pThisRef,
          jsval *vp)
{
    nsresult rv = idobj->QueryInterface(iid, ppThis);
    *pThisRef = static_cast<nsISupports*>(*ppThis);
    if (NS_SUCCEEDED(rv))
        *vp = OBJECT_TO_JSVAL(obj);
    return rv;
}

static inline nsresult
getNativeFromWrapper(JSContext *cx,
                     XPCWrappedNative *wrapper,
                     const nsIID &iid,
                     void **ppThis,
                     nsISupports **pThisRef,
                     jsval *vp)
{
    RootedObject obj(cx, wrapper->GetFlatJSObject());
    return getNative(wrapper->GetIdentityObject(), obj, iid, ppThis, pThisRef,
                     vp);
}


static nsresult
getWrapper(JSContext *cx,
           JSObject *obj,
           XPCWrappedNative **wrapper,
           JSObject **cur,
           XPCWrappedNativeTearOff **tearoff)
{
    
    
    
    
    
    
    
    if (js::IsWrapper(obj)) {
        JSObject* inner = js::CheckedUnwrap(obj,  false);

        
        
        
        if (!inner)
            return NS_ERROR_XPC_SECURITY_MANAGER_VETO;
        MOZ_ASSERT(!js::IsWrapper(inner));

        obj = inner;
    }

    
    *wrapper = nullptr;
    *cur = nullptr;
    *tearoff = nullptr;

    if (dom::IsDOMObject(obj)) {
        *cur = obj;

        return NS_OK;
    }

    
    
    
    
    
    
    const js::Class* clasp = js::GetObjectClass(obj);
    if (clasp == &XPC_WN_Tearoff_JSClass) {
        *tearoff = (XPCWrappedNativeTearOff*) js::GetObjectPrivate(obj);
        obj = js::GetObjectParent(obj);
    }

    
    
    if (IS_WN_CLASS(clasp))
        *wrapper = XPCWrappedNative::Get(obj);

    return NS_OK;
}

static nsresult
castNative(JSContext *cx,
           XPCWrappedNative *wrapper,
           JSObject *curArg,
           XPCWrappedNativeTearOff *tearoff,
           const nsIID &iid,
           void **ppThis,
           nsISupports **pThisRef,
           MutableHandleValue vp)
{
    RootedObject cur(cx, curArg);
    if (wrapper) {
        nsresult rv = getNativeFromWrapper(cx,wrapper, iid, ppThis, pThisRef,
                                           vp.address());

        if (rv != NS_ERROR_NO_INTERFACE)
            return rv;
    } else if (cur) {
        nsISupports *native;
        if (!(native = mozilla::dom::UnwrapDOMObjectToISupports(cur))) {
            *pThisRef = nullptr;
            return NS_ERROR_ILLEGAL_VALUE;
        }

        if (NS_SUCCEEDED(getNative(native, cur, iid, ppThis, pThisRef, vp.address()))) {
            return NS_OK;
        }
    }

    *pThisRef = nullptr;
    return NS_ERROR_XPC_BAD_OP_ON_WN_PROTO;
}

nsresult
xpc_qsUnwrapArgImpl(JSContext *cx,
                    HandleValue v,
                    const nsIID &iid,
                    void **ppArg,
                    nsISupports **ppArgRef,
                    MutableHandleValue vp)
{
    MOZ_ASSERT(v.isObject());
    RootedObject src(cx, &v.toObject());

    XPCWrappedNative *wrapper;
    XPCWrappedNativeTearOff *tearoff;
    JSObject *obj2;
    nsresult rv = getWrapper(cx, src, &wrapper, &obj2, &tearoff);
    NS_ENSURE_SUCCESS(rv, rv);

    if (wrapper || obj2) {
        if (NS_FAILED(castNative(cx, wrapper, obj2, tearoff, iid, ppArg,
                                 ppArgRef, vp)))
            return NS_ERROR_XPC_BAD_CONVERT_JS;
        return NS_OK;
    }
    
    

    
    nsISupports *iface;
    if (XPCConvert::GetISupportsFromJSObject(src, &iface)) {
        if (!iface || NS_FAILED(iface->QueryInterface(iid, ppArg))) {
            *ppArgRef = nullptr;
            return NS_ERROR_XPC_BAD_CONVERT_JS;
        }

        *ppArgRef = static_cast<nsISupports*>(*ppArg);
        return NS_OK;
    }

    
    XPCCallContext ccx(JS_CALLER, cx);
    if (!ccx.IsValid()) {
        *ppArgRef = nullptr;
        return NS_ERROR_XPC_BAD_CONVERT_JS;
    }

    nsRefPtr<nsXPCWrappedJS> wrappedJS;
    rv = nsXPCWrappedJS::GetNewOrUsed(src, iid, getter_AddRefs(wrappedJS));
    if (NS_FAILED(rv) || !wrappedJS) {
        *ppArgRef = nullptr;
        return rv;
    }

    
    
    
    
    rv = wrappedJS->QueryInterface(iid, ppArg);
    if (NS_SUCCEEDED(rv)) {
        *ppArgRef = static_cast<nsISupports*>(*ppArg);
        vp.setObjectOrNull(wrappedJS->GetJSObject());
    }
    return rv;
}

namespace xpc {

bool
NonVoidStringToJsval(JSContext *cx, nsAString &str, MutableHandleValue rval)
{
    nsStringBuffer* sharedBuffer;
    if (!XPCStringConvert::ReadableToJSVal(cx, str, &sharedBuffer, rval))
      return false;

    if (sharedBuffer) {
        
        
        str.ForgetSharedBuffer();
    }
    return true;
}

} 

