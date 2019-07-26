










#include "GLSLANG/ShaderLang.h"

#include "compiler/InitializeDll.h"
#include "compiler/preprocessor/length_limits.h"
#include "compiler/ShHandle.h"






static bool checkActiveUniformAndAttribMaxLengths(const ShHandle handle,
                                                  int expectedValue)
{
    int activeUniformLimit = 0;
    ShGetInfo(handle, SH_ACTIVE_UNIFORM_MAX_LENGTH, &activeUniformLimit);
    int activeAttribLimit = 0;
    ShGetInfo(handle, SH_ACTIVE_ATTRIBUTE_MAX_LENGTH, &activeAttribLimit);
    return (expectedValue == activeUniformLimit && expectedValue == activeAttribLimit);
}

static bool checkMappedNameMaxLength(const ShHandle handle, int expectedValue)
{
    int mappedNameMaxLength = 0;
    ShGetInfo(handle, SH_MAPPED_NAME_MAX_LENGTH, &mappedNameMaxLength);
    return (expectedValue == mappedNameMaxLength);
}

static void getVariableInfo(ShShaderInfo varType,
                            const ShHandle handle,
                            int index,
                            int* length,
                            int* size,
                            ShDataType* type,
                            char* name,
                            char* mappedName)
{
    if (!handle || !size || !type || !name)
        return;
    ASSERT((varType == SH_ACTIVE_ATTRIBUTES) ||
           (varType == SH_ACTIVE_UNIFORMS));

    TShHandleBase* base = reinterpret_cast<TShHandleBase*>(handle);
    TCompiler* compiler = base->getAsCompiler();
    if (compiler == 0)
        return;

    const TVariableInfoList& varList = varType == SH_ACTIVE_ATTRIBUTES ?
        compiler->getAttribs() : compiler->getUniforms();
    if (index < 0 || index >= static_cast<int>(varList.size()))
        return;

    const TVariableInfo& varInfo = varList[index];
    if (length) *length = varInfo.name.size();
    *size = varInfo.size;
    *type = varInfo.type;

    
    
    
    int activeUniformAndAttribLength = 1 + MAX_SYMBOL_NAME_LEN;
    ASSERT(checkActiveUniformAndAttribMaxLengths(handle, activeUniformAndAttribLength));
    strncpy(name, varInfo.name.c_str(), activeUniformAndAttribLength);
    name[activeUniformAndAttribLength - 1] = 0;
    if (mappedName) {
        
        
        int maxMappedNameLength = 1 + MAX_SYMBOL_NAME_LEN;
        ASSERT(checkMappedNameMaxLength(handle, maxMappedNameLength));
        strncpy(mappedName, varInfo.mappedName.c_str(), maxMappedNameLength);
        mappedName[maxMappedNameLength - 1] = 0;
    }
}





int ShInitialize()
{
    if (!InitProcess())
        return 0;

    return 1;
}




int ShFinalize()
{
    if (!DetachProcess())
        return 0;

    return 1;
}




void ShInitBuiltInResources(ShBuiltInResources* resources)
{
    
    resources->MaxVertexAttribs = 8;
    resources->MaxVertexUniformVectors = 128;
    resources->MaxVaryingVectors = 8;
    resources->MaxVertexTextureImageUnits = 0;
    resources->MaxCombinedTextureImageUnits = 8;
    resources->MaxTextureImageUnits = 8;
    resources->MaxFragmentUniformVectors = 16;
    resources->MaxDrawBuffers = 1;

    
    resources->OES_standard_derivatives = 0;
    resources->OES_EGL_image_external = 0;
    resources->ARB_texture_rectangle = 0;

    
    resources->HashFunction = NULL;
}




ShHandle ShConstructCompiler(ShShaderType type, ShShaderSpec spec,
                             ShShaderOutput output,
                             const ShBuiltInResources* resources)
{
    if (!InitThread())
        return 0;

    TShHandleBase* base = static_cast<TShHandleBase*>(ConstructCompiler(type, spec, output));
    TCompiler* compiler = base->getAsCompiler();
    if (compiler == 0)
        return 0;

    
    if (!compiler->Init(*resources)) {
        ShDestruct(base);
        return 0;
    }

    return reinterpret_cast<void*>(base);
}

void ShDestruct(ShHandle handle)
{
    if (handle == 0)
        return;

    TShHandleBase* base = static_cast<TShHandleBase*>(handle);

    if (base->getAsCompiler())
        DeleteCompiler(base->getAsCompiler());
}








int ShCompile(
    const ShHandle handle,
    const char* const shaderStrings[],
    const int numStrings,
    int compileOptions)
{
    if (!InitThread())
        return 0;

    if (handle == 0)
        return 0;

    TShHandleBase* base = reinterpret_cast<TShHandleBase*>(handle);
    TCompiler* compiler = base->getAsCompiler();
    if (compiler == 0)
        return 0;

    bool success = compiler->compile(shaderStrings, numStrings, compileOptions);
    return success ? 1 : 0;
}

void ShGetInfo(const ShHandle handle, ShShaderInfo pname, int* params)
{
    if (!handle || !params)
        return;

    TShHandleBase* base = static_cast<TShHandleBase*>(handle);
    TCompiler* compiler = base->getAsCompiler();
    if (!compiler) return;

    switch(pname)
    {
    case SH_INFO_LOG_LENGTH:
        *params = compiler->getInfoSink().info.size() + 1;
        break;
    case SH_OBJECT_CODE_LENGTH:
        *params = compiler->getInfoSink().obj.size() + 1;
        break;
    case SH_ACTIVE_UNIFORMS:
        *params = compiler->getUniforms().size();
        break;
    case SH_ACTIVE_UNIFORM_MAX_LENGTH:
        *params = 1 +  MAX_SYMBOL_NAME_LEN;
        break;
    case SH_ACTIVE_ATTRIBUTES:
        *params = compiler->getAttribs().size();
        break;
    case SH_ACTIVE_ATTRIBUTE_MAX_LENGTH:
        *params = 1 + MAX_SYMBOL_NAME_LEN;
        break;
    case SH_MAPPED_NAME_MAX_LENGTH:
        
        
        *params = 1 + MAX_SYMBOL_NAME_LEN;
        break;
    case SH_NAME_MAX_LENGTH:
        *params = 1 + MAX_SYMBOL_NAME_LEN;
        break;
    case SH_HASHED_NAME_MAX_LENGTH:
        if (compiler->getHashFunction() == NULL) {
            *params = 0;
        } else {
            
            
            const char HashedNamePrefix[] = HASHED_NAME_PREFIX;
            *params = 16 + sizeof(HashedNamePrefix);
        }
        break;
    case SH_HASHED_NAMES_COUNT:
        *params = compiler->getNameMap().size();
        break;
    default: UNREACHABLE();
    }
}




void ShGetInfoLog(const ShHandle handle, char* infoLog)
{
    if (!handle || !infoLog)
        return;

    TShHandleBase* base = static_cast<TShHandleBase*>(handle);
    TCompiler* compiler = base->getAsCompiler();
    if (!compiler) return;

    TInfoSink& infoSink = compiler->getInfoSink();
    strcpy(infoLog, infoSink.info.c_str());
}




void ShGetObjectCode(const ShHandle handle, char* objCode)
{
    if (!handle || !objCode)
        return;

    TShHandleBase* base = static_cast<TShHandleBase*>(handle);
    TCompiler* compiler = base->getAsCompiler();
    if (!compiler) return;

    TInfoSink& infoSink = compiler->getInfoSink();
    strcpy(objCode, infoSink.obj.c_str());
}

void ShGetActiveAttrib(const ShHandle handle,
                       int index,
                       int* length,
                       int* size,
                       ShDataType* type,
                       char* name,
                       char* mappedName)
{
    getVariableInfo(SH_ACTIVE_ATTRIBUTES,
                    handle, index, length, size, type, name, mappedName);
}

void ShGetActiveUniform(const ShHandle handle,
                        int index,
                        int* length,
                        int* size,
                        ShDataType* type,
                        char* name,
                        char* mappedName)
{
    getVariableInfo(SH_ACTIVE_UNIFORMS,
                    handle, index, length, size, type, name, mappedName);
}

void ShGetNameHashingEntry(const ShHandle handle,
                           int index,
                           char* name,
                           char* hashedName)
{
    if (!handle || !name || !hashedName || index < 0)
        return;

    TShHandleBase* base = static_cast<TShHandleBase*>(handle);
    TCompiler* compiler = base->getAsCompiler();
    if (!compiler) return;

    const NameMap& nameMap = compiler->getNameMap();
    if (index >= static_cast<int>(nameMap.size()))
        return;

    NameMap::const_iterator it = nameMap.begin();
    for (int i = 0; i < index; ++i)
        ++it;

    size_t len = it->first.length() + 1;
    int max_len = 0;
    ShGetInfo(handle, SH_NAME_MAX_LENGTH, &max_len);
    if (static_cast<int>(len) > max_len) {
        ASSERT(false);
        len = max_len;
    }
    strncpy(name, it->first.c_str(), len);
    
    name[len] = '\0';

    len = it->second.length() + 1;
    max_len = 0;
    ShGetInfo(handle, SH_HASHED_NAME_MAX_LENGTH, &max_len);
    if (static_cast<int>(len) > max_len) {
        ASSERT(false);
        len = max_len;
    }
    strncpy(hashedName, it->second.c_str(), len);
    
    hashedName[len] = '\0';
}
