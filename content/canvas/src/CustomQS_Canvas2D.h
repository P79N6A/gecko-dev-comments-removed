







































#include "nsDOMError.h"
#include "nsIDOMCanvasRenderingContext2D.h"
#include "CheckedInt.h"
#include "nsMathUtils.h"

typedef NS_STDCALL_FUNCPROTO(nsresult, CanvasStyleSetterType, nsIDOMCanvasRenderingContext2D,
                             SetStrokeStyle_multi, (const nsAString &, nsISupports *));
typedef NS_STDCALL_FUNCPROTO(nsresult, CanvasStyleGetterType, nsIDOMCanvasRenderingContext2D,
                             GetStrokeStyle_multi, (nsAString &, nsISupports **, PRInt32 *));

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

    nsresult rv = NS_OK;
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

        rv = (self->*setfunc)(NullString(), arg0);
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
nsIDOMCanvasRenderingContext2D_SetStrokeStyle(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)
{
    return Canvas2D_SetStyleHelper(cx, obj, id, vp, &nsIDOMCanvasRenderingContext2D::SetStrokeStyle_multi);
}

static JSBool
nsIDOMCanvasRenderingContext2D_GetStrokeStyle(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    return Canvas2D_GetStyleHelper(cx, obj, id, vp, &nsIDOMCanvasRenderingContext2D::GetStrokeStyle_multi);
}

static JSBool
nsIDOMCanvasRenderingContext2D_SetFillStyle(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)
{
    return Canvas2D_SetStyleHelper(cx, obj, id, vp, &nsIDOMCanvasRenderingContext2D::SetFillStyle_multi);
}

static JSBool
nsIDOMCanvasRenderingContext2D_GetFillStyle(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    return Canvas2D_GetStyleHelper(cx, obj, id, vp, &nsIDOMCanvasRenderingContext2D::GetFillStyle_multi);
}

static bool
CreateImageData(JSContext* cx,
                uint32 w,
                uint32 h,
                nsIDOMCanvasRenderingContext2D* self,
                int32 x,
                int32 y,
                jsval* vp)
{
    using mozilla::CheckedInt;

    if (w == 0)
        w = 1;
    if (h == 0)
        h = 1;

    CheckedInt<uint32_t> len = CheckedInt<uint32_t>(w) * h * 4;
    if (!len.valid()) {
        return xpc_qsThrow(cx, NS_ERROR_DOM_INDEX_SIZE_ERR);
    }

    
    JSObject* darray =
      js_CreateTypedArray(cx, js::TypedArray::TYPE_UINT8_CLAMPED, len.value());
    js::AutoObjectRooter rd(cx, darray);
    if (!darray) {
        return false;
    }

    if (self) {
        JSObject *tdest = js::TypedArray::getTypedArray(darray);

        
        nsresult rv =
            self->GetImageData_explicit(x, y, w, h,
                                        static_cast<PRUint8*>(JS_GetTypedArrayData(tdest)),
                                        JS_GetTypedArrayByteLength(tdest));
        if (NS_FAILED(rv)) {
            return xpc_qsThrowMethodFailed(cx, rv, vp);
        }
    }

    
    
    JSObject* result = JS_NewObject(cx, NULL, NULL, NULL);
    js::AutoObjectRooter rr(cx, result);
    if (!result) {
        return false;
    }

    if (!JS_DefineProperty(cx, result, "width", INT_TO_JSVAL(w), NULL, NULL,
                           JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT) ||
        !JS_DefineProperty(cx, result, "height", INT_TO_JSVAL(h), NULL, NULL,
                           JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT) ||
        !JS_DefineProperty(cx, result, "data", OBJECT_TO_JSVAL(darray), NULL, NULL,
                           JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT)) {
        return false;
    }

    *vp = OBJECT_TO_JSVAL(result);
    return true;
}

static bool
GetImageDataDimensions(JSContext *cx, JSObject *dataObject, uint32_t *width, uint32_t *height)
{
    jsval temp;
    int32_t wi, hi;
    
    
    
    
    
    if (!JS_GetProperty(cx, dataObject, "width", &temp) ||
        !JS_ValueToECMAInt32(cx, temp, &wi))
        return false;

    if (!JS_GetProperty(cx, dataObject, "height", &temp) ||
        !JS_ValueToECMAInt32(cx, temp, &hi))
        return false;

    if (wi <= 0 || hi <= 0)
        return xpc_qsThrow(cx, NS_ERROR_DOM_INDEX_SIZE_ERR);

    *width = uint32_t(wi);
    *height = uint32_t(hi);
    return true;
}

static JSBool
nsIDOMCanvasRenderingContext2D_CreateImageData(JSContext *cx, uintN argc, jsval *vp)
{
    XPC_QS_ASSERT_CONTEXT_OK(cx);

    

    if (argc < 1)
        return xpc_qsThrow(cx, NS_ERROR_XPC_NOT_ENOUGH_ARGS);

    jsval *argv = JS_ARGV(cx, vp);

    if (argc == 1) {
        
        
        if (JSVAL_IS_PRIMITIVE(argv[0]))
            return xpc_qsThrow(cx, NS_ERROR_DOM_NOT_SUPPORTED_ERR);

        JSObject *dataObject = JSVAL_TO_OBJECT(argv[0]);

        uint32_t data_width, data_height;
        if (!GetImageDataDimensions(cx, dataObject, &data_width, &data_height))
            return false;

        return CreateImageData(cx, data_width, data_height, NULL, 0, 0, vp);
    }

    jsdouble width, height;
    if (!JS_ValueToNumber(cx, argv[0], &width) ||
        !JS_ValueToNumber(cx, argv[1], &height))
        return false;

    if (!NS_finite(width) || !NS_finite(height))
        return xpc_qsThrow(cx, NS_ERROR_DOM_NOT_SUPPORTED_ERR);

    if (!width || !height)
        return xpc_qsThrow(cx, NS_ERROR_DOM_INDEX_SIZE_ERR);

    int32 wi = JS_DoubleToInt32(width);
    int32 hi = JS_DoubleToInt32(height);

    uint32 w = NS_ABS(wi);
    uint32 h = NS_ABS(hi);
    return CreateImageData(cx, w, h, NULL, 0, 0, vp);
}

static JSBool
nsIDOMCanvasRenderingContext2D_GetImageData(JSContext *cx, uintN argc, jsval *vp)
{
    XPC_QS_ASSERT_CONTEXT_OK(cx);

    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    if (!obj)
        return JS_FALSE;

    nsIDOMCanvasRenderingContext2D *self;
    xpc_qsSelfRef selfref;
    js::AutoValueRooter tvr(cx);
    if (!xpc_qsUnwrapThis(cx, obj, nsnull, &self, &selfref.ptr, tvr.jsval_addr(), nsnull))
        return JS_FALSE;

    if (argc < 4)
        return xpc_qsThrow(cx, NS_ERROR_XPC_NOT_ENOUGH_ARGS);

    jsval *argv = JS_ARGV(cx, vp);

    jsdouble xd, yd, width, height;
    if (!JS_ValueToNumber(cx, argv[0], &xd) ||
        !JS_ValueToNumber(cx, argv[1], &yd) ||
        !JS_ValueToNumber(cx, argv[2], &width) ||
        !JS_ValueToNumber(cx, argv[3], &height))
        return false;

    if (!NS_finite(xd) || !NS_finite(yd) ||
        !NS_finite(width) || !NS_finite(height))
        return xpc_qsThrow(cx, NS_ERROR_DOM_NOT_SUPPORTED_ERR);

    if (!width || !height)
        return xpc_qsThrow(cx, NS_ERROR_DOM_INDEX_SIZE_ERR);

    int32 x = JS_DoubleToInt32(xd);
    int32 y = JS_DoubleToInt32(yd);
    int32 wi = JS_DoubleToInt32(width);
    int32 hi = JS_DoubleToInt32(height);

    
    
    uint32 w, h;
    if (width < 0) {
        w = -wi;
        x -= w;
    } else {
        w = wi;
    }
    if (height < 0) {
        h = -hi;
        y -= h;
    } else {
        h = hi;
    }
    return CreateImageData(cx, w, h, self, x, y, vp);
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

    double xd, yd;
    if (!JS_ValueToNumber(cx, argv[1], &xd) ||
        !JS_ValueToNumber(cx, argv[2], &yd)) {
        return false;
    }

    if (!NS_finite(xd) || !NS_finite(yd)) {
        return xpc_qsThrow(cx, NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    }

    int32 x = JS_DoubleToInt32(xd);
    int32 y = JS_DoubleToInt32(yd);

    
    js::AutoValueRooter tv(cx);

    uint32_t w, h;
    if (!GetImageDataDimensions(cx, dataObject, &w, &h))
        return JS_FALSE;

    
    bool hasDirtyRect = false;
    int32 dirtyX = 0,
          dirtyY = 0,
          dirtyWidth = w,
          dirtyHeight = h;

    if (argc >= 7) {
        double dx, dy, dw, dh;
        if (!JS_ValueToNumber(cx, argv[3], &dx) ||
            !JS_ValueToNumber(cx, argv[4], &dy) ||
            !JS_ValueToNumber(cx, argv[5], &dw) ||
            !JS_ValueToNumber(cx, argv[6], &dh)) {
            return false;
        }

        if (!NS_finite(dx) || !NS_finite(dy) ||
            !NS_finite(dw) || !NS_finite(dh)) {
            return xpc_qsThrow(cx, NS_ERROR_DOM_NOT_SUPPORTED_ERR);
        }

        dirtyX = JS_DoubleToInt32(dx);
        dirtyY = JS_DoubleToInt32(dy);
        dirtyWidth = JS_DoubleToInt32(dw);
        dirtyHeight = JS_DoubleToInt32(dh);

        hasDirtyRect = true;
    }

    if (!JS_GetProperty(cx, dataObject, "data", tv.jsval_addr()))
        return JS_FALSE;

    if (JSVAL_IS_PRIMITIVE(tv.jsval_value()))
        return xpc_qsThrow(cx, NS_ERROR_DOM_TYPE_MISMATCH_ERR);

    JSObject *darray = JSVAL_TO_OBJECT(tv.jsval_value());

    js::AutoValueRooter tsrc_tvr(cx);

    JSObject *tsrc = NULL;
    if (js::GetObjectClass(darray) == &js::TypedArray::fastClasses[js::TypedArray::TYPE_UINT8] ||
        js::GetObjectClass(darray) == &js::TypedArray::fastClasses[js::TypedArray::TYPE_UINT8_CLAMPED])
    {
        tsrc = js::TypedArray::getTypedArray(darray);
    } else if (JS_IsArrayObject(cx, darray) || js_IsTypedArray(darray)) {
        
        JSObject *nobj = js_CreateTypedArrayWithArray(cx, js::TypedArray::TYPE_UINT8, darray);
        if (!nobj)
            return JS_FALSE;

        *tsrc_tvr.jsval_addr() = OBJECT_TO_JSVAL(nobj);
        tsrc = js::TypedArray::getTypedArray(nobj);
    } else {
        
        return xpc_qsThrow(cx, NS_ERROR_DOM_TYPE_MISMATCH_ERR);
    }

    
    rv = self->PutImageData_explicit(x, y, w, h, static_cast<PRUint8*>(JS_GetTypedArrayData(tsrc)), JS_GetTypedArrayByteLength(tsrc), hasDirtyRect, dirtyX, dirtyY, dirtyWidth, dirtyHeight);
    if (NS_FAILED(rv))
        return xpc_qsThrowMethodFailed(cx, rv, vp);

    *vp = JSVAL_VOID;
    return JS_TRUE;
}
