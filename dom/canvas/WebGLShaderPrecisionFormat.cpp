




#include "WebGLShaderPrecisionFormat.h"

#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "WebGLContext.h"

namespace mozilla {

bool
WebGLShaderPrecisionFormat::WrapObject(JSContext* cx,
                                       JS::Handle<JSObject*> givenProto,
                                       JS::MutableHandle<JSObject*> reflector)
{
    return dom::WebGLShaderPrecisionFormatBinding::Wrap(cx, this, givenProto, reflector);
}

} 
