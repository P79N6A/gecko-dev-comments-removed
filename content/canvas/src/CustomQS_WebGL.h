









































#include "jsapi.h"
#include "jsfriendapi.h"
#include "CustomQS_Canvas.h"

#define GET_INT32_ARG(var, index) \
  int32_t var; \
  do { \
    if (!JS_ValueToECMAInt32(cx, argv[index], &(var))) \
      return JS_FALSE; \
  } while (0)

#define GET_UINT32_ARG(var, index) \
  uint32_t var; \
  do { \
    if (!JS_ValueToECMAUint32(cx, argv[index], &(var))) \
      return JS_FALSE; \
  } while (0)

class CallTexImage2D
{
private:
    nsIDOMWebGLRenderingContext* self;
    WebGLenum target;
    WebGLint level;
    WebGLenum internalformat;
    WebGLenum format;
    WebGLenum type;

public:
    explicit CallTexImage2D(nsIDOMWebGLRenderingContext* aSelf,
                            WebGLenum aTarget,
                            WebGLint aLevel,
                            WebGLenum aInternalformat,
                            WebGLenum aFormat,
                            WebGLenum aType)
        : self(aSelf)
        , target(aTarget)
        , level(aLevel)
        , internalformat(aInternalformat)
        , format(aFormat)
        , type(aType)
    {}

    nsresult DoCallForImageData(WebGLsizei width, WebGLsizei height,
                                JSObject* pixels, JSContext *cx)
    {
        return self->TexImage2D_imageData(target, level, internalformat, width,
                                          height, 0, format, type, pixels, cx);
    }
    nsresult DoCallForElement(mozilla::dom::Element* elt)
    {
        return self->TexImage2D_dom(target, level, internalformat, format, type,
                                    elt);
    }
};

class CallTexSubImage2D
{
private:
    nsIDOMWebGLRenderingContext* self;
    WebGLenum target;
    WebGLint level;
    WebGLint xoffset;
    WebGLint yoffset;
    WebGLenum format;
    WebGLenum type;

public:
    explicit CallTexSubImage2D(nsIDOMWebGLRenderingContext* aSelf,
                               WebGLenum aTarget,
                               WebGLint aLevel,
                               WebGLint aXoffset,
                               WebGLint aYoffset,
                               WebGLenum aFormat,
                               WebGLenum aType)

        : self(aSelf)
        , target(aTarget)
        , level(aLevel)
        , xoffset(aXoffset)
        , yoffset(aYoffset)
        , format(aFormat)
        , type(aType)
    {}

    nsresult DoCallForImageData(WebGLsizei width, WebGLsizei height,
                                JSObject* pixels, JSContext *cx)
    {
        return self->TexSubImage2D_imageData(target, level, xoffset, yoffset,
                                             width, height, format, type,
                                             pixels, cx);
    }
    nsresult DoCallForElement(mozilla::dom::Element* elt)
    {
        return self->TexSubImage2D_dom(target, level, xoffset, yoffset, format,
                                       type, elt);
    }
};

template<class T>
static bool
TexImage2DImageDataOrElement(JSContext* cx, T& self, JS::Value* object)
{
    MOZ_ASSERT(object && object->isObject());

    nsGenericElement* elt;
    xpc_qsSelfRef eltRef;
    if (NS_SUCCEEDED(xpc_qsUnwrapArg<nsGenericElement>(
            cx, *object, &elt, &eltRef.ptr, object))) {
        nsresult rv = self.DoCallForElement(elt);
        return NS_SUCCEEDED(rv) || xpc_qsThrow(cx, rv);
    }

    
    
    uint32_t int_width, int_height;
    JS::Anchor<JSObject*> obj_data;
    if (!GetImageData(cx, *object, &int_width, &int_height, &obj_data)) {
        return false;
    }
    if (!JS_IsTypedArrayObject(obj_data.get(), cx)) {
        return xpc_qsThrow(cx, NS_ERROR_FAILURE);
    }

    nsresult rv = self.DoCallForImageData(int_width, int_height, obj_data.get(), cx);
    return NS_SUCCEEDED(rv) || xpc_qsThrow(cx, rv);
}







static JSBool
nsIDOMWebGLRenderingContext_TexImage2D(JSContext *cx, unsigned argc, jsval *vp)
{
    XPC_QS_ASSERT_CONTEXT_OK(cx);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    if (!obj)
        return JS_FALSE;

    nsresult rv;

    nsIDOMWebGLRenderingContext *self;
    xpc_qsSelfRef selfref;
    JS::AutoValueRooter tvr(cx);
    if (!xpc_qsUnwrapThis(cx, obj, &self, &selfref.ptr, tvr.jsval_addr(), nsnull))
        return JS_FALSE;

    if (argc < 6 || argc == 7 || argc == 8)
        return xpc_qsThrow(cx, NS_ERROR_XPC_NOT_ENOUGH_ARGS);

    JS::Value* argv = JS_ARGV(cx, vp);

    
    GET_UINT32_ARG(argv0, 0);
    GET_INT32_ARG(argv1, 1);
    GET_UINT32_ARG(argv2, 2);

    if (argc > 5 && argv[5].isObject()) {
        
        GET_UINT32_ARG(argv3, 3);
        GET_UINT32_ARG(argv4, 4);

        CallTexImage2D selfCaller(self, argv0, argv1, argv2, argv3, argv4);
        if (!TexImage2DImageDataOrElement(cx, selfCaller, argv + 5)) {
            return false;
        }
        rv = NS_OK;
    } else if (argc > 8 && argv[8].isObjectOrNull()) {
        
        
        GET_INT32_ARG(argv3, 3);
        GET_INT32_ARG(argv4, 4);
        GET_INT32_ARG(argv5, 5);
        GET_UINT32_ARG(argv6, 6);
        GET_UINT32_ARG(argv7, 7);

        JSObject* argv8 = argv[8].toObjectOrNull();

        
        if (argv8 == nsnull) {
            rv = self->TexImage2D_array(argv0, argv1, argv2, argv3,
                                        argv4, argv5, argv6, argv7,
                                        nsnull, cx);
        } else if (JS_IsTypedArrayObject(argv8, cx)) {
            rv = self->TexImage2D_array(argv0, argv1, argv2, argv3,
                                        argv4, argv5, argv6, argv7,
                                        argv8, cx);
        } else {
            xpc_qsThrowBadArg(cx, NS_ERROR_FAILURE, vp, 8);
            return JS_FALSE;
        }
    } else {
        xpc_qsThrow(cx, NS_ERROR_XPC_NOT_ENOUGH_ARGS);
        return JS_FALSE;
    }

    if (NS_FAILED(rv))
        return xpc_qsThrowMethodFailed(cx, rv, vp);

    *vp = JSVAL_VOID;
    return JS_TRUE;
}






static JSBool
nsIDOMWebGLRenderingContext_TexSubImage2D(JSContext *cx, unsigned argc, jsval *vp)
{
    XPC_QS_ASSERT_CONTEXT_OK(cx);
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    if (!obj)
        return JS_FALSE;

    nsresult rv;

    nsIDOMWebGLRenderingContext *self;
    xpc_qsSelfRef selfref;
    JS::AutoValueRooter tvr(cx);
    if (!xpc_qsUnwrapThis(cx, obj, &self, &selfref.ptr, tvr.jsval_addr(), nsnull))
        return JS_FALSE;

    if (argc < 7 || argc == 8)
        return xpc_qsThrow(cx, NS_ERROR_XPC_NOT_ENOUGH_ARGS);

    jsval *argv = JS_ARGV(cx, vp);

    
    GET_UINT32_ARG(argv0, 0);
    GET_INT32_ARG(argv1, 1);
    GET_INT32_ARG(argv2, 2);
    GET_INT32_ARG(argv3, 3);

    if (argc > 6 && !JSVAL_IS_PRIMITIVE(argv[6])) {
        
        GET_UINT32_ARG(argv4, 4);
        GET_UINT32_ARG(argv5, 5);

        CallTexSubImage2D selfCaller(self, argv0, argv1, argv2, argv3, argv4, argv5);
        if (!TexImage2DImageDataOrElement(cx, selfCaller, argv + 6)) {
            return false;
        }
        rv = NS_OK;
    } else if (argc > 8 && !JSVAL_IS_PRIMITIVE(argv[8])) {
        
        GET_INT32_ARG(argv4, 4);
        GET_INT32_ARG(argv5, 5);
        GET_UINT32_ARG(argv6, 6);
        GET_UINT32_ARG(argv7, 7);

        JSObject *argv8 = JSVAL_TO_OBJECT(argv[8]);
        
        if (JS_IsTypedArrayObject(argv8, cx)) {
            rv = self->TexSubImage2D_array(argv0, argv1, argv2, argv3,
                                           argv4, argv5, argv6, argv7,
                                           argv8, cx);
        } else {
            xpc_qsThrowBadArg(cx, NS_ERROR_FAILURE, vp, 8);
            return JS_FALSE;
        }
    } else {
        xpc_qsThrow(cx, NS_ERROR_XPC_NOT_ENOUGH_ARGS);
        return JS_FALSE;
    }

    if (NS_FAILED(rv))
        return xpc_qsThrowMethodFailed(cx, rv, vp);

    *vp = JSVAL_VOID;
    return JS_TRUE;
}
