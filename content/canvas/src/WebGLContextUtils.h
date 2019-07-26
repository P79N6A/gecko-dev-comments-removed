




#ifndef WEBGLCONTEXTUTILS_H_
#define WEBGLCONTEXTUTILS_H_

#include "WebGLContext.h"
#include "mozilla/Assertions.h"
#include "mozilla/dom/BindingUtils.h"

namespace mozilla {

bool IsGLDepthFormat(GLenum internalFormat);
bool IsGLDepthStencilFormat(GLenum internalFormat);

template <typename WebGLObjectType>
JS::Value
WebGLContext::WebGLObjectAsJSValue(JSContext *cx, const WebGLObjectType *object, ErrorResult& rv) const
{
    if (!object) {
        return JS::NullValue();
    }
    MOZ_ASSERT(this == object->Context());
    JS::Rooted<JS::Value> v(cx);
    JS::Rooted<JSObject*> wrapper(cx, GetWrapper());
    JSAutoCompartment ac(cx, wrapper);
    if (!dom::WrapNewBindingObject(cx, const_cast<WebGLObjectType*>(object), &v)) {
        rv.Throw(NS_ERROR_FAILURE);
        return JS::NullValue();
    }
    return v;
}

template <typename WebGLObjectType>
JSObject*
WebGLContext::WebGLObjectAsJSObject(JSContext *cx, const WebGLObjectType *object, ErrorResult& rv) const
{
    JS::Value v = WebGLObjectAsJSValue(cx, object, rv);
    if (v.isNull()) {
        return nullptr;
    }
    return &v.toObject();
}

} 

#endif 
