



































#ifndef nsDOMQS_h__
#define nsDOMQS_h__

#include "nsDOMClassInfoID.h"

#define DEFINE_UNWRAP_CAST(_interface, _bit)                                  \
NS_SPECIALIZE_TEMPLATE                                                        \
inline JSBool                                                                 \
xpc_qsUnwrapThis<_interface>(JSContext *cx,                                   \
                             JSObject *obj,                                   \
                             JSObject *callee,                                \
                             _interface **ppThis,                             \
                             nsISupports **pThisRef,                          \
                             jsval *pThisVal,                                 \
                             XPCLazyCallContext *lccx)                        \
{                                                                             \
    nsresult rv;                                                              \
    nsISupports *native = castNativeFromWrapper(cx, obj, callee, _bit,        \
                                                pThisRef, pThisVal, lccx,     \
                                                &rv);                         \
    if(!native)                                                               \
        return JS_FALSE;                                                      \
    *ppThis = static_cast<_interface*>(native);                               \
    return JS_TRUE;                                                           \
}                                                                             \
                                                                              \
NS_SPECIALIZE_TEMPLATE                                                        \
inline nsresult                                                               \
xpc_qsUnwrapArg<_interface>(JSContext *cx,                                    \
                            jsval v,                                          \
                            _interface **ppArg,                               \
                            nsISupports **ppArgRef,                           \
                            jsval *vp)                                        \
{                                                                             \
    nsresult rv;                                                              \
    nsISupports *native = castNativeArgFromWrapper(cx, v, _bit, ppArgRef, vp, \
                                                   &rv);                      \
    if(NS_SUCCEEDED(rv))                                                      \
        *ppArg = static_cast<_interface*>(native);                            \
    return rv;                                                                \
}

#define DOMCI_CASTABLE_INTERFACE(_interface, _bit, _extra)                    \
  DEFINE_UNWRAP_CAST(_interface, _bit)

DOMCI_CASTABLE_INTERFACES(unused)

#undef DOMCI_CASTABLE_INTERFACE

#endif 
