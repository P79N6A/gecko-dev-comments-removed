




#ifndef WEBGLCONTEXTUTILS_H_
#define WEBGLCONTEXTUTILS_H_

#include "WebGLContext.h"
#include "WebGLStrongTypes.h"
#include "mozilla/Assertions.h"
#include "mozilla/dom/BindingUtils.h"

namespace mozilla {

bool IsGLDepthFormat(TexInternalFormat webGLFormat);
bool IsGLDepthStencilFormat(TexInternalFormat webGLFormat);
bool FormatHasAlpha(TexInternalFormat webGLFormat);
void
DriverFormatsFromEffectiveInternalFormat(gl::GLContext* gl,
                                         TexInternalFormat internalformat,
                                         GLenum* out_driverInternalFormat,
                                         GLenum* out_driverFormat,
                                         GLenum* out_driverType);
TexInternalFormat
EffectiveInternalFormatFromInternalFormatAndType(TexInternalFormat internalformat,
                                                 TexType type);
TexInternalFormat
EffectiveInternalFormatFromUnsizedInternalFormatAndType(TexInternalFormat internalformat,
                                                        TexType type);
void
UnsizedInternalFormatAndTypeFromEffectiveInternalFormat(TexInternalFormat effectiveinternalformat,
                                                        TexInternalFormat* out_internalformat,
                                                        TexType* out_type);
TexType
TypeFromInternalFormat(TexInternalFormat internalformat);
TexInternalFormat
UnsizedInternalFormatFromInternalFormat(TexInternalFormat internalformat);
size_t
GetBitsPerTexel(TexInternalFormat effectiveinternalformat);













TexTarget TexImageTargetToTexTarget(TexImageTarget texImageTarget);

struct GLComponents
{
    unsigned char mComponents;

    enum Components {
        Red     = (1 << 0),
        Green   = (1 << 1),
        Blue    = (1 << 2),
        Alpha   = (1 << 3),
        Stencil = (1 << 4),
        Depth   = (1 << 5),
    };

    GLComponents()
        : mComponents(0)
    { }

    explicit GLComponents(TexInternalFormat aFormat);

    
    
    bool IsSubsetOf(const GLComponents& other) const;
};

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
