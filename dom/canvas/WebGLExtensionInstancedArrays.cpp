




#include "WebGLContext.h"
#include "WebGLExtensions.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "GLContext.h"

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
WebGLExtensionInstancedArrays::DrawArraysInstancedANGLE(GLenum mode, GLint first,
                                                        GLsizei count, GLsizei primcount)
{
    if (mIsLost)
        return mContext->GenerateWarning("drawArraysInstancedANGLE: Extension is lost.");

    mContext->DrawArraysInstanced(mode, first, count, primcount);
}

void
WebGLExtensionInstancedArrays::DrawElementsInstancedANGLE(GLenum mode, GLsizei count,
                                                          GLenum type, WebGLintptr offset,
                                                          GLsizei primcount)
{
    if (mIsLost)
        return mContext->GenerateWarning("drawElementsInstancedANGLE: Extension is lost.");

    mContext->DrawElementsInstanced(mode, count, type, offset, primcount);
}

void
WebGLExtensionInstancedArrays::VertexAttribDivisorANGLE(GLuint index, GLuint divisor)
{
    if (mIsLost)
        return mContext->GenerateWarning("vertexAttribDivisorANGLE: Extension is lost.");

    mContext->VertexAttribDivisor(index, divisor);
}

bool
WebGLExtensionInstancedArrays::IsSupported(const WebGLContext* context)
{
    gl::GLContext* gl = context->GL();

    return gl->IsSupported(gl::GLFeature::draw_instanced) &&
           gl->IsSupported(gl::GLFeature::instanced_arrays);
}

IMPL_WEBGL_EXTENSION_GOOP(WebGLExtensionInstancedArrays)
