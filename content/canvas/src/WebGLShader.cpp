




#include "WebGLContext.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"

using namespace mozilla;

JSObject*
WebGLShader::WrapObject(JSContext *cx, JSObject *scope, bool *triedToWrap) {
    return dom::WebGLShaderBinding::Wrap(cx, scope, this, triedToWrap);
}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(WebGLShader)

NS_IMPL_CYCLE_COLLECTING_ADDREF(WebGLShader)
NS_IMPL_CYCLE_COLLECTING_RELEASE(WebGLShader)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(WebGLShader)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END
