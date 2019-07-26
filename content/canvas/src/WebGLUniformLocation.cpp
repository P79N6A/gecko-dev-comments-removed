




#include "WebGLContext.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"

using namespace mozilla;

JSObject*
WebGLUniformLocation::WrapObject(JSContext *cx, JSObject *scope)
{
    return dom::WebGLUniformLocationBinding::Wrap(cx, scope, this);
}

NS_IMPL_CYCLE_COLLECTION_CLASS(WebGLUniformLocation)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(WebGLUniformLocation)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mProgram)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(WebGLUniformLocation)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mProgram)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(WebGLUniformLocation)
NS_IMPL_CYCLE_COLLECTING_RELEASE(WebGLUniformLocation)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(WebGLUniformLocation)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END
