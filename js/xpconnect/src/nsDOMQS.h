



































#ifndef nsDOMQS_h__
#define nsDOMQS_h__

#include "nsDOMClassInfoID.h"

#define DEFINE_UNWRAP_CAST(_interface, _base, _bit)                           \
NS_SPECIALIZE_TEMPLATE                                                        \
inline JSBool                                                                 \
xpc_qsUnwrapThis<_interface>(JSContext *cx,                                   \
                             JSObject *obj,                                   \
                             JSObject *callee,                                \
                             _interface **ppThis,                             \
                             nsISupports **pThisRef,                          \
                             jsval *pThisVal,                                 \
                             XPCLazyCallContext *lccx,                        \
                             bool failureFatal)                               \
{                                                                             \
    nsresult rv;                                                              \
    nsISupports *native = castNativeFromWrapper(cx, obj, callee, _bit,        \
                                                pThisRef, pThisVal, lccx,     \
                                                &rv);                         \
    if (failureFatal && !native)                                              \
        return xpc_qsThrow(cx, rv);                                           \
    *ppThis = static_cast<_interface*>(static_cast<_base*>(native));          \
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
    if (NS_SUCCEEDED(rv))                                                     \
        *ppArg = static_cast<_interface*>(static_cast<_base*>(native));       \
    return rv;                                                                \
}

#undef DOMCI_CASTABLE_INTERFACE

#undef DOMCI_CASTABLE_INTERFACE
#define DOMCI_CASTABLE_INTERFACE(_interface, _base, _bit, _extra)             \
  DEFINE_UNWRAP_CAST(_interface, _base, _bit)

DOMCI_CASTABLE_INTERFACES(unused)

#undef DOMCI_CASTABLE_INTERFACE






inline JSBool
castToElement(nsIContent *content, jsval val, nsGenericElement **ppInterface,
              jsval *pVal)
{
    if (!content->IsElement())
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
                                   XPCLazyCallContext *lccx,
                                   bool failureFatal)
{
    nsIContent *content;
    jsval val;
    JSBool ok = xpc_qsUnwrapThis<nsIContent>(cx, obj, callee, &content,
                                             pThisRef, &val, lccx,
                                             failureFatal);
    if (ok) {
        if (failureFatal || content)
          ok = castToElement(content, val, ppThis, pThisVal);
        if (failureFatal && !ok)
            xpc_qsThrow(cx, NS_ERROR_XPC_BAD_OP_ON_WN_PROTO);
    }

    if (!failureFatal && (!ok || !content)) {
      ok = JS_TRUE;
      *ppThis = nsnull;
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
    if (NS_SUCCEEDED(rv) && !castToElement(content, val, ppArg, vp))
        rv = NS_ERROR_XPC_BAD_CONVERT_JS;
    return rv;
}

inline nsISupports*
ToSupports(nsContentList *p)
{
    return static_cast<nsINodeList*>(p);
}

inline nsISupports*
ToCanonicalSupports(nsINode* p)
{
    return p;
}

inline nsISupports*
ToCanonicalSupports(nsContentList *p)
{
    return static_cast<nsINodeList*>(p);
}

#endif 
