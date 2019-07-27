





#ifndef WEBGLCONTEXTUNCHECKED_H
#define WEBGLCONTEXTUNCHECKED_H

#include "GLDefs.h"
#include "nsRefPtr.h"
#include "WebGLTypes.h"

namespace mozilla {

class WebGLBuffer;
class WebGLSampler;
namespace gl {
    class GLContext;
} 

class WebGLContextUnchecked
{
public:
    explicit WebGLContextUnchecked(gl::GLContext* gl);

    
    
    void BindBuffer(GLenum target, WebGLBuffer* buffer);
    void BindBufferBase(GLenum target, GLuint index, WebGLBuffer* buffer);
    void BindBufferRange(GLenum taret, GLuint index, WebGLBuffer* buffer, WebGLintptr offset, WebGLsizeiptr size);
    void CopyBufferSubData(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size);

    
    
    void BindSampler(GLuint unit, WebGLSampler* sampler);

    GLint   GetSamplerParameteriv(WebGLSampler* sampler, GLenum pname);
    GLfloat GetSamplerParameterfv(WebGLSampler* sampler, GLenum pname);

    void SamplerParameteri(WebGLSampler* sampler, GLenum pname, GLint param);
    void SamplerParameteriv(WebGLSampler* sampler, GLenum pname, const GLint* param);
    void SamplerParameterf(WebGLSampler* sampler, GLenum pname, GLfloat param);
    void SamplerParameterfv(WebGLSampler* sampler, GLenum pname, const GLfloat* param);

protected: 
    nsRefPtr<gl::GLContext> gl;
};

} 

#endif 
