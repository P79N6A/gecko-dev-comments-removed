






































#include "nsDOMError.h"
#include "nsIDOMCanvasRenderingContext2D.h"

typedef nsresult (NS_STDCALL nsIDOMCanvasRenderingContext2D::*CanvasStyleSetterType)(const nsAString &, nsISupports *);
typedef nsresult (NS_STDCALL nsIDOMCanvasRenderingContext2D::*CanvasStyleGetterType)(nsAString &, nsISupports **, PRInt32 *);

static JSBool
Canvas2D_SetStyleHelper(JSContext *cx, JSObject *obj, jsid id, jsval *vp,
                        CanvasStyleSetterType setfunc)
{
    XPC_QS_ASSERT_CONTEXT_OK(cx);
    nsIDOMCanvasRenderingContext2D *self;
    xpc_qsSelfRef selfref;
    js::AutoValueRooter tvr(cx);
    if (!xpc_qsUnwrapThis(cx, obj, nsnull, &self, &selfref.ptr, tvr.jsval_addr(), nsnull))
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
            xpc_qsThrowBadSetterValue(cx, rv, JSVAL_TO_OBJECT(*tvr.jsval_addr()), id);
            return JS_FALSE;
        }

        nsString voidStr;
        voidStr.SetIsVoid(PR_TRUE);

        rv = (self->*setfunc)(voidStr, arg0);
    }

    if (NS_FAILED(rv))
        return xpc_qsThrowGetterSetterFailed(cx, rv, JSVAL_TO_OBJECT(*tvr.jsval_addr()), id);

    return JS_TRUE;
}

static JSBool
Canvas2D_GetStyleHelper(JSContext *cx, JSObject *obj, jsid id, jsval *vp,
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
    {
        qsObjectHelper helper(resultInterface,
                              xpc_qsGetWrapperCache(resultInterface));
        return xpc_qsXPCOMObjectToJsval(lccx, helper,
                                        &NS_GET_IID(nsIDOMCanvasPattern),
                                        &interfaces[k_nsIDOMCanvasPattern], vp);
    }
    case nsIDOMCanvasRenderingContext2D::CMG_STYLE_GRADIENT:
    {
        qsObjectHelper helper(resultInterface,
                              xpc_qsGetWrapperCache(resultInterface));
        return xpc_qsXPCOMObjectToJsval(lccx, helper,
                                        &NS_GET_IID(nsIDOMCanvasGradient),
                                        &interfaces[k_nsIDOMCanvasGradient], vp);
    }
    default:
        return xpc_qsThrowGetterSetterFailed(cx, NS_ERROR_FAILURE, JSVAL_TO_OBJECT(*vp), id);
    }
}

static JSBool
nsIDOMCanvasRenderingContext2D_SetStrokeStyle(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    return Canvas2D_SetStyleHelper(cx, obj, id, vp, &nsIDOMCanvasRenderingContext2D::SetStrokeStyle_multi);
}

static JSBool
nsIDOMCanvasRenderingContext2D_GetStrokeStyle(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    return Canvas2D_GetStyleHelper(cx, obj, id, vp, &nsIDOMCanvasRenderingContext2D::GetStrokeStyle_multi);
}

static JSBool
nsIDOMCanvasRenderingContext2D_SetFillStyle(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    return Canvas2D_SetStyleHelper(cx, obj, id, vp, &nsIDOMCanvasRenderingContext2D::SetFillStyle_multi);
}

static JSBool
nsIDOMCanvasRenderingContext2D_GetFillStyle(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    return Canvas2D_GetStyleHelper(cx, obj, id, vp, &nsIDOMCanvasRenderingContext2D::GetFillStyle_multi);
}

static JSBool
nsIDOMCanvasRenderingContext2D_CreateImageData(JSContext *cx, uintN argc, jsval *vp)
{
    XPC_QS_ASSERT_CONTEXT_OK(cx);

    

    if (argc < 2)
        return xpc_qsThrow(cx, NS_ERROR_XPC_NOT_ENOUGH_ARGS);

    jsval *argv = JS_ARGV(cx, vp);

    int32 wi, hi;
    if (!JS_ValueToECMAInt32(cx, argv[0], &wi) ||
        !JS_ValueToECMAInt32(cx, argv[1], &hi))
        return JS_FALSE;

    if (wi <= 0 || hi <= 0)
        return xpc_qsThrow(cx, NS_ERROR_DOM_INDEX_SIZE_ERR);

    uint32 w = (uint32) wi;
    uint32 h = (uint32) hi;

    
    uint32 len0 = w * h;
    if (len0 / w != (uint32) h)
        return xpc_qsThrow(cx, NS_ERROR_DOM_INDEX_SIZE_ERR);

    uint32 len = len0 * 4;
    if (len / 4 != len0)
        return xpc_qsThrow(cx, NS_ERROR_DOM_INDEX_SIZE_ERR);

    
    JSObject *darray = js_CreateTypedArray(cx, js::TypedArray::TYPE_UINT8_CLAMPED, len);
    js::AutoObjectRooter rd(cx, darray);
    if (!darray)
        return JS_FALSE;

    
    
    JSObject *result = JS_NewObject(cx, NULL, NULL, NULL);
    js::AutoObjectRooter rr(cx, result);
    if (!result)
        return JS_FALSE;

    if (!JS_DefineProperty(cx, result, "width", INT_TO_JSVAL(w), NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT) ||
        !JS_DefineProperty(cx, result, "height", INT_TO_JSVAL(h), NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT) ||
        !JS_DefineProperty(cx, result, "data", OBJECT_TO_JSVAL(darray), NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
        return JS_FALSE;

    *vp = OBJECT_TO_JSVAL(result);
    return JS_TRUE;
}

static JSBool
nsIDOMCanvasRenderingContext2D_GetImageData(JSContext *cx, uintN argc, jsval *vp)
{
    XPC_QS_ASSERT_CONTEXT_OK(cx);

    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    if (!obj)
        return JS_FALSE;

    nsresult rv;

    nsIDOMCanvasRenderingContext2D *self;
    xpc_qsSelfRef selfref;
    js::AutoValueRooter tvr(cx);
    if (!xpc_qsUnwrapThis(cx, obj, nsnull, &self, &selfref.ptr, tvr.jsval_addr(), nsnull))
        return JS_FALSE;

    if (argc < 4)
        return xpc_qsThrow(cx, NS_ERROR_XPC_NOT_ENOUGH_ARGS);

    jsval *argv = JS_ARGV(cx, vp);

    int32 x, y;
    int32 wi, hi;
    if (!JS_ValueToECMAInt32(cx, argv[0], &x) ||
        !JS_ValueToECMAInt32(cx, argv[1], &y) ||
        !JS_ValueToECMAInt32(cx, argv[2], &wi) ||
        !JS_ValueToECMAInt32(cx, argv[3], &hi))
        return JS_FALSE;

    if (wi <= 0 || hi <= 0)
        return xpc_qsThrow(cx, NS_ERROR_DOM_INDEX_SIZE_ERR);

    uint32 w = (uint32) wi;
    uint32 h = (uint32) hi;

    
    uint32 len0 = w * h;
    if (len0 / w != (uint32) h)
        return xpc_qsThrow(cx, NS_ERROR_DOM_INDEX_SIZE_ERR);

    uint32 len = len0 * 4;
    if (len / 4 != len0)
        return xpc_qsThrow(cx, NS_ERROR_DOM_INDEX_SIZE_ERR);

    
    JSObject *darray = js_CreateTypedArray(cx, js::TypedArray::TYPE_UINT8_CLAMPED, len);
    js::AutoObjectRooter rd(cx, darray);
    if (!darray)
        return JS_FALSE;

    js::TypedArray *tdest = js::TypedArray::fromJSObject(darray);

    
    rv = self->GetImageData_explicit(x, y, w, h, (PRUint8*) tdest->data, tdest->byteLength);
    if (NS_FAILED(rv))
        return xpc_qsThrowMethodFailed(cx, rv, vp);

    
    
    JSObject *result = JS_NewObject(cx, NULL, NULL, NULL);
    js::AutoObjectRooter rr(cx, result);
    if (!result)
        return JS_FALSE;

    if (!JS_DefineProperty(cx, result, "width", INT_TO_JSVAL(w), NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT) ||
        !JS_DefineProperty(cx, result, "height", INT_TO_JSVAL(h), NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT) ||
        !JS_DefineProperty(cx, result, "data", OBJECT_TO_JSVAL(darray), NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT))
        return JS_FALSE;

    *vp = OBJECT_TO_JSVAL(result);
    return JS_TRUE;
}

static JSBool
nsIDOMCanvasRenderingContext2D_PutImageData(JSContext *cx, uintN argc, jsval *vp)
{
    XPC_QS_ASSERT_CONTEXT_OK(cx);

    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    if (!obj)
        return JS_FALSE;

    nsresult rv;

    nsIDOMCanvasRenderingContext2D *self;
    xpc_qsSelfRef selfref;
    js::AutoValueRooter tvr(cx);
    if (!xpc_qsUnwrapThis(cx, obj, nsnull, &self, &selfref.ptr, tvr.jsval_addr(), nsnull))
        return JS_FALSE;

    if (argc < 3)
        return xpc_qsThrow(cx, NS_ERROR_XPC_NOT_ENOUGH_ARGS);

    jsval *argv = JS_ARGV(cx, vp);

    if (JSVAL_IS_PRIMITIVE(argv[0]))
        return xpc_qsThrow(cx, NS_ERROR_DOM_TYPE_MISMATCH_ERR);

    JSObject *dataObject = JSVAL_TO_OBJECT(argv[0]);
    int32 x, y;
    if (!JS_ValueToECMAInt32(cx, argv[1], &x) ||
        !JS_ValueToECMAInt32(cx, argv[2], &y))
        return JS_FALSE;

    int32 wi, hi;
    JSObject *darray;

    
    js::AutoValueRooter tv(cx);

    if (!JS_GetProperty(cx, dataObject, "width", tv.jsval_addr()) ||
        !JS_ValueToECMAInt32(cx, tv.jsval_value(), &wi))
        return JS_FALSE;

    if (!JS_GetProperty(cx, dataObject, "height", tv.jsval_addr()) ||
        !JS_ValueToECMAInt32(cx, tv.jsval_value(), &hi))
        return JS_FALSE;

    if (wi <= 0 || hi <= 0)
        return xpc_qsThrow(cx, NS_ERROR_DOM_INDEX_SIZE_ERR);

    uint32 w = (uint32) wi;
    uint32 h = (uint32) hi;

    
    PRBool hasDirtyRect = PR_FALSE;
    int32 dirtyX = 0,
          dirtyY = 0,
          dirtyWidth = w,
          dirtyHeight = h;

    if (argc >= 7) {
        if (!JS_ValueToECMAInt32(cx, argv[3], &dirtyX) ||
            !JS_ValueToECMAInt32(cx, argv[4], &dirtyY) ||
            !JS_ValueToECMAInt32(cx, argv[5], &dirtyWidth) ||
            !JS_ValueToECMAInt32(cx, argv[6], &dirtyHeight))
            return JS_FALSE;

        hasDirtyRect = PR_TRUE;
    }

    if (!JS_GetProperty(cx, dataObject, "data", tv.jsval_addr()) ||
        JSVAL_IS_PRIMITIVE(tv.jsval_value()))
        return JS_FALSE;
    darray = JSVAL_TO_OBJECT(tv.jsval_value());

    js::AutoValueRooter tsrc_tvr(cx);

    js::TypedArray *tsrc = NULL;
    if (darray->getClass() == &js::TypedArray::fastClasses[js::TypedArray::TYPE_UINT8] ||
        darray->getClass() == &js::TypedArray::fastClasses[js::TypedArray::TYPE_UINT8_CLAMPED])
    {
        tsrc = js::TypedArray::fromJSObject(darray);
    } else if (JS_IsArrayObject(cx, darray) || js_IsTypedArray(darray)) {
        
        JSObject *nobj = js_CreateTypedArrayWithArray(cx, js::TypedArray::TYPE_UINT8, darray);
        if (!nobj)
            return JS_FALSE;

        *tsrc_tvr.jsval_addr() = OBJECT_TO_JSVAL(nobj);
        tsrc = js::TypedArray::fromJSObject(nobj);
    } else {
        
        return xpc_qsThrow(cx, NS_ERROR_DOM_TYPE_MISMATCH_ERR);
    }

    
    rv = self->PutImageData_explicit(x, y, w, h, (PRUint8*) tsrc->data, tsrc->byteLength, hasDirtyRect, dirtyX, dirtyY, dirtyWidth, dirtyHeight);
    if (NS_FAILED(rv))
        return xpc_qsThrowMethodFailed(cx, rv, vp);

    *vp = JSVAL_VOID;
    return JS_TRUE;
}
