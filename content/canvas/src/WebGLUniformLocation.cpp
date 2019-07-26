




#include "WebGLContext.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"

using namespace mozilla;

JSObject*
WebGLUniformLocation::WrapObject(JSContext *cx, JSObject *scope)
{
    return dom::WebGLUniformLocationBinding::Wrap(cx, scope, this);
}

WebGLUniformLocation::WebGLUniformLocation(WebGLContext *context, WebGLProgram *program, GLint location, const WebGLUniformInfo& info)
    : WebGLContextBoundObject(context)
    , mProgram(program)
    , mProgramGeneration(program->Generation())
    , mLocation(location)
    , mInfo(info)
{
    mElementSize = info.ElementSize();
}

NS_IMPL_CYCLE_COLLECTION_1(WebGLUniformLocation, mProgram)

NS_IMPL_CYCLE_COLLECTING_ADDREF(WebGLUniformLocation)
NS_IMPL_CYCLE_COLLECTING_RELEASE(WebGLUniformLocation)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(WebGLUniformLocation)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END
