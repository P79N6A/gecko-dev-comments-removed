





#ifndef xpcquickstubs_h___
#define xpcquickstubs_h___

#include "XPCForwards.h"



nsresult
getWrapper(JSContext *cx,
           JSObject *obj,
           XPCWrappedNative **wrapper,
           JSObject **cur,
           XPCWrappedNativeTearOff **tearoff);

nsresult
castNative(JSContext *cx,
           XPCWrappedNative *wrapper,
           JSObject *cur,
           XPCWrappedNativeTearOff *tearoff,
           const nsIID &iid,
           void **ppThis,
           nsISupports **ppThisRef,
           JS::MutableHandleValue vp);

nsISupports*
castNativeFromWrapper(JSContext *cx,
                      JSObject *obj,
                      uint32_t interfaceBit,
                      uint32_t protoID,
                      int32_t protoDepth,
                      nsISupports **pRef,
                      JS::MutableHandleValue pVal,
                      nsresult *rv);

MOZ_ALWAYS_INLINE JSObject*
xpc_qsUnwrapObj(jsval v, nsISupports **ppArgRef, nsresult *rv)
{
    *rv = NS_OK;
    if (v.isObject()) {
        return &v.toObject();
    }

    if (!v.isNullOrUndefined()) {
        *rv = ((v.isInt32() && v.toInt32() == 0)
               ? NS_ERROR_XPC_BAD_CONVERT_JS_ZERO_ISNOT_NULL
               : NS_ERROR_XPC_BAD_CONVERT_JS);
    }

    *ppArgRef = nullptr;
    return nullptr;
}

nsresult
xpc_qsUnwrapArgImpl(JSContext *cx, JS::HandleValue v, const nsIID &iid, void **ppArg,
                    nsISupports **ppArgRef, JS::MutableHandleValue vp);


template <class Interface, class StrongRefType>
inline nsresult
xpc_qsUnwrapArg(JSContext *cx, JS::HandleValue v, Interface **ppArg,
                StrongRefType **ppArgRef, JS::MutableHandleValue vp)
{
    nsISupports* argRef = *ppArgRef;
    nsresult rv = xpc_qsUnwrapArgImpl(cx, v, NS_GET_TEMPLATE_IID(Interface),
                                      reinterpret_cast<void **>(ppArg), &argRef,
                                      vp);
    *ppArgRef = static_cast<StrongRefType*>(argRef);
    return rv;
}

MOZ_ALWAYS_INLINE nsISupports*
castNativeArgFromWrapper(JSContext *cx,
                         jsval v,
                         uint32_t bit,
                         uint32_t protoID,
                         int32_t protoDepth,
                         nsISupports **pArgRef,
                         JS::MutableHandleValue vp,
                         nsresult *rv)
{
    JSObject *src = xpc_qsUnwrapObj(v, pArgRef, rv);
    if (!src)
        return nullptr;

    return castNativeFromWrapper(cx, src, bit, protoID, protoDepth, pArgRef, vp, rv);
}

#endif 
