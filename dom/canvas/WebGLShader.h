




#ifndef WEBGL_SHADER_H_
#define WEBGL_SHADER_H_

#include "GLDefs.h"
#include "mozilla/LinkedList.h"
#include "mozilla/MemoryReporting.h"
#include "nsWrapperCache.h"
#include "WebGLObjectModel.h"

namespace mozilla {

namespace webgl {
class ShaderValidator;
} 

class WebGLShader final
    : public nsWrapperCache
    , public WebGLRefCountedObject<WebGLShader>
    , public LinkedListElement<WebGLShader>
    , public WebGLContextBoundObject
{
    friend class WebGLContext;
    friend class WebGLProgram;

public:
    WebGLShader(WebGLContext* webgl, GLenum type);

protected:
    ~WebGLShader();

public:
    
    void CompileShader();
    JS::Value GetShaderParameter(GLenum pname) const;
    void GetShaderInfoLog(nsAString* out) const;
    void GetShaderSource(nsAString* out) const;
    void GetShaderTranslatedSource(nsAString* out) const;
    void ShaderSource(const nsAString& source);

    
    bool CanLinkTo(const WebGLShader* prev, nsCString* const out_log) const;
    size_t CalcNumSamplerUniforms() const;
    void BindAttribLocation(GLuint prog, const nsCString& userName, GLuint index) const;
    bool FindAttribUserNameByMappedName(const nsACString& mappedName,
                                        nsDependentCString* const out_userName) const;
    bool FindUniformByMappedName(const nsACString& mappedName,
                                 nsCString* const out_userName,
                                 bool* const out_isArray) const;
    bool FindUniformBlockByMappedName(const nsACString& mappedName,
                                      nsCString* const out_userName,
                                      bool* const out_isArray) const;

    bool IsCompiled() const {
        return mTranslationSuccessful && mCompilationSuccessful;
    }

    void ApplyTransformFeedbackVaryings(GLuint prog,
                                        const std::vector<nsCString>& varyings,
                                        GLenum bufferMode,
                                        std::vector<std::string>* out_mappedVaryings) const;

    
    size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;
    void Delete();

    WebGLContext* GetParentObject() const { return Context(); }

    virtual JSObject* WrapObject(JSContext* js, JS::Handle<JSObject*> aGivenProto) override;

    NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(WebGLShader)
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(WebGLShader)

public:
    const GLuint mGLName;
    const GLenum mType;
protected:
    nsString mSource;
    nsCString mCleanSource;

    UniquePtr<webgl::ShaderValidator> mValidator;
    nsCString mValidationLog;
    bool mTranslationSuccessful;
    nsCString mTranslatedSource;

    bool mCompilationSuccessful;
    nsCString mCompilationLog;
};

} 

#endif 
