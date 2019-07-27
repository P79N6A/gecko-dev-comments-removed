#include "precompiled.h"










#include "libGLESv2/Shader.h"

#include "GLSLANG/ShaderLang.h"
#include "common/utilities.h"
#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/Constants.h"
#include "libGLESv2/ResourceManager.h"

namespace gl
{
void *Shader::mFragmentCompiler = NULL;
void *Shader::mVertexCompiler = NULL;

Shader::Shader(ResourceManager *manager, const rx::Renderer *renderer, GLuint handle)
    : mHandle(handle), mRenderer(renderer), mResourceManager(manager)
{
    uncompile();
    initializeCompiler();

    mRefCount = 0;
    mDeleteStatus = false;
    mShaderVersion = 100;
}

Shader::~Shader()
{
}

GLuint Shader::getHandle() const
{
    return mHandle;
}

void Shader::setSource(GLsizei count, const char *const *string, const GLint *length)
{
    std::ostringstream stream;

    for (int i = 0; i < count; i++)
    {
        stream << string[i];
    }

    mSource = stream.str();
}

int Shader::getInfoLogLength() const
{
    return mInfoLog.empty() ? 0 : (mInfoLog.length() + 1);
}

void Shader::getInfoLog(GLsizei bufSize, GLsizei *length, char *infoLog) const
{
    int index = 0;

    if (bufSize > 0)
    {
        index = std::min(bufSize - 1, static_cast<GLsizei>(mInfoLog.length()));
        memcpy(infoLog, mInfoLog.c_str(), index);

        infoLog[index] = '\0';
    }

    if (length)
    {
        *length = index;
    }
}

int Shader::getSourceLength() const
{
    return mSource.empty() ? 0 : (mSource.length() + 1);
}

int Shader::getTranslatedSourceLength() const
{
    return mHlsl.empty() ? 0 : (mHlsl.length() + 1);
}

void Shader::getSourceImpl(const std::string &source, GLsizei bufSize, GLsizei *length, char *buffer) const
{
    int index = 0;

    if (bufSize > 0)
    {
        index = std::min(bufSize - 1, static_cast<GLsizei>(source.length()));
        memcpy(buffer, source.c_str(), index);

        buffer[index] = '\0';
    }

    if (length)
    {
        *length = index;
    }
}

void Shader::getSource(GLsizei bufSize, GLsizei *length, char *buffer) const
{
    getSourceImpl(mSource, bufSize, length, buffer);
}

void Shader::getTranslatedSource(GLsizei bufSize, GLsizei *length, char *buffer) const
{
    getSourceImpl(mHlsl, bufSize, length, buffer);
}

const std::vector<sh::Uniform> &Shader::getUniforms() const
{
    return mActiveUniforms;
}

const std::vector<sh::InterfaceBlock> &Shader::getInterfaceBlocks() const
{
    return mActiveInterfaceBlocks;
}

std::vector<PackedVarying> &Shader::getVaryings()
{
    return mVaryings;
}

bool Shader::isCompiled() const
{
    return !mHlsl.empty();
}

const std::string &Shader::getHLSL() const
{
    return mHlsl;
}

void Shader::addRef()
{
    mRefCount++;
}

void Shader::release()
{
    mRefCount--;

    if (mRefCount == 0 && mDeleteStatus)
    {
        mResourceManager->deleteShader(mHandle);
    }
}

unsigned int Shader::getRefCount() const
{
    return mRefCount;
}

bool Shader::isFlaggedForDeletion() const
{
    return mDeleteStatus;
}

void Shader::flagForDeletion()
{
    mDeleteStatus = true;
}


void Shader::initializeCompiler()
{
    if (!mFragmentCompiler)
    {
        int result = ShInitialize();

        if (result)
        {
            ShShaderOutput hlslVersion = (mRenderer->getMajorShaderModel() >= 4) ? SH_HLSL11_OUTPUT : SH_HLSL9_OUTPUT;

            ShBuiltInResources resources;
            ShInitBuiltInResources(&resources);

            const gl::Caps &caps = mRenderer->getCaps();

            resources.MaxVertexAttribs = MAX_VERTEX_ATTRIBS;
            resources.MaxVertexUniformVectors = mRenderer->getMaxVertexUniformVectors();
            resources.MaxVaryingVectors = mRenderer->getMaxVaryingVectors();
            resources.MaxVertexTextureImageUnits = mRenderer->getMaxVertexTextureImageUnits();
            resources.MaxCombinedTextureImageUnits = mRenderer->getMaxCombinedTextureImageUnits();
            resources.MaxTextureImageUnits = MAX_TEXTURE_IMAGE_UNITS;
            resources.MaxFragmentUniformVectors = mRenderer->getMaxFragmentUniformVectors();
            resources.MaxDrawBuffers = mRenderer->getMaxRenderTargets();
            resources.OES_standard_derivatives = caps.extensions.standardDerivatives;
            resources.EXT_draw_buffers = caps.extensions.drawBuffers;
            resources.EXT_shader_texture_lod = 1;
            
            resources.FragmentPrecisionHigh = 1;   
            resources.EXT_frag_depth = 1; 
            
            resources.MaxVertexOutputVectors = mRenderer->getMaxVaryingVectors();
            resources.MaxFragmentInputVectors = mRenderer->getMaxVaryingVectors();
            resources.MinProgramTexelOffset = -8;   
            resources.MaxProgramTexelOffset = 7;    

            mFragmentCompiler = ShConstructCompiler(SH_FRAGMENT_SHADER, SH_GLES2_SPEC, hlslVersion, &resources);
            mVertexCompiler = ShConstructCompiler(SH_VERTEX_SHADER, SH_GLES2_SPEC, hlslVersion, &resources);
        }
    }
}

void Shader::releaseCompiler()
{
    ShDestruct(mFragmentCompiler);
    ShDestruct(mVertexCompiler);

    mFragmentCompiler = NULL;
    mVertexCompiler = NULL;

    ShFinalize();
}

void Shader::parseVaryings(void *compiler)
{
    if (!mHlsl.empty())
    {
        std::vector<sh::Varying> *activeVaryings;
        ShGetInfoPointer(compiler, SH_ACTIVE_VARYINGS_ARRAY, reinterpret_cast<void**>(&activeVaryings));

        for (size_t varyingIndex = 0; varyingIndex < activeVaryings->size(); varyingIndex++)
        {
            mVaryings.push_back(PackedVarying((*activeVaryings)[varyingIndex]));
        }

        mUsesMultipleRenderTargets = mHlsl.find("GL_USES_MRT")          != std::string::npos;
        mUsesFragColor             = mHlsl.find("GL_USES_FRAG_COLOR")   != std::string::npos;
        mUsesFragData              = mHlsl.find("GL_USES_FRAG_DATA")    != std::string::npos;
        mUsesFragCoord             = mHlsl.find("GL_USES_FRAG_COORD")   != std::string::npos;
        mUsesFrontFacing           = mHlsl.find("GL_USES_FRONT_FACING") != std::string::npos;
        mUsesPointSize             = mHlsl.find("GL_USES_POINT_SIZE")   != std::string::npos;
        mUsesPointCoord            = mHlsl.find("GL_USES_POINT_COORD")  != std::string::npos;
        mUsesDepthRange            = mHlsl.find("GL_USES_DEPTH_RANGE")  != std::string::npos;
        mUsesFragDepth             = mHlsl.find("GL_USES_FRAG_DEPTH")   != std::string::npos;
        mUsesDiscardRewriting      = mHlsl.find("ANGLE_USES_DISCARD_REWRITING") != std::string::npos;
        mUsesNestedBreak           = mHlsl.find("ANGLE_USES_NESTED_BREAK") != std::string::npos;
    }
}

void Shader::resetVaryingsRegisterAssignment()
{
    for (unsigned int varyingIndex = 0; varyingIndex < mVaryings.size(); varyingIndex++)
    {
        mVaryings[varyingIndex].resetRegisterAssignment();
    }
}


void Shader::uncompile()
{
    
    mHlsl.clear();
    mInfoLog.clear();

    
    mVaryings.clear();

    mUsesMultipleRenderTargets = false;
    mUsesFragColor = false;
    mUsesFragData = false;
    mUsesFragCoord = false;
    mUsesFrontFacing = false;
    mUsesPointSize = false;
    mUsesPointCoord = false;
    mUsesDepthRange = false;
    mUsesFragDepth = false;
    mShaderVersion = 100;
    mUsesDiscardRewriting = false;
    mUsesNestedBreak = false;

    mActiveUniforms.clear();
    mActiveInterfaceBlocks.clear();
}

void Shader::compileToHLSL(void *compiler)
{
    
    initializeCompiler();

    int compileOptions = SH_OBJECT_CODE;
    std::string sourcePath;
    if (perfActive())
    {
        sourcePath = getTempPath();
        writeFile(sourcePath.c_str(), mSource.c_str(), mSource.length());
        compileOptions |= SH_LINE_DIRECTIVES;
    }

    int result;
    if (sourcePath.empty())
    {
        const char* sourceStrings[] =
        {
            mSource.c_str(),
        };

        result = ShCompile(compiler, sourceStrings, ArraySize(sourceStrings), compileOptions);
    }
    else
    {
        const char* sourceStrings[] =
        {
            sourcePath.c_str(),
            mSource.c_str(),
        };

        result = ShCompile(compiler, sourceStrings, ArraySize(sourceStrings), compileOptions | SH_SOURCE_PATH);
    }

    size_t shaderVersion = 100;
    ShGetInfo(compiler, SH_SHADER_VERSION, &shaderVersion);

    mShaderVersion = static_cast<int>(shaderVersion);

    if (shaderVersion == 300 && mRenderer->getCurrentClientVersion() < 3)
    {
        mInfoLog = "GLSL ES 3.00 is not supported by OpenGL ES 2.0 contexts";
        TRACE("\n%s", mInfoLog.c_str());
    }
    else if (result)
    {
        size_t objCodeLen = 0;
        ShGetInfo(compiler, SH_OBJECT_CODE_LENGTH, &objCodeLen);

        char* outputHLSL = new char[objCodeLen];
        ShGetObjectCode(compiler, outputHLSL);

#ifdef _DEBUG
        std::ostringstream hlslStream;
        hlslStream << "// GLSL\n";
        hlslStream << "//\n";

        size_t curPos = 0;
        while (curPos != std::string::npos)
        {
            size_t nextLine = mSource.find("\n", curPos);
            size_t len = (nextLine == std::string::npos) ? std::string::npos : (nextLine - curPos + 1);

            hlslStream << "// " << mSource.substr(curPos, len);

            curPos = (nextLine == std::string::npos) ? std::string::npos : (nextLine + 1);
        }
        hlslStream << "\n\n";
        hlslStream << outputHLSL;
        mHlsl = hlslStream.str();
#else
        mHlsl = outputHLSL;
#endif

        delete[] outputHLSL;

        void *activeUniforms;
        ShGetInfoPointer(compiler, SH_ACTIVE_UNIFORMS_ARRAY, &activeUniforms);
        mActiveUniforms = *(std::vector<sh::Uniform>*)activeUniforms;

        void *activeInterfaceBlocks;
        ShGetInfoPointer(compiler, SH_ACTIVE_INTERFACE_BLOCKS_ARRAY, &activeInterfaceBlocks);
        mActiveInterfaceBlocks = *(std::vector<sh::InterfaceBlock>*)activeInterfaceBlocks;
    }
    else
    {
        size_t infoLogLen = 0;
        ShGetInfo(compiler, SH_INFO_LOG_LENGTH, &infoLogLen);

        char* infoLog = new char[infoLogLen];
        ShGetInfoLog(compiler, infoLog);
        mInfoLog = infoLog;

        TRACE("\n%s", mInfoLog.c_str());
    }
}

rx::D3DWorkaroundType Shader::getD3DWorkarounds() const
{
    if (mUsesDiscardRewriting)
    {
        
        
        return rx::ANGLE_D3D_WORKAROUND_SKIP_OPTIMIZATION;
    }

    if (mUsesNestedBreak)
    {
        
        
        
        return rx::ANGLE_D3D_WORKAROUND_MAX_OPTIMIZATION;
    }

    return rx::ANGLE_D3D_WORKAROUND_NONE;
}



static const GLenum varyingPriorityList[] =
{
    
    GL_FLOAT_MAT4,

    
    
    GL_FLOAT_MAT3x4,
    GL_FLOAT_MAT4x3,
    GL_FLOAT_MAT2x4,
    GL_FLOAT_MAT4x2,

    
    GL_FLOAT_MAT2,

    
    GL_FLOAT_VEC4,
    GL_INT_VEC4,
    GL_UNSIGNED_INT_VEC4,

    
    GL_FLOAT_MAT3,
    GL_FLOAT_MAT2x3,
    GL_FLOAT_MAT3x2,

    
    GL_FLOAT_VEC3,
    GL_INT_VEC3,
    GL_UNSIGNED_INT_VEC3,

    
    GL_FLOAT_VEC2,
    GL_INT_VEC2,
    GL_UNSIGNED_INT_VEC2,

    
    GL_FLOAT,
    GL_INT,
    GL_UNSIGNED_INT,
};


bool Shader::compareVarying(const PackedVarying &x, const PackedVarying &y)
{
    if (x.type == y.type)
    {
        return x.arraySize > y.arraySize;
    }

    
    if (x.type == GL_STRUCT_ANGLEX)
    {
        return false;
    }

    unsigned int xPriority = GL_INVALID_INDEX;
    unsigned int yPriority = GL_INVALID_INDEX;

    for (unsigned int priorityIndex = 0; priorityIndex < ArraySize(varyingPriorityList); priorityIndex++)
    {
        if (varyingPriorityList[priorityIndex] == x.type) xPriority = priorityIndex;
        if (varyingPriorityList[priorityIndex] == y.type) yPriority = priorityIndex;
        if (xPriority != GL_INVALID_INDEX && yPriority != GL_INVALID_INDEX) break;
    }

    ASSERT(xPriority != GL_INVALID_INDEX && yPriority != GL_INVALID_INDEX);

    return xPriority <= yPriority;
}

int Shader::getShaderVersion() const
{
    return mShaderVersion;
}

VertexShader::VertexShader(ResourceManager *manager, const rx::Renderer *renderer, GLuint handle)
    : Shader(manager, renderer, handle)
{
}

VertexShader::~VertexShader()
{
}

GLenum VertexShader::getType()
{
    return GL_VERTEX_SHADER;
}

void VertexShader::uncompile()
{
    Shader::uncompile();

    
    mActiveAttributes.clear();
}

void VertexShader::compile()
{
    uncompile();

    compileToHLSL(mVertexCompiler);
    parseAttributes();
    parseVaryings(mVertexCompiler);
}

int VertexShader::getSemanticIndex(const std::string &attributeName)
{
    if (!attributeName.empty())
    {
        int semanticIndex = 0;
        for (unsigned int attributeIndex = 0; attributeIndex < mActiveAttributes.size(); attributeIndex++)
        {
            const sh::ShaderVariable &attribute = mActiveAttributes[attributeIndex];

            if (attribute.name == attributeName)
            {
                return semanticIndex;
            }

            semanticIndex += VariableRegisterCount(attribute.type);
        }
    }

    return -1;
}

void VertexShader::parseAttributes()
{
    const std::string &hlsl = getHLSL();
    if (!hlsl.empty())
    {
        void *activeAttributes;
        ShGetInfoPointer(mVertexCompiler, SH_ACTIVE_ATTRIBUTES_ARRAY, &activeAttributes);
        mActiveAttributes = *(std::vector<sh::Attribute>*)activeAttributes;
    }
}

FragmentShader::FragmentShader(ResourceManager *manager, const rx::Renderer *renderer, GLuint handle)
    : Shader(manager, renderer, handle)
{
}

FragmentShader::~FragmentShader()
{
}

GLenum FragmentShader::getType()
{
    return GL_FRAGMENT_SHADER;
}

void FragmentShader::compile()
{
    uncompile();

    compileToHLSL(mFragmentCompiler);
    parseVaryings(mFragmentCompiler);
    std::sort(mVaryings.begin(), mVaryings.end(), compareVarying);

    const std::string &hlsl = getHLSL();
    if (!hlsl.empty())
    {
        void *activeOutputVariables;
        ShGetInfoPointer(mFragmentCompiler, SH_ACTIVE_OUTPUT_VARIABLES_ARRAY, &activeOutputVariables);
        mActiveOutputVariables = *(std::vector<sh::Attribute>*)activeOutputVariables;
    }
}

void FragmentShader::uncompile()
{
    Shader::uncompile();

    mActiveOutputVariables.clear();
}

const std::vector<sh::Attribute> &FragmentShader::getOutputVariables() const
{
    return mActiveOutputVariables;
}

ShShaderOutput Shader::getCompilerOutputType(GLenum shader)
{
    void *compiler = NULL;

    switch (shader)
    {
      case GL_VERTEX_SHADER:   compiler = mVertexCompiler;   break;
      case GL_FRAGMENT_SHADER: compiler = mFragmentCompiler; break;
      default: UNREACHABLE();  return SH_HLSL9_OUTPUT;
    }

    size_t outputType = 0;
    ShGetInfo(compiler, SH_OUTPUT_TYPE, &outputType);

    return static_cast<ShShaderOutput>(outputType);
}

}
