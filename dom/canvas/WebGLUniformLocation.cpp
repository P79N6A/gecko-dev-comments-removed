




#include "WebGLUniformLocation.h"

#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "WebGLContext.h"
#include "WebGLProgram.h"
#include "WebGLShader.h"

namespace mozilla {

bool
WebGLUniformLocation::WrapObject(JSContext* aCx,
                                 JS::MutableHandle<JSObject*> aReflector)
{
    return dom::WebGLUniformLocationBinding::Wrap(aCx, this, aReflector);
}

WebGLUniformLocation::WebGLUniformLocation(WebGLContext* context,
                                           WebGLProgram* program,
                                           GLint location,
                                           const WebGLUniformInfo& info)
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

} 
