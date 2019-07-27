




#ifndef WEBGL2CONTEXT_H_
#define WEBGL2CONTEXT_H_

#include "WebGLContext.h"

namespace mozilla {

class WebGLSampler;
class WebGLSync;
class WebGLTransformFeedback;
class WebGLVertexArrayObject;

class WebGL2Context
    : public WebGLContext
{
public:

    virtual ~WebGL2Context();

    static bool IsSupported();
    static WebGL2Context* Create();

    virtual bool IsWebGL2() const MOZ_OVERRIDE
    {
        return true;
    }

    
    

    virtual JSObject* WrapObject(JSContext *cx) MOZ_OVERRIDE;


    
    

    void CopyBufferSubData(GLenum readTarget, GLenum writeTarget,
                           GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size);
    void GetBufferSubData(GLenum target, GLintptr offset, const dom::ArrayBuffer& returnedData);
    void GetBufferSubData(GLenum target, GLintptr offset, const dom::ArrayBufferView& returnedData);

    
    

    void BlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                         GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                         GLbitfield mask, GLenum filter);
    void FramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer);
    void GetInternalformatParameter(JSContext*, GLenum target, GLenum internalformat, GLenum pname, JS::MutableHandleValue retval);
    void InvalidateFramebuffer(GLenum target, const dom::Sequence<GLenum>& attachments);
    void InvalidateSubFramebuffer (GLenum target, const dom::Sequence<GLenum>& attachments, GLint x, GLint y,
                                   GLsizei width, GLsizei height);
    void ReadBuffer(GLenum mode);
    void RenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat,
                                        GLsizei width, GLsizei height);


    
    

    void TexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
    void TexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height,
                      GLsizei depth);

    void TexSubImage3D(GLenum target, GLint level,
                       GLint xoffset, GLint yoffset, GLint zoffset,
                       GLsizei width, GLsizei height, GLsizei depth,
                       GLenum format, GLenum type, const Nullable<dom::ArrayBufferView>& pixels,
                       ErrorResult& rv);
    void TexSubImage3D(GLenum target, GLint level,
                       GLint xoffset, GLint yoffset, GLint zoffset,
                       GLenum format, GLenum type, dom::ImageData* data,
                       ErrorResult& rv);
    template<class ElementType>
    void TexSubImage3D(GLenum target, GLint level,
                       GLint xoffset, GLint yoffset, GLint zoffset,
                       GLenum format, GLenum type, ElementType& elt, ErrorResult& rv)
    {}

    void CopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
                           GLint x, GLint y, GLsizei width, GLsizei height);
    void CompressedTexImage3D(GLenum target, GLint level, GLenum internalformat,
                              GLsizei width, GLsizei height, GLsizei depth,
                              GLint border, GLsizei imageSize, const dom::ArrayBufferView& data);
    void CompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
                                 GLsizei width, GLsizei height, GLsizei depth,
                                 GLenum format, GLsizei imageSize, const dom::ArrayBufferView& data);


    
    
    GLint GetFragDataLocation(WebGLProgram* program, const nsAString& name);


    
    
    void VertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, GLintptr offset);

    void Uniform1ui(WebGLUniformLocation* location, GLuint v0);
    void Uniform2ui(WebGLUniformLocation* location, GLuint v0, GLuint v1);
    void Uniform3ui(WebGLUniformLocation* location, GLuint v0, GLuint v1, GLuint v2);
    void Uniform4ui(WebGLUniformLocation* location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
    void Uniform1uiv(WebGLUniformLocation* location, const dom::Sequence<GLuint>& value);
    void Uniform2uiv(WebGLUniformLocation* location, const dom::Sequence<GLuint>& value);
    void Uniform3uiv(WebGLUniformLocation* location, const dom::Sequence<GLuint>& value);
    void Uniform4uiv(WebGLUniformLocation* location, const dom::Sequence<GLuint>& value);
    void UniformMatrix2x3fv(WebGLUniformLocation* location, bool transpose, const dom::Float32Array& value);
    void UniformMatrix2x3fv(WebGLUniformLocation* location, bool transpose, const dom::Sequence<GLfloat>& value);
    void UniformMatrix3x2fv(WebGLUniformLocation* location, bool transpose, const dom::Float32Array& value);
    void UniformMatrix3x2fv(WebGLUniformLocation* location, bool transpose, const dom::Sequence<GLfloat>& value);
    void UniformMatrix2x4fv(WebGLUniformLocation* location, bool transpose, const dom::Float32Array& value);
    void UniformMatrix2x4fv(WebGLUniformLocation* location, bool transpose, const dom::Sequence<GLfloat>& value);
    void UniformMatrix4x2fv(WebGLUniformLocation* location, bool transpose, const dom::Float32Array& value);
    void UniformMatrix4x2fv(WebGLUniformLocation* location, bool transpose, const dom::Sequence<GLfloat>& value);
    void UniformMatrix3x4fv(WebGLUniformLocation* location, bool transpose, const dom::Float32Array& value);
    void UniformMatrix3x4fv(WebGLUniformLocation* location, bool transpose, const dom::Sequence<GLfloat>& value);
    void UniformMatrix4x3fv(WebGLUniformLocation* location, bool transpose, const dom::Float32Array& value);
    void UniformMatrix4x3fv(WebGLUniformLocation* location, bool transpose, const dom::Sequence<GLfloat>& value);

    void VertexAttribI4i(GLuint index, GLint x, GLint y, GLint z, GLint w);
    void VertexAttribI4iv(GLuint index, const dom::Sequence<GLint>& v);
    void VertexAttribI4ui(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w);
    void VertexAttribI4uiv(GLuint index, const dom::Sequence<GLuint>& v);


    
    
    





    void DrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, GLintptr offset);


    
    
    



    void ClearBufferiv(GLenum buffer, GLint drawbuffer, const dom::Int32Array& value);
    void ClearBufferiv(GLenum buffer, GLint drawbuffer, const dom::Sequence<GLint>& value);
    void ClearBufferuiv(GLenum buffer, GLint drawbuffer, const dom::Uint32Array& value);
    void ClearBufferuiv(GLenum buffer, GLint drawbuffer, const dom::Sequence<GLuint>& value);
    void ClearBufferfv(GLenum buffer, GLint drawbuffer, const dom::Float32Array& value);
    void ClearBufferfv(GLenum buffer, GLint drawbuffer, const dom::Sequence<GLfloat>& value);
    void ClearBufferfi(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil);


    
    
    
    





    void GetQueryParameter(JSContext*, WebGLQuery* query, GLenum pname, JS::MutableHandleValue retval);

    
    

    already_AddRefed<WebGLSampler> CreateSampler();
    void DeleteSampler(WebGLSampler* sampler);
    bool IsSampler(WebGLSampler* sampler);
    void BindSampler(GLuint unit, WebGLSampler* sampler);
    void SamplerParameteri(WebGLSampler* sampler, GLenum pname, GLint param);
    void SamplerParameteriv(WebGLSampler* sampler, GLenum pname, const dom::Int32Array& param);
    void SamplerParameteriv(WebGLSampler* sampler, GLenum pname, const dom::Sequence<GLint>& param);
    void SamplerParameterf(WebGLSampler* sampler, GLenum pname, GLfloat param);
    void SamplerParameterfv(WebGLSampler* sampler, GLenum pname, const dom::Float32Array& param);
    void SamplerParameterfv(WebGLSampler* sampler, GLenum pname, const dom::Sequence<GLfloat>& param);
    void GetSamplerParameter(JSContext*, WebGLSampler* sampler, GLenum pname, JS::MutableHandleValue retval);


    
    

    already_AddRefed<WebGLSync> FenceSync(GLenum condition, GLbitfield flags);
    bool IsSync(WebGLSync* sync);
    void DeleteSync(WebGLSync* sync);
    GLenum ClientWaitSync(WebGLSync* sync, GLbitfield flags, GLuint64 timeout);
    void WaitSync(WebGLSync* sync, GLbitfield flags, GLuint64 timeout);
    void GetSyncParameter(JSContext*, WebGLSync* sync, GLenum pname, JS::MutableHandleValue retval);


    
    
    already_AddRefed<WebGLTransformFeedback> CreateTransformFeedback();
    void DeleteTransformFeedback(WebGLTransformFeedback* tf);
    bool IsTransformFeedback(WebGLTransformFeedback* tf);
    void BindTransformFeedback(GLenum target, GLuint id);
    void BeginTransformFeedback(GLenum primitiveMode);
    void EndTransformFeedback();
    void TransformFeedbackVaryings(WebGLProgram* program, GLsizei count,
                                   const dom::Sequence<nsString>& varyings, GLenum bufferMode);
    already_AddRefed<WebGLActiveInfo> GetTransformFeedbackVarying(WebGLProgram* program, GLuint index);
    void PauseTransformFeedback();
    void ResumeTransformFeedback();


    
    
    




    void GetIndexedParameter(JSContext*, GLenum target, GLuint index, JS::MutableHandleValue retval);
    void GetUniformIndices(WebGLProgram* program, const dom::Sequence<nsString>& uniformNames, dom::Nullable< nsTArray<GLuint> >& retval);
    void GetActiveUniforms(WebGLProgram* program, const dom::Sequence<GLuint>& uniformIndices, GLenum pname,
                           dom::Nullable< nsTArray<GLint> >& retval);
    GLuint GetUniformBlockIndex(WebGLProgram* program, const nsAString& uniformBlockName);
    void GetActiveUniformBlockParameter(JSContext*, WebGLProgram* program, GLuint uniformBlockIndex, GLenum pname, JS::MutableHandleValue retval);
    void GetActiveUniformBlockName(WebGLProgram* program, GLuint uniformBlockIndex, dom::DOMString& retval);
    void UniformBlockBinding(WebGLProgram* program, GLuint uniformBlockIndex, GLuint uniformBlockBinding);


    
    
    







private:

    WebGL2Context();
};

} 

#endif
