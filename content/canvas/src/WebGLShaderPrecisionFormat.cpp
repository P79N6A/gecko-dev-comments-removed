




#include "WebGLContext.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"

using namespace mozilla;

NS_INTERFACE_MAP_BEGIN(WebGLShaderPrecisionFormat)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(WebGLShaderPrecisionFormat)
NS_IMPL_RELEASE(WebGLShaderPrecisionFormat)

JSObject*
WebGLShaderPrecisionFormat::WrapObject(JSContext *cx, JSObject *scope)
{
    return dom::WebGLShaderPrecisionFormatBinding::Wrap(cx, scope, this);
}
