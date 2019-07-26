




#include "WebGLContext.h"
#include "WebGLUniformLocation.h"
#include "WebGLShader.h"
#include "WebGLProgram.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"

using namespace mozilla;

JSObject*
WebGLUniformLocation::WrapObject(JSContext *cx)
{
    return dom::WebGLUniformLocationBinding::Wrap(cx, this);
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

NS_IMPL_CYCLE_COLLECTION(WebGLUniformLocation, mProgram)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(WebGLUniformLocation, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(WebGLUniformLocation, Release)
