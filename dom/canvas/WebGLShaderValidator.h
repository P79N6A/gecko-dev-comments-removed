




#ifndef WEBGL_SHADER_VALIDATOR_H_
#define WEBGL_SHADER_VALIDATOR_H_

#include "angle/ShaderLang.h"
#include "GLDefs.h"
#include "nsString.h"
#include <string>

namespace mozilla {
namespace webgl {

class ShaderValidator final
{
    const ShHandle mHandle;
    const int mCompileOptions;
    const int mMaxVaryingVectors;
    bool mHasRun;

public:
    static ShaderValidator* Create(GLenum shaderType, ShShaderSpec spec,
                                   ShShaderOutput outputLanguage,
                                   const ShBuiltInResources& resources,
                                   int compileOptions);

private:
    ShaderValidator(ShHandle handle, int compileOptions, int maxVaryingVectors)
        : mHandle(handle)
        , mCompileOptions(compileOptions)
        , mMaxVaryingVectors(maxVaryingVectors)
        , mHasRun(false)
    { }

public:
    ~ShaderValidator();

    bool ValidateAndTranslate(const char* source);
    void GetInfoLog(nsACString* out) const;
    void GetOutput(nsACString* out) const;
    bool CanLinkTo(const ShaderValidator* prev, nsCString* const out_log) const;
    size_t CalcNumSamplerUniforms() const;

    bool FindAttribUserNameByMappedName(const std::string& mappedName,
                                        const std::string** const out_userName) const;

    bool FindAttribMappedNameByUserName(const std::string& userName,
                                        const std::string** const out_mappedName) const;

    bool FindUniformByMappedName(const std::string& mappedName,
                                 std::string* const out_userName,
                                 bool* const out_isArray) const;
};

} 
} 

#endif 
