







#include "libGLESv2/renderer/d3d/ShaderD3D.h"
#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/Shader.h"
#include "libGLESv2/main.h"

#include "common/utilities.h"

namespace rx
{

void *ShaderD3D::mFragmentCompiler = NULL;
void *ShaderD3D::mVertexCompiler = NULL;

template <typename VarT>
const std::vector<VarT> *GetShaderVariables(const std::vector<VarT> *variableList)
{
    
    ASSERT(variableList);
    return variableList;
}

ShaderD3D::ShaderD3D(rx::Renderer *renderer)
    : ShaderImpl(),
      mRenderer(renderer),
      mShaderVersion(100)
{
    uncompile();
    initializeCompiler();
}

ShaderD3D::~ShaderD3D()
{
}

ShaderD3D *ShaderD3D::makeShaderD3D(ShaderImpl *impl)
{
    ASSERT(HAS_DYNAMIC_TYPE(ShaderD3D*, impl));
    return static_cast<ShaderD3D*>(impl);
}

const ShaderD3D *ShaderD3D::makeShaderD3D(const ShaderImpl *impl)
{
    ASSERT(HAS_DYNAMIC_TYPE(const ShaderD3D*, impl));
    return static_cast<const ShaderD3D*>(impl);
}


void ShaderD3D::initializeCompiler()
{
    if (!mFragmentCompiler)
    {
        int result = ShInitialize();

        if (result)
        {
            ShShaderOutput hlslVersion = (mRenderer->getMajorShaderModel() >= 4) ? SH_HLSL11_OUTPUT : SH_HLSL9_OUTPUT;

            ShBuiltInResources resources;
            ShInitBuiltInResources(&resources);

            
            const gl::Caps &caps = mRenderer->getRendererCaps();
            const gl::Extensions &extensions = mRenderer->getRendererExtensions();

            resources.MaxVertexAttribs = caps.maxVertexAttributes;
            resources.MaxVertexUniformVectors = caps.maxVertexUniformVectors;
            resources.MaxVaryingVectors = caps.maxVaryingVectors;
            resources.MaxVertexTextureImageUnits = caps.maxVertexTextureImageUnits;
            resources.MaxCombinedTextureImageUnits = caps.maxCombinedTextureImageUnits;
            resources.MaxTextureImageUnits = caps.maxTextureImageUnits;
            resources.MaxFragmentUniformVectors = caps.maxFragmentUniformVectors;
            resources.MaxDrawBuffers = caps.maxDrawBuffers;
            resources.OES_standard_derivatives = extensions.standardDerivatives;
            resources.EXT_draw_buffers = extensions.drawBuffers;
            resources.EXT_shader_texture_lod = 1;
            
            resources.FragmentPrecisionHigh = 1;   
            resources.EXT_frag_depth = 1; 
            
            resources.MaxVertexOutputVectors = caps.maxVertexOutputComponents / 4;
            resources.MaxFragmentInputVectors = caps.maxFragmentInputComponents / 4;
            resources.MinProgramTexelOffset = caps.minProgramTexelOffset;
            resources.MaxProgramTexelOffset = caps.maxProgramTexelOffset;

            mFragmentCompiler = ShConstructCompiler(GL_FRAGMENT_SHADER, SH_GLES2_SPEC, hlslVersion, &resources);
            mVertexCompiler = ShConstructCompiler(GL_VERTEX_SHADER, SH_GLES2_SPEC, hlslVersion, &resources);
        }
    }
}

void ShaderD3D::releaseCompiler()
{
    ShDestruct(mFragmentCompiler);
    ShDestruct(mVertexCompiler);

    mFragmentCompiler = NULL;
    mVertexCompiler = NULL;

    ShFinalize();
}

void ShaderD3D::parseVaryings(void *compiler)
{
     if (!mHlsl.empty())
    {
        const std::vector<sh::Varying> *activeVaryings = ShGetVaryings(compiler);
        ASSERT(activeVaryings);

        for (size_t varyingIndex = 0; varyingIndex < activeVaryings->size(); varyingIndex++)
        {
            mVaryings.push_back(gl::PackedVarying((*activeVaryings)[varyingIndex]));
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

void ShaderD3D::resetVaryingsRegisterAssignment()
{
    for (unsigned int varyingIndex = 0; varyingIndex < mVaryings.size(); varyingIndex++)
    {
        mVaryings[varyingIndex].resetRegisterAssignment();
    }
}


void ShaderD3D::uncompile()
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

void ShaderD3D::compileToHLSL(void *compiler, const std::string &source)
{
    
    initializeCompiler();

    int compileOptions = SH_OBJECT_CODE;
    std::string sourcePath;
    if (gl::perfActive())
    {
        sourcePath = getTempPath();
        writeFile(sourcePath.c_str(), source.c_str(), source.length());
        compileOptions |= SH_LINE_DIRECTIVES;
    }

    int result;
    if (sourcePath.empty())
    {
        const char* sourceStrings[] =
        {
            source.c_str(),
        };

        result = ShCompile(compiler, sourceStrings, ArraySize(sourceStrings), compileOptions);
    }
    else
    {
        const char* sourceStrings[] =
        {
            sourcePath.c_str(),
            source.c_str(),
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
            size_t nextLine = source.find("\n", curPos);
            size_t len = (nextLine == std::string::npos) ? std::string::npos : (nextLine - curPos + 1);

            hlslStream << "// " << source.substr(curPos, len);

            curPos = (nextLine == std::string::npos) ? std::string::npos : (nextLine + 1);
        }
        hlslStream << "\n\n";
        hlslStream << outputHLSL;
        mHlsl = hlslStream.str();
#else
        mHlsl = outputHLSL;
#endif

        SafeDeleteArray(outputHLSL);

        mActiveUniforms = *GetShaderVariables(ShGetUniforms(compiler));

        for (size_t uniformIndex = 0; uniformIndex < mActiveUniforms.size(); uniformIndex++)
        {
            const sh::Uniform &uniform = mActiveUniforms[uniformIndex];

            unsigned int index = -1;
            bool result = ShGetUniformRegister(compiler, uniform.name.c_str(), &index);
            UNUSED_ASSERTION_VARIABLE(result);
            ASSERT(result);

            mUniformRegisterMap[uniform.name] = index;
        }

        mActiveInterfaceBlocks = *GetShaderVariables(ShGetInterfaceBlocks(compiler));

        for (size_t blockIndex = 0; blockIndex < mActiveInterfaceBlocks.size(); blockIndex++)
        {
            const sh::InterfaceBlock &interfaceBlock = mActiveInterfaceBlocks[blockIndex];

            unsigned int index = -1;
            bool result = ShGetInterfaceBlockRegister(compiler, interfaceBlock.name.c_str(), &index);
            UNUSED_ASSERTION_VARIABLE(result);
            ASSERT(result);

            mInterfaceBlockRegisterMap[interfaceBlock.name] = index;
        }
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

rx::D3DWorkaroundType ShaderD3D::getD3DWorkarounds() const
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


bool ShaderD3D::compareVarying(const gl::PackedVarying &x, const gl::PackedVarying &y)
{
    if (x.type == y.type)
    {
        return x.arraySize > y.arraySize;
    }

    
    if (x.type == GL_STRUCT_ANGLEX)
    {
        return false;
    }

    if (y.type == GL_STRUCT_ANGLEX)
    {
        return true;
    }

    return gl::VariableSortOrder(x.type) <= gl::VariableSortOrder(y.type);
}

unsigned int ShaderD3D::getUniformRegister(const std::string &uniformName) const
{
    ASSERT(mUniformRegisterMap.count(uniformName) > 0);
    return mUniformRegisterMap.find(uniformName)->second;
}

unsigned int ShaderD3D::getInterfaceBlockRegister(const std::string &blockName) const
{
    ASSERT(mInterfaceBlockRegisterMap.count(blockName) > 0);
    return mInterfaceBlockRegisterMap.find(blockName)->second;
}

ShShaderOutput ShaderD3D::getCompilerOutputType(GLenum shader)
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

VertexShaderD3D::VertexShaderD3D(rx::Renderer *renderer) : ShaderD3D(renderer)
{
}

VertexShaderD3D::~VertexShaderD3D()
{
}

VertexShaderD3D *VertexShaderD3D::makeVertexShaderD3D(ShaderImpl *impl)
{
    ASSERT(HAS_DYNAMIC_TYPE(VertexShaderD3D*, impl));
    return static_cast<VertexShaderD3D*>(impl);
}

const VertexShaderD3D *VertexShaderD3D::makeVertexShaderD3D(const ShaderImpl *impl)
{
    ASSERT(HAS_DYNAMIC_TYPE(const VertexShaderD3D*, impl));
    return static_cast<const VertexShaderD3D*>(impl);
}

bool VertexShaderD3D::compile(const std::string &source)
{
    uncompile();

    compileToHLSL(mVertexCompiler, source);
    parseAttributes();
    parseVaryings(mVertexCompiler);

    return !getTranslatedSource().empty();
}

void VertexShaderD3D::uncompile()
{
    ShaderD3D::uncompile();

    
    mActiveAttributes.clear();
}

void VertexShaderD3D::parseAttributes()
{
    const std::string &hlsl = getTranslatedSource();
    if (!hlsl.empty())
    {
        mActiveAttributes = *GetShaderVariables(ShGetAttributes(mVertexCompiler));
    }
}

int VertexShaderD3D::getSemanticIndex(const std::string &attributeName)
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

            semanticIndex += gl::VariableRegisterCount(attribute.type);
        }
    }

    return -1;
}

FragmentShaderD3D::FragmentShaderD3D(rx::Renderer *renderer) : ShaderD3D(renderer)
{
}

FragmentShaderD3D::~FragmentShaderD3D()
{
}

FragmentShaderD3D *FragmentShaderD3D::makeFragmentShaderD3D(ShaderImpl *impl)
{
    ASSERT(HAS_DYNAMIC_TYPE(FragmentShaderD3D*, impl));
    return static_cast<FragmentShaderD3D*>(impl);
}

const FragmentShaderD3D *FragmentShaderD3D::makeFragmentShaderD3D(const ShaderImpl *impl)
{
    ASSERT(HAS_DYNAMIC_TYPE(const FragmentShaderD3D*, impl));
    return static_cast<const FragmentShaderD3D*>(impl);
}

bool FragmentShaderD3D::compile(const std::string &source)
{
    uncompile();

    compileToHLSL(mFragmentCompiler, source);
    parseVaryings(mFragmentCompiler);
    std::sort(mVaryings.begin(), mVaryings.end(), compareVarying);

    const std::string &hlsl = getTranslatedSource();
    if (!hlsl.empty())
    {
        mActiveOutputVariables = *GetShaderVariables(ShGetOutputVariables(mFragmentCompiler));
        return true;
    }
    return false;
}

void FragmentShaderD3D::uncompile()
{
    ShaderD3D::uncompile();

    mActiveOutputVariables.clear();
}

}
