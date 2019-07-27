




#include "WebGLShaderPrecisionFormat.h"

#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "WebGLContext.h"

namespace mozilla {

JSObject*
WebGLShaderPrecisionFormat::WrapObject(JSContext* cx)
{
    return dom::WebGLShaderPrecisionFormatBinding::Wrap(cx, this);
}

} 
