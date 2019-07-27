




#include "WebGLContext.h"
#include "WebGLShaderPrecisionFormat.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"

using namespace mozilla;

JSObject*
WebGLShaderPrecisionFormat::WrapObject(JSContext *cx)
{
    return dom::WebGLShaderPrecisionFormatBinding::Wrap(cx, this);
}
