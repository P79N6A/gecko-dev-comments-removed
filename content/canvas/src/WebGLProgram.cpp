




#include "WebGLContext.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"

using namespace mozilla;

JSObject*
WebGLProgram::WrapObject(JSContext *cx, JSObject *scope, bool *triedToWrap) {
    return dom::WebGLProgramBinding::Wrap(cx, scope, this, triedToWrap);
}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(WebGLProgram)

NS_IMPL_CYCLE_COLLECTING_ADDREF(WebGLProgram)
NS_IMPL_CYCLE_COLLECTING_RELEASE(WebGLProgram)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(WebGLProgram)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END
