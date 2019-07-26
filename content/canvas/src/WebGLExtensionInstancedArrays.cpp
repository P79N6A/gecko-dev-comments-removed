




#include "WebGLContext.h"
#include "WebGLExtensions.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"

using namespace mozilla;

WebGLExtensionInstancedArrays::WebGLExtensionInstancedArrays(WebGLContext* context)
  : WebGLExtensionBase(context)
{
    MOZ_ASSERT(IsSupported(context), "should not construct WebGLExtensionInstancedArrays: "
                                     "ANGLE_instanced_arrays unsupported.");
}

WebGLExtensionInstancedArrays::~WebGLExtensionInstancedArrays()
{
}

void
WebGLExtensionInstancedArrays::DrawArraysInstancedANGLE(WebGLenum mode, WebGLint first,
                                                        WebGLsizei count, WebGLsizei primcount)
{
    mContext->DrawArraysInstanced(mode, first, count, primcount);
}

void
WebGLExtensionInstancedArrays::DrawElementsInstancedANGLE(WebGLenum mode, WebGLsizei count,
                                                          WebGLenum type, WebGLintptr offset,
                                                          WebGLsizei primcount)
{
    mContext->DrawElementsInstanced(mode, count, type, offset, primcount);
}

void
WebGLExtensionInstancedArrays::VertexAttribDivisorANGLE(WebGLuint index, WebGLuint divisor)
{
    mContext->VertexAttribDivisor(index, divisor);
}

bool
WebGLExtensionInstancedArrays::IsSupported(const WebGLContext* context)
{
    gl::GLContext* gl = context->GL();

    return gl->IsExtensionSupported(gl::GLContext::XXX_draw_instanced) &&
           gl->IsExtensionSupported(gl::GLContext::XXX_instanced_arrays);
}

IMPL_WEBGL_EXTENSION_GOOP(WebGLExtensionInstancedArrays)
