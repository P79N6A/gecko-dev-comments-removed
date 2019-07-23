





































#include "nsIDOMCanvasRenderingContext2D.h"

typedef nsresult (NS_STDCALL nsIDOMCanvasRenderingContext2D::*CanvasStyleSetterType)(const nsAString &, nsISupports *);
typedef nsresult (NS_STDCALL nsIDOMCanvasRenderingContext2D::*CanvasStyleGetterType)(nsAString &, nsISupports **, PRInt32 *);

static JSBool
Canvas2D_SetStyleHelper(JSContext *cx, JSObject *obj, jsval id, jsval *vp,
                        CanvasStyleSetterType setfunc)
{
    XPC_QS_ASSERT_CONTEXT_OK(cx);
    nsIDOMCanvasRenderingContext2D *self;
    xpc_qsSelfRef selfref;
    JSAutoTempValueRooter tvr(cx);
    if (!xpc_qsUnwrapThis(cx, obj, nsnull, &self, &selfref.ptr, tvr.addr(), nsnull))
        return JS_FALSE;

    nsresult rv;

    if (JSVAL_IS_STRING(*vp)) {
        xpc_qsDOMString arg0(cx, *vp, vp,
                             xpc_qsDOMString::eDefaultNullBehavior,
                             xpc_qsDOMString::eDefaultUndefinedBehavior);
        if (!arg0.IsValid())
            return JS_FALSE;

        rv = (self->*setfunc)(arg0, nsnull);
    } else {
        nsISupports *arg0;
        xpc_qsSelfRef arg0ref;
        rv = xpc_qsUnwrapArg<nsISupports>(cx, *vp, &arg0, &arg0ref.ptr, vp);
        if (NS_FAILED(rv)) {
            xpc_qsThrowBadSetterValue(cx, rv, JSVAL_TO_OBJECT(*tvr.addr()), id);
            return JS_FALSE;
        }

        nsString voidStr;
        voidStr.SetIsVoid(PR_TRUE);

        rv = (self->*setfunc)(voidStr, arg0);
    }

    if (NS_FAILED(rv))
        return xpc_qsThrowGetterSetterFailed(cx, rv, JSVAL_TO_OBJECT(*tvr.addr()), id);

    return JS_TRUE;
}

static JSBool
Canvas2D_GetStyleHelper(JSContext *cx, JSObject *obj, jsval id, jsval *vp,
                        CanvasStyleGetterType getfunc)
{
    XPC_QS_ASSERT_CONTEXT_OK(cx);
    nsIDOMCanvasRenderingContext2D *self;
    xpc_qsSelfRef selfref;
    XPCLazyCallContext lccx(JS_CALLER, cx, obj);
    if (!xpc_qsUnwrapThis(cx, obj, nsnull, &self, &selfref.ptr, vp, &lccx))
        return JS_FALSE;
    nsresult rv;

    nsString resultString;
    nsCOMPtr<nsISupports> resultInterface;
    PRInt32 resultType;
    rv = (self->*getfunc)(resultString, getter_AddRefs(resultInterface), &resultType);
    if (NS_FAILED(rv))
        return xpc_qsThrowGetterSetterFailed(cx, rv, JSVAL_TO_OBJECT(*vp), id);

    switch (resultType) {
    case nsIDOMCanvasRenderingContext2D::CMG_STYLE_STRING:
        return xpc_qsStringToJsval(cx, resultString, vp);

    case nsIDOMCanvasRenderingContext2D::CMG_STYLE_PATTERN:
        return xpc_qsXPCOMObjectToJsval(lccx, resultInterface, xpc_qsGetWrapperCache(resultInterface),
                                        &NS_GET_IID(nsIDOMCanvasPattern), &interfaces[k_nsIDOMCanvasPattern], vp);

    case nsIDOMCanvasRenderingContext2D::CMG_STYLE_GRADIENT:
        return xpc_qsXPCOMObjectToJsval(lccx, resultInterface, xpc_qsGetWrapperCache(resultInterface),
                                        &NS_GET_IID(nsIDOMCanvasGradient), &interfaces[k_nsIDOMCanvasGradient], vp);

    default:
        return xpc_qsThrowGetterSetterFailed(cx, NS_ERROR_FAILURE, JSVAL_TO_OBJECT(*vp), id);
    }
}

static JSBool
nsIDOMCanvasRenderingContext2D_SetStrokeStyle(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    return Canvas2D_SetStyleHelper(cx, obj, id, vp, &nsIDOMCanvasRenderingContext2D::SetStrokeStyle_multi);
}

static JSBool
nsIDOMCanvasRenderingContext2D_GetStrokeStyle(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    return Canvas2D_GetStyleHelper(cx, obj, id, vp, &nsIDOMCanvasRenderingContext2D::GetStrokeStyle_multi);
}

static JSBool
nsIDOMCanvasRenderingContext2D_SetFillStyle(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    return Canvas2D_SetStyleHelper(cx, obj, id, vp, &nsIDOMCanvasRenderingContext2D::SetFillStyle_multi);
}

static JSBool
nsIDOMCanvasRenderingContext2D_GetFillStyle(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    return Canvas2D_GetStyleHelper(cx, obj, id, vp, &nsIDOMCanvasRenderingContext2D::GetFillStyle_multi);
}
