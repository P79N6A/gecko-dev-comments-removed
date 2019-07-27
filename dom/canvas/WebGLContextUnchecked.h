





#ifndef WEBGLCONTEXTUNCHECKED_H
#define WEBGLCONTEXTUNCHECKED_H

#include "GLDefs.h"
#include "WebGLTypes.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"

namespace mozilla {

class WebGLSampler;
namespace gl {
    class GLContext;
}

class WebGLContextUnchecked
{
public:
    explicit WebGLContextUnchecked(gl::GLContext* gl);

    
    
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
