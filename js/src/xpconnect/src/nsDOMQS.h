



































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






inline JSBool
castToElement(nsIContent *content, jsval val, nsGenericElement **ppInterface,
              jsval *pVal)
{
    if(!content->IsElement())
        return JS_FALSE;
    *ppInterface = static_cast<nsGenericElement*>(content->AsElement());
    *pVal = val;
    return JS_TRUE;
}

NS_SPECIALIZE_TEMPLATE
inline JSBool
xpc_qsUnwrapThis<nsGenericElement>(JSContext *cx,
                                   JSObject *obj,
                                   JSObject *callee,
                                   nsGenericElement **ppThis,
                                   nsISupports **pThisRef,
                                   jsval *pThisVal,
                                   XPCLazyCallContext *lccx)
{
    nsIContent *content;
    jsval val;
    JSBool ok = xpc_qsUnwrapThis<nsIContent>(cx, obj, callee, &content,
                                             pThisRef, &val, lccx);
    if(ok)
    {
        ok = castToElement(content, val, ppThis, pThisVal);
        if(!ok)
            xpc_qsThrow(cx, NS_ERROR_XPC_BAD_OP_ON_WN_PROTO);
    }

    return ok;
}

NS_SPECIALIZE_TEMPLATE
inline nsresult
xpc_qsUnwrapArg<nsGenericElement>(JSContext *cx,
                                  jsval v,
                                  nsGenericElement **ppArg,
                                  nsISupports **ppArgRef,
                                  jsval *vp)
{
    nsIContent *content;
    jsval val;
    nsresult rv = xpc_qsUnwrapArg<nsIContent>(cx, v, &content, ppArgRef, &val);
    if(NS_SUCCEEDED(rv) && !castToElement(content, val, ppArg, vp))
        rv = NS_ERROR_XPC_BAD_CONVERT_JS;
    return rv;
}

#endif 
