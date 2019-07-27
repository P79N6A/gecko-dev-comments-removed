





#include "WebGLContextUnchecked.h"

#include "GLContext.h"
#include "WebGLBuffer.h"
#include "WebGLSampler.h"

namespace mozilla {

WebGLContextUnchecked::WebGLContextUnchecked(gl::GLContext* gl)
    : gl(gl)
{ }





void
WebGLContextUnchecked::BindBuffer(GLenum target, WebGLBuffer* buffer)
{
    gl->MakeCurrent();
    gl->fBindBuffer(target, buffer ? buffer->GLName() : 0);
}

void
WebGLContextUnchecked::BindBufferBase(GLenum target, GLuint index, WebGLBuffer* buffer)
{
    gl->MakeCurrent();
    gl->fBindBufferBase(target, index, buffer ? buffer->GLName() : 0);
}

void
WebGLContextUnchecked::BindBufferRange(GLenum target, GLuint index, WebGLBuffer* buffer, WebGLintptr offset, WebGLsizeiptr size)
{
    gl->MakeCurrent();
    gl->fBindBufferRange(target, index, buffer ? buffer->GLName() : 0, offset, size);
}

void
WebGLContextUnchecked::CopyBufferSubData(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size)
{
    gl->MakeCurrent();
    gl->fCopyBufferSubData(readTarget, writeTarget, readOffset, writeOffset, size);
}





void
WebGLContextUnchecked::BindSampler(GLuint unit, WebGLSampler* sampler)
{
    gl->MakeCurrent();
    gl->fBindSampler(unit, sampler ? sampler->mGLName : 0);
}

GLint
WebGLContextUnchecked::GetSamplerParameteriv(WebGLSampler* sampler,
                                             GLenum pname)
{
    MOZ_ASSERT(sampler, "Did you validate?");

    GLint param = 0;
    gl->MakeCurrent();
    gl->fGetSamplerParameteriv(sampler->mGLName, pname, &param);

    return param;
}

GLfloat
WebGLContextUnchecked::GetSamplerParameterfv(WebGLSampler* sampler,
                                             GLenum pname)
{
    MOZ_ASSERT(sampler, "Did you validate?");

    GLfloat param = 0.0f;
    gl->MakeCurrent();
    gl->fGetSamplerParameterfv(sampler->mGLName, pname, &param);
    return param;
}

void
WebGLContextUnchecked::SamplerParameteri(WebGLSampler* sampler,
                                         GLenum pname,
                                         GLint param)
{
    MOZ_ASSERT(sampler, "Did you validate?");
    gl->MakeCurrent();
    gl->fSamplerParameteri(sampler->mGLName, pname, param);
}

void
WebGLContextUnchecked::SamplerParameteriv(WebGLSampler* sampler,
                                          GLenum pname,
                                          const GLint* param)
{
    MOZ_ASSERT(sampler, "Did you validate?");
    gl->MakeCurrent();
    gl->fSamplerParameteriv(sampler->mGLName, pname, param);
}

void
WebGLContextUnchecked::SamplerParameterf(WebGLSampler* sampler,
                                         GLenum pname,
                                         GLfloat param)
{
    MOZ_ASSERT(sampler, "Did you validate?");
    gl->MakeCurrent();
    gl->fSamplerParameterf(sampler->mGLName, pname, param);
}

void
WebGLContextUnchecked::SamplerParameterfv(WebGLSampler* sampler,
                                          GLenum pname,
                                          const GLfloat* param)
{
    MOZ_ASSERT(sampler, "Did you validate?");
    gl->MakeCurrent();
    gl->fSamplerParameterfv(sampler->mGLName, pname, param);
}

} 
