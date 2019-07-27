








#include "libGLESv2/BinaryStream.h"
#include "libGLESv2/ProgramBinary.h"
#include "libGLESv2/Framebuffer.h"
#include "libGLESv2/Renderbuffer.h"
#include "libGLESv2/renderer/ShaderExecutable.h"

#include "common/debug.h"
#include "common/version.h"
#include "common/utilities.h"
#include "common/platform.h"

#include "libGLESv2/main.h"
#include "libGLESv2/Shader.h"
#include "libGLESv2/Program.h"
#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/renderer/d3d/DynamicHLSL.h"
#include "libGLESv2/renderer/d3d/ShaderD3D.h"
#include "libGLESv2/renderer/d3d/VertexDataManager.h"
#include "libGLESv2/Context.h"
#include "libGLESv2/Buffer.h"
#include "common/blocklayout.h"

namespace gl
{

namespace
{

TextureType GetTextureType(GLenum samplerType)
{
    switch (samplerType)
    {
      case GL_SAMPLER_2D:
      case GL_INT_SAMPLER_2D:
      case GL_UNSIGNED_INT_SAMPLER_2D:
      case GL_SAMPLER_2D_SHADOW:
        return TEXTURE_2D;
      case GL_SAMPLER_3D:
      case GL_INT_SAMPLER_3D:
      case GL_UNSIGNED_INT_SAMPLER_3D:
        return TEXTURE_3D;
      case GL_SAMPLER_CUBE:
      case GL_SAMPLER_CUBE_SHADOW:
        return TEXTURE_CUBE;
      case GL_INT_SAMPLER_CUBE:
      case GL_UNSIGNED_INT_SAMPLER_CUBE:
        return TEXTURE_CUBE;
      case GL_SAMPLER_2D_ARRAY:
      case GL_INT_SAMPLER_2D_ARRAY:
      case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
      case GL_SAMPLER_2D_ARRAY_SHADOW:
        return TEXTURE_2D_ARRAY;
      default: UNREACHABLE();
    }

    return TEXTURE_2D;
}

unsigned int ParseAndStripArrayIndex(std::string* name)
{
    unsigned int subscript = GL_INVALID_INDEX;

    
    size_t open = name->find_last_of('[');
    size_t close = name->find_last_of(']');
    if (open != std::string::npos && close == name->length() - 1)
    {
        subscript = atoi(name->substr(open + 1).c_str());
        name->erase(open);
    }

    return subscript;
}

void GetInputLayoutFromShader(const std::vector<sh::Attribute> &shaderAttributes, VertexFormat inputLayout[MAX_VERTEX_ATTRIBS])
{
    size_t layoutIndex = 0;
    for (size_t attributeIndex = 0; attributeIndex < shaderAttributes.size(); attributeIndex++)
    {
        ASSERT(layoutIndex < MAX_VERTEX_ATTRIBS);

        const sh::Attribute &shaderAttr = shaderAttributes[attributeIndex];

        if (shaderAttr.type != GL_NONE)
        {
            GLenum transposedType = TransposeMatrixType(shaderAttr.type);

            for (size_t rowIndex = 0; static_cast<int>(rowIndex) < VariableRowCount(transposedType); rowIndex++, layoutIndex++)
            {
                VertexFormat *defaultFormat = &inputLayout[layoutIndex];

                defaultFormat->mType = VariableComponentType(transposedType);
                defaultFormat->mNormalized = false;
                defaultFormat->mPureInteger = (defaultFormat->mType != GL_FLOAT); 
                defaultFormat->mComponents = VariableColumnCount(transposedType);
            }
        }
    }
}

bool IsRowMajorLayout(const sh::InterfaceBlockField &var)
{
    return var.isRowMajorLayout;
}

bool IsRowMajorLayout(const sh::ShaderVariable &var)
{
    return false;
}

}

VariableLocation::VariableLocation(const std::string &name, unsigned int element, unsigned int index)
    : name(name), element(element), index(index)
{
}

ProgramBinary::VertexExecutable::VertexExecutable(const VertexFormat inputLayout[],
                                                  const GLenum signature[],
                                                  rx::ShaderExecutable *shaderExecutable)
    : mShaderExecutable(shaderExecutable)
{
    for (size_t attributeIndex = 0; attributeIndex < gl::MAX_VERTEX_ATTRIBS; attributeIndex++)
    {
        mInputs[attributeIndex] = inputLayout[attributeIndex];
        mSignature[attributeIndex] = signature[attributeIndex];
    }
}

ProgramBinary::VertexExecutable::~VertexExecutable()
{
    SafeDelete(mShaderExecutable);
}

bool ProgramBinary::VertexExecutable::matchesSignature(const GLenum signature[]) const
{
    for (size_t attributeIndex = 0; attributeIndex < MAX_VERTEX_ATTRIBS; attributeIndex++)
    {
        if (mSignature[attributeIndex] != signature[attributeIndex])
        {
            return false;
        }
    }

    return true;
}

ProgramBinary::PixelExecutable::PixelExecutable(const std::vector<GLenum> &outputSignature, rx::ShaderExecutable *shaderExecutable)
    : mOutputSignature(outputSignature),
      mShaderExecutable(shaderExecutable)
{
}

ProgramBinary::PixelExecutable::~PixelExecutable()
{
    SafeDelete(mShaderExecutable);
}

LinkedVarying::LinkedVarying()
{
}

LinkedVarying::LinkedVarying(const std::string &name, GLenum type, GLsizei size, const std::string &semanticName,
                             unsigned int semanticIndex, unsigned int semanticIndexCount)
    : name(name), type(type), size(size), semanticName(semanticName), semanticIndex(semanticIndex), semanticIndexCount(semanticIndexCount)
{
}

unsigned int ProgramBinary::mCurrentSerial = 1;

ProgramBinary::ProgramBinary(rx::Renderer *renderer)
    : RefCountObject(0),
      mRenderer(renderer),
      mDynamicHLSL(NULL),
      mVertexWorkarounds(rx::ANGLE_D3D_WORKAROUND_NONE),
      mPixelWorkarounds(rx::ANGLE_D3D_WORKAROUND_NONE),
      mGeometryExecutable(NULL),
      mUsedVertexSamplerRange(0),
      mUsedPixelSamplerRange(0),
      mUsesPointSize(false),
      mShaderVersion(100),
      mDirtySamplerMapping(true),
      mVertexUniformStorage(NULL),
      mFragmentUniformStorage(NULL),
      mValidated(false),
      mSerial(issueSerial())
{
    for (int index = 0; index < MAX_VERTEX_ATTRIBS; index++)
    {
        mSemanticIndex[index] = -1;
    }

    for (int index = 0; index < MAX_TEXTURE_IMAGE_UNITS; index++)
    {
        mSamplersPS[index].active = false;
    }

    for (int index = 0; index < IMPLEMENTATION_MAX_VERTEX_TEXTURE_IMAGE_UNITS; index++)
    {
        mSamplersVS[index].active = false;
    }

    mDynamicHLSL = new rx::DynamicHLSL(renderer);
}

ProgramBinary::~ProgramBinary()
{
    reset();
    SafeDelete(mDynamicHLSL);
}

unsigned int ProgramBinary::getSerial() const
{
    return mSerial;
}

int ProgramBinary::getShaderVersion() const
{
    return mShaderVersion;
}

unsigned int ProgramBinary::issueSerial()
{
    return mCurrentSerial++;
}

rx::ShaderExecutable *ProgramBinary::getPixelExecutableForFramebuffer(const Framebuffer *fbo)
{
    std::vector<GLenum> outputs(IMPLEMENTATION_MAX_DRAW_BUFFERS);
    for (size_t outputIndex = 0; outputIndex < IMPLEMENTATION_MAX_DRAW_BUFFERS; outputIndex++)
    {
        if (fbo->getColorbuffer(outputIndex) != NULL)
        {
            
            outputs[outputIndex] = GL_FLOAT;
        }
        else
        {
            outputs[outputIndex] = GL_NONE;
        }
    }

    return getPixelExecutableForOutputLayout(outputs);
}

rx::ShaderExecutable *ProgramBinary::getPixelExecutableForOutputLayout(const std::vector<GLenum> &outputSignature)
{
    for (size_t executableIndex = 0; executableIndex < mPixelExecutables.size(); executableIndex++)
    {
        if (mPixelExecutables[executableIndex]->matchesSignature(outputSignature))
        {
            return mPixelExecutables[executableIndex]->shaderExecutable();
        }
    }

    std::string finalPixelHLSL = mDynamicHLSL->generatePixelShaderForOutputSignature(mPixelHLSL, mPixelShaderKey, mUsesFragDepth,
                                                                                     outputSignature);

    
    InfoLog tempInfoLog;
    rx::ShaderExecutable *pixelExecutable = mRenderer->compileToExecutable(tempInfoLog, finalPixelHLSL.c_str(), rx::SHADER_PIXEL,
                                                                           mTransformFeedbackLinkedVaryings,
                                                                           (mTransformFeedbackBufferMode == GL_SEPARATE_ATTRIBS),
                                                                           mPixelWorkarounds);

    if (!pixelExecutable)
    {
        std::vector<char> tempCharBuffer(tempInfoLog.getLength() + 3);
        tempInfoLog.getLog(tempInfoLog.getLength(), NULL, &tempCharBuffer[0]);
        ERR("Error compiling dynamic pixel executable:\n%s\n", &tempCharBuffer[0]);
    }
    else
    {
        mPixelExecutables.push_back(new PixelExecutable(outputSignature, pixelExecutable));
    }

    return pixelExecutable;
}

rx::ShaderExecutable *ProgramBinary::getVertexExecutableForInputLayout(const VertexFormat inputLayout[MAX_VERTEX_ATTRIBS])
{
    GLenum signature[MAX_VERTEX_ATTRIBS];
    mDynamicHLSL->getInputLayoutSignature(inputLayout, signature);

    for (size_t executableIndex = 0; executableIndex < mVertexExecutables.size(); executableIndex++)
    {
        if (mVertexExecutables[executableIndex]->matchesSignature(signature))
        {
            return mVertexExecutables[executableIndex]->shaderExecutable();
        }
    }

    
    std::string finalVertexHLSL = mDynamicHLSL->generateVertexShaderForInputLayout(mVertexHLSL, inputLayout, mShaderAttributes);

    
    InfoLog tempInfoLog;
    rx::ShaderExecutable *vertexExecutable = mRenderer->compileToExecutable(tempInfoLog, finalVertexHLSL.c_str(),
                                                                            rx::SHADER_VERTEX,
                                                                            mTransformFeedbackLinkedVaryings,
                                                                            (mTransformFeedbackBufferMode == GL_SEPARATE_ATTRIBS),
                                                                            mVertexWorkarounds);

    if (!vertexExecutable)
    {
        std::vector<char> tempCharBuffer(tempInfoLog.getLength()+3);
        tempInfoLog.getLog(tempInfoLog.getLength(), NULL, &tempCharBuffer[0]);
        ERR("Error compiling dynamic vertex executable:\n%s\n", &tempCharBuffer[0]);
    }
    else
    {
        mVertexExecutables.push_back(new VertexExecutable(inputLayout, signature, vertexExecutable));
    }

    return vertexExecutable;
}

rx::ShaderExecutable *ProgramBinary::getGeometryExecutable() const
{
    return mGeometryExecutable;
}

GLuint ProgramBinary::getAttributeLocation(const char *name)
{
    if (name)
    {
        for (int index = 0; index < MAX_VERTEX_ATTRIBS; index++)
        {
            if (mLinkedAttribute[index].name == std::string(name))
            {
                return index;
            }
        }
    }

    return -1;
}

int ProgramBinary::getSemanticIndex(int attributeIndex)
{
    ASSERT(attributeIndex >= 0 && attributeIndex < MAX_VERTEX_ATTRIBS);

    return mSemanticIndex[attributeIndex];
}


GLint ProgramBinary::getUsedSamplerRange(SamplerType type)
{
    switch (type)
    {
      case SAMPLER_PIXEL:
        return mUsedPixelSamplerRange;
      case SAMPLER_VERTEX:
        return mUsedVertexSamplerRange;
      default:
        UNREACHABLE();
        return 0;
    }
}

bool ProgramBinary::usesPointSize() const
{
    return mUsesPointSize;
}

bool ProgramBinary::usesPointSpriteEmulation() const
{
    return mUsesPointSize && mRenderer->getMajorShaderModel() >= 4;
}

bool ProgramBinary::usesGeometryShader() const
{
    return usesPointSpriteEmulation();
}



GLint ProgramBinary::getSamplerMapping(SamplerType type, unsigned int samplerIndex, const Caps &caps)
{
    GLint logicalTextureUnit = -1;

    switch (type)
    {
      case SAMPLER_PIXEL:
        ASSERT(samplerIndex < ArraySize(mSamplersPS));

        if (mSamplersPS[samplerIndex].active)
        {
            logicalTextureUnit = mSamplersPS[samplerIndex].logicalTextureUnit;
        }
        break;
      case SAMPLER_VERTEX:
        ASSERT(samplerIndex < ArraySize(mSamplersVS));

        if (mSamplersVS[samplerIndex].active)
        {
            logicalTextureUnit = mSamplersVS[samplerIndex].logicalTextureUnit;
        }
        break;
      default: UNREACHABLE();
    }

    if (logicalTextureUnit >= 0 && logicalTextureUnit < static_cast<GLint>(caps.maxCombinedTextureImageUnits))
    {
        return logicalTextureUnit;
    }

    return -1;
}



TextureType ProgramBinary::getSamplerTextureType(SamplerType type, unsigned int samplerIndex)
{
    switch (type)
    {
      case SAMPLER_PIXEL:
        ASSERT(samplerIndex < ArraySize(mSamplersPS));
        ASSERT(mSamplersPS[samplerIndex].active);
        return mSamplersPS[samplerIndex].textureType;
      case SAMPLER_VERTEX:
        ASSERT(samplerIndex < ArraySize(mSamplersVS));
        ASSERT(mSamplersVS[samplerIndex].active);
        return mSamplersVS[samplerIndex].textureType;
      default: UNREACHABLE();
    }

    return TEXTURE_2D;
}

GLint ProgramBinary::getUniformLocation(std::string name)
{
    unsigned int subscript = ParseAndStripArrayIndex(&name);

    unsigned int numUniforms = mUniformIndex.size();
    for (unsigned int location = 0; location < numUniforms; location++)
    {
        if (mUniformIndex[location].name == name)
        {
            const int index = mUniformIndex[location].index;
            const bool isArray = mUniforms[index]->isArray();

            if ((isArray && mUniformIndex[location].element == subscript) ||
                (subscript == GL_INVALID_INDEX))
            {
                return location;
            }
        }
    }

    return -1;
}

GLuint ProgramBinary::getUniformIndex(std::string name)
{
    unsigned int subscript = ParseAndStripArrayIndex(&name);

    
    if (subscript != 0 && subscript != GL_INVALID_INDEX)
    {
        return GL_INVALID_INDEX;
    }

    unsigned int numUniforms = mUniforms.size();
    for (unsigned int index = 0; index < numUniforms; index++)
    {
        if (mUniforms[index]->name == name)
        {
            if (mUniforms[index]->isArray() || subscript == GL_INVALID_INDEX)
            {
                return index;
            }
        }
    }

    return GL_INVALID_INDEX;
}

GLuint ProgramBinary::getUniformBlockIndex(std::string name)
{
    unsigned int subscript = ParseAndStripArrayIndex(&name);

    unsigned int numUniformBlocks = mUniformBlocks.size();
    for (unsigned int blockIndex = 0; blockIndex < numUniformBlocks; blockIndex++)
    {
        const UniformBlock &uniformBlock = *mUniformBlocks[blockIndex];
        if (uniformBlock.name == name)
        {
            const bool arrayElementZero = (subscript == GL_INVALID_INDEX && uniformBlock.elementIndex == 0);
            if (subscript == uniformBlock.elementIndex || arrayElementZero)
            {
                return blockIndex;
            }
        }
    }

    return GL_INVALID_INDEX;
}

UniformBlock *ProgramBinary::getUniformBlockByIndex(GLuint blockIndex)
{
    ASSERT(blockIndex < mUniformBlocks.size());
    return mUniformBlocks[blockIndex];
}

GLint ProgramBinary::getFragDataLocation(const char *name) const
{
    std::string baseName(name);
    unsigned int arrayIndex;
    arrayIndex = ParseAndStripArrayIndex(&baseName);

    for (auto locationIt = mOutputVariables.begin(); locationIt != mOutputVariables.end(); locationIt++)
    {
        const VariableLocation &outputVariable = locationIt->second;

        if (outputVariable.name == baseName && (arrayIndex == GL_INVALID_INDEX || arrayIndex == outputVariable.element))
        {
            return static_cast<GLint>(locationIt->first);
        }
    }

    return -1;
}

size_t ProgramBinary::getTransformFeedbackVaryingCount() const
{
    return mTransformFeedbackLinkedVaryings.size();
}

const LinkedVarying &ProgramBinary::getTransformFeedbackVarying(size_t idx) const
{
    return mTransformFeedbackLinkedVaryings[idx];
}

GLenum ProgramBinary::getTransformFeedbackBufferMode() const
{
    return mTransformFeedbackBufferMode;
}

template <typename T>
static inline void SetIfDirty(T *dest, const T& source, bool *dirtyFlag)
{
    ASSERT(dest != NULL);
    ASSERT(dirtyFlag != NULL);

    *dirtyFlag = *dirtyFlag || (memcmp(dest, &source, sizeof(T)) != 0);
    *dest = source;
}

template <typename T>
void ProgramBinary::setUniform(GLint location, GLsizei count, const T* v, GLenum targetUniformType)
{
    const int components = VariableComponentCount(targetUniformType);
    const GLenum targetBoolType = VariableBoolVectorType(targetUniformType);

    LinkedUniform *targetUniform = getUniformByLocation(location);

    int elementCount = targetUniform->elementCount();

    count = std::min(elementCount - (int)mUniformIndex[location].element, count);

    if (targetUniform->type == targetUniformType)
    {
        T *target = (T*)targetUniform->data + mUniformIndex[location].element * 4;

        for (int i = 0; i < count; i++)
        {
            for (int c = 0; c < components; c++)
            {
                SetIfDirty(target + c, v[c], &targetUniform->dirty);
            }
            for (int c = components; c < 4; c++)
            {
                SetIfDirty(target + c, T(0), &targetUniform->dirty);
            }
            target += 4;
            v += components;
        }
    }
    else if (targetUniform->type == targetBoolType)
    {
        GLint *boolParams = (GLint*)targetUniform->data + mUniformIndex[location].element * 4;

        for (int i = 0; i < count; i++)
        {
            for (int c = 0; c < components; c++)
            {
                SetIfDirty(boolParams + c, (v[c] == static_cast<T>(0)) ? GL_FALSE : GL_TRUE, &targetUniform->dirty);
            }
            for (int c = components; c < 4; c++)
            {
                SetIfDirty(boolParams + c, GL_FALSE, &targetUniform->dirty);
            }
            boolParams += 4;
            v += components;
        }
    }
    else UNREACHABLE();
}

void ProgramBinary::setUniform1fv(GLint location, GLsizei count, const GLfloat* v)
{
    setUniform(location, count, v, GL_FLOAT);
}

void ProgramBinary::setUniform2fv(GLint location, GLsizei count, const GLfloat *v)
{
    setUniform(location, count, v, GL_FLOAT_VEC2);
}

void ProgramBinary::setUniform3fv(GLint location, GLsizei count, const GLfloat *v)
{
    setUniform(location, count, v, GL_FLOAT_VEC3);
}

void ProgramBinary::setUniform4fv(GLint location, GLsizei count, const GLfloat *v)
{
    setUniform(location, count, v, GL_FLOAT_VEC4);
}

template<typename T>
bool transposeMatrix(T *target, const GLfloat *value, int targetWidth, int targetHeight, int srcWidth, int srcHeight)
{
    bool dirty = false;
    int copyWidth = std::min(targetHeight, srcWidth);
    int copyHeight = std::min(targetWidth, srcHeight);

    for (int x = 0; x < copyWidth; x++)
    {
        for (int y = 0; y < copyHeight; y++)
        {
            SetIfDirty(target + (x * targetWidth + y), static_cast<T>(value[y * srcWidth + x]), &dirty);
        }
    }
    
    for (int y = 0; y < copyWidth; y++)
    {
        for (int x = copyHeight; x < targetWidth; x++)
        {
            SetIfDirty(target + (y * targetWidth + x), static_cast<T>(0), &dirty);
        }
    }
    
    for (int y = copyWidth; y < targetHeight; y++)
    {
        for (int x = 0; x < targetWidth; x++)
        {
            SetIfDirty(target + (y * targetWidth + x), static_cast<T>(0), &dirty);
        }
    }

    return dirty;
}

template<typename T>
bool expandMatrix(T *target, const GLfloat *value, int targetWidth, int targetHeight, int srcWidth, int srcHeight)
{
    bool dirty = false;
    int copyWidth = std::min(targetWidth, srcWidth);
    int copyHeight = std::min(targetHeight, srcHeight);

    for (int y = 0; y < copyHeight; y++)
    {
        for (int x = 0; x < copyWidth; x++)
        {
            SetIfDirty(target + (y * targetWidth + x), static_cast<T>(value[y * srcWidth + x]), &dirty);
        }
    }
    
    for (int y = 0; y < copyHeight; y++)
    {
        for (int x = copyWidth; x < targetWidth; x++)
        {
            SetIfDirty(target + (y * targetWidth + x), static_cast<T>(0), &dirty);
        }
    }
    
    for (int y = copyHeight; y < targetHeight; y++)
    {
        for (int x = 0; x < targetWidth; x++)
        {
            SetIfDirty(target + (y * targetWidth + x), static_cast<T>(0), &dirty);
        }
    }

    return dirty;
}

template <int cols, int rows>
void ProgramBinary::setUniformMatrixfv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value, GLenum targetUniformType)
{
    LinkedUniform *targetUniform = getUniformByLocation(location);

    int elementCount = targetUniform->elementCount();

    count = std::min(elementCount - (int)mUniformIndex[location].element, count);
    const unsigned int targetMatrixStride = (4 * rows);
    GLfloat *target = (GLfloat*)(targetUniform->data + mUniformIndex[location].element * sizeof(GLfloat) * targetMatrixStride);

    for (int i = 0; i < count; i++)
    {
        
        if (transpose == GL_FALSE)
        {
            targetUniform->dirty = transposeMatrix<GLfloat>(target, value, 4, rows, rows, cols) || targetUniform->dirty;
        }
        else
        {
            targetUniform->dirty = expandMatrix<GLfloat>(target, value, 4, rows, cols, rows) || targetUniform->dirty;
        }
        target += targetMatrixStride;
        value += cols * rows;
    }
}

void ProgramBinary::setUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    setUniformMatrixfv<2, 2>(location, count, transpose, value, GL_FLOAT_MAT2);
}

void ProgramBinary::setUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    setUniformMatrixfv<3, 3>(location, count, transpose, value, GL_FLOAT_MAT3);
}

void ProgramBinary::setUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    setUniformMatrixfv<4, 4>(location, count, transpose, value, GL_FLOAT_MAT4);
}

void ProgramBinary::setUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    setUniformMatrixfv<2, 3>(location, count, transpose, value, GL_FLOAT_MAT2x3);
}

void ProgramBinary::setUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    setUniformMatrixfv<3, 2>(location, count, transpose, value, GL_FLOAT_MAT3x2);
}

void ProgramBinary::setUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    setUniformMatrixfv<2, 4>(location, count, transpose, value, GL_FLOAT_MAT2x4);
}

void ProgramBinary::setUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    setUniformMatrixfv<4, 2>(location, count, transpose, value, GL_FLOAT_MAT4x2);
}

void ProgramBinary::setUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    setUniformMatrixfv<3, 4>(location, count, transpose, value, GL_FLOAT_MAT3x4);
}

void ProgramBinary::setUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    setUniformMatrixfv<4, 3>(location, count, transpose, value, GL_FLOAT_MAT4x3);
}

void ProgramBinary::setUniform1iv(GLint location, GLsizei count, const GLint *v)
{
    LinkedUniform *targetUniform = mUniforms[mUniformIndex[location].index];

    int elementCount = targetUniform->elementCount();

    count = std::min(elementCount - (int)mUniformIndex[location].element, count);

    if (targetUniform->type == GL_INT || IsSampler(targetUniform->type))
    {
        GLint *target = (GLint*)targetUniform->data + mUniformIndex[location].element * 4;

        for (int i = 0; i < count; i++)
        {
            SetIfDirty(target + 0, v[0], &targetUniform->dirty);
            SetIfDirty(target + 1, 0, &targetUniform->dirty);
            SetIfDirty(target + 2, 0, &targetUniform->dirty);
            SetIfDirty(target + 3, 0, &targetUniform->dirty);
            target += 4;
            v += 1;
        }
    }
    else if (targetUniform->type == GL_BOOL)
    {
        GLint *boolParams = (GLint*)targetUniform->data + mUniformIndex[location].element * 4;

        for (int i = 0; i < count; i++)
        {
            SetIfDirty(boolParams + 0, (v[0] == 0) ? GL_FALSE : GL_TRUE, &targetUniform->dirty);
            SetIfDirty(boolParams + 1, GL_FALSE, &targetUniform->dirty);
            SetIfDirty(boolParams + 2, GL_FALSE, &targetUniform->dirty);
            SetIfDirty(boolParams + 3, GL_FALSE, &targetUniform->dirty);
            boolParams += 4;
            v += 1;
        }
    }
    else UNREACHABLE();

    
    if (IsSampler(targetUniform->type) &&
        (memcmp(targetUniform->data, v, sizeof(GLint)) != 0))
    {
        mDirtySamplerMapping = true;
    }
}

void ProgramBinary::setUniform2iv(GLint location, GLsizei count, const GLint *v)
{
    setUniform(location, count, v, GL_INT_VEC2);
}

void ProgramBinary::setUniform3iv(GLint location, GLsizei count, const GLint *v)
{
    setUniform(location, count, v, GL_INT_VEC3);
}

void ProgramBinary::setUniform4iv(GLint location, GLsizei count, const GLint *v)
{
    setUniform(location, count, v, GL_INT_VEC4);
}

void ProgramBinary::setUniform1uiv(GLint location, GLsizei count, const GLuint *v)
{
    setUniform(location, count, v, GL_UNSIGNED_INT);
}

void ProgramBinary::setUniform2uiv(GLint location, GLsizei count, const GLuint *v)
{
    setUniform(location, count, v, GL_UNSIGNED_INT_VEC2);
}

void ProgramBinary::setUniform3uiv(GLint location, GLsizei count, const GLuint *v)
{
    setUniform(location, count, v, GL_UNSIGNED_INT_VEC3);
}

void ProgramBinary::setUniform4uiv(GLint location, GLsizei count, const GLuint *v)
{
    setUniform(location, count, v, GL_UNSIGNED_INT_VEC4);
}

template <typename T>
void ProgramBinary::getUniformv(GLint location, T *params, GLenum uniformType)
{
    LinkedUniform *targetUniform = mUniforms[mUniformIndex[location].index];

    if (IsMatrixType(targetUniform->type))
    {
        const int rows = VariableRowCount(targetUniform->type);
        const int cols = VariableColumnCount(targetUniform->type);
        transposeMatrix(params, (GLfloat*)targetUniform->data + mUniformIndex[location].element * 4 * rows, rows, cols, 4, rows);
    }
    else if (uniformType == VariableComponentType(targetUniform->type))
    {
        unsigned int size = VariableComponentCount(targetUniform->type);
        memcpy(params, targetUniform->data + mUniformIndex[location].element * 4 * sizeof(T),
                size * sizeof(T));
    }
    else
    {
        unsigned int size = VariableComponentCount(targetUniform->type);
        switch (VariableComponentType(targetUniform->type))
        {
          case GL_BOOL:
            {
                GLint *boolParams = (GLint*)targetUniform->data + mUniformIndex[location].element * 4;

                for (unsigned int i = 0; i < size; i++)
                {
                    params[i] = (boolParams[i] == GL_FALSE) ? static_cast<T>(0) : static_cast<T>(1);
                }
            }
            break;

          case GL_FLOAT:
            {
                GLfloat *floatParams = (GLfloat*)targetUniform->data + mUniformIndex[location].element * 4;

                for (unsigned int i = 0; i < size; i++)
                {
                    params[i] = static_cast<T>(floatParams[i]);
                }
            }
            break;

          case GL_INT:
            {
                GLint *intParams = (GLint*)targetUniform->data + mUniformIndex[location].element * 4;

                for (unsigned int i = 0; i < size; i++)
                {
                    params[i] = static_cast<T>(intParams[i]);
                }
            }
            break;

          case GL_UNSIGNED_INT:
            {
                GLuint *uintParams = (GLuint*)targetUniform->data + mUniformIndex[location].element * 4;

                for (unsigned int i = 0; i < size; i++)
                {
                    params[i] = static_cast<T>(uintParams[i]);
                }
            }
            break;

          default: UNREACHABLE();
        }
    }
}

void ProgramBinary::getUniformfv(GLint location, GLfloat *params)
{
    getUniformv(location, params, GL_FLOAT);
}

void ProgramBinary::getUniformiv(GLint location, GLint *params)
{
    getUniformv(location, params, GL_INT);
}

void ProgramBinary::getUniformuiv(GLint location, GLuint *params)
{
    getUniformv(location, params, GL_UNSIGNED_INT);
}

void ProgramBinary::dirtyAllUniforms()
{
    unsigned int numUniforms = mUniforms.size();
    for (unsigned int index = 0; index < numUniforms; index++)
    {
        mUniforms[index]->dirty = true;
    }
}

void ProgramBinary::updateSamplerMapping()
{
    if (!mDirtySamplerMapping)
    {
        return;
    }

    mDirtySamplerMapping = false;

    
    for (size_t uniformIndex = 0; uniformIndex < mUniforms.size(); uniformIndex++)
    {
        LinkedUniform *targetUniform = mUniforms[uniformIndex];

        if (targetUniform->dirty)
        {
            if (IsSampler(targetUniform->type))
            {
                int count = targetUniform->elementCount();
                GLint (*v)[4] = reinterpret_cast<GLint(*)[4]>(targetUniform->data);

                if (targetUniform->isReferencedByFragmentShader())
                {
                    unsigned int firstIndex = targetUniform->psRegisterIndex;

                    for (int i = 0; i < count; i++)
                    {
                        unsigned int samplerIndex = firstIndex + i;

                        if (samplerIndex < MAX_TEXTURE_IMAGE_UNITS)
                        {
                            ASSERT(mSamplersPS[samplerIndex].active);
                            mSamplersPS[samplerIndex].logicalTextureUnit = v[i][0];
                        }
                    }
                }

                if (targetUniform->isReferencedByVertexShader())
                {
                    unsigned int firstIndex = targetUniform->vsRegisterIndex;

                    for (int i = 0; i < count; i++)
                    {
                        unsigned int samplerIndex = firstIndex + i;

                        if (samplerIndex < IMPLEMENTATION_MAX_VERTEX_TEXTURE_IMAGE_UNITS)
                        {
                            ASSERT(mSamplersVS[samplerIndex].active);
                            mSamplersVS[samplerIndex].logicalTextureUnit = v[i][0];
                        }
                    }
                }
            }
        }
    }
}


void ProgramBinary::applyUniforms()
{
    updateSamplerMapping();

    mRenderer->applyUniforms(*this);

    for (size_t uniformIndex = 0; uniformIndex < mUniforms.size(); uniformIndex++)
    {
        mUniforms[uniformIndex]->dirty = false;
    }
}

bool ProgramBinary::applyUniformBuffers(const std::vector<gl::Buffer*> boundBuffers, const Caps &caps)
{
    const gl::Buffer *vertexUniformBuffers[gl::IMPLEMENTATION_MAX_VERTEX_SHADER_UNIFORM_BUFFERS] = {NULL};
    const gl::Buffer *fragmentUniformBuffers[gl::IMPLEMENTATION_MAX_FRAGMENT_SHADER_UNIFORM_BUFFERS] = {NULL};

    const unsigned int reservedBuffersInVS = mRenderer->getReservedVertexUniformBuffers();
    const unsigned int reservedBuffersInFS = mRenderer->getReservedFragmentUniformBuffers();

    ASSERT(boundBuffers.size() == mUniformBlocks.size());

    for (unsigned int uniformBlockIndex = 0; uniformBlockIndex < mUniformBlocks.size(); uniformBlockIndex++)
    {
        UniformBlock *uniformBlock = getUniformBlockByIndex(uniformBlockIndex);
        gl::Buffer *uniformBuffer = boundBuffers[uniformBlockIndex];

        ASSERT(uniformBlock && uniformBuffer);

        if (uniformBuffer->getSize() < uniformBlock->dataSize)
        {
            
            return false;
        }

        ASSERT(uniformBlock->isReferencedByVertexShader() || uniformBlock->isReferencedByFragmentShader());

        if (uniformBlock->isReferencedByVertexShader())
        {
            unsigned int registerIndex = uniformBlock->vsRegisterIndex - reservedBuffersInVS;
            ASSERT(vertexUniformBuffers[registerIndex] == NULL);
            ASSERT(registerIndex < caps.maxVertexUniformBlocks);
            vertexUniformBuffers[registerIndex] = uniformBuffer;
        }

        if (uniformBlock->isReferencedByFragmentShader())
        {
            unsigned int registerIndex = uniformBlock->psRegisterIndex - reservedBuffersInFS;
            ASSERT(fragmentUniformBuffers[registerIndex] == NULL);
            ASSERT(registerIndex < caps.maxFragmentUniformBlocks);
            fragmentUniformBuffers[registerIndex] = uniformBuffer;
        }
    }

    return mRenderer->setUniformBuffers(vertexUniformBuffers, fragmentUniformBuffers);
}

bool ProgramBinary::linkVaryings(InfoLog &infoLog, Shader *fragmentShader, Shader *vertexShader)
{
    rx::VertexShaderD3D *vertexShaderD3D = rx::VertexShaderD3D::makeVertexShaderD3D(vertexShader->getImplementation());
    rx::FragmentShaderD3D *fragmentShaderD3D = rx::FragmentShaderD3D::makeFragmentShaderD3D(fragmentShader->getImplementation());

    std::vector<PackedVarying> &fragmentVaryings = fragmentShaderD3D->getVaryings();
    std::vector<PackedVarying> &vertexVaryings = vertexShaderD3D->getVaryings();

    for (size_t fragVaryingIndex = 0; fragVaryingIndex < fragmentVaryings.size(); fragVaryingIndex++)
    {
        PackedVarying *input = &fragmentVaryings[fragVaryingIndex];
        bool matched = false;

        for (size_t vertVaryingIndex = 0; vertVaryingIndex < vertexVaryings.size(); vertVaryingIndex++)
        {
            PackedVarying *output = &vertexVaryings[vertVaryingIndex];
            if (output->name == input->name)
            {
                if (!linkValidateVaryings(infoLog, output->name, *input, *output))
                {
                    return false;
                }

                output->registerIndex = input->registerIndex;

                matched = true;
                break;
            }
        }

        if (!matched)
        {
            infoLog.append("Fragment varying %s does not match any vertex varying", input->name.c_str());
            return false;
        }
    }

    return true;
}

bool ProgramBinary::load(InfoLog &infoLog, GLenum binaryFormat, const void *binary, GLsizei length)
{
#ifdef ANGLE_DISABLE_PROGRAM_BINARY_LOAD
    return false;
#else
    ASSERT(binaryFormat == GL_PROGRAM_BINARY_ANGLE);

    reset();

    BinaryInputStream stream(binary, length);

    int format = stream.readInt<int>();
    if (format != GL_PROGRAM_BINARY_ANGLE)
    {
        infoLog.append("Invalid program binary format.");
        return false;
    }

    int majorVersion = stream.readInt<int>();
    int minorVersion = stream.readInt<int>();
    if (majorVersion != ANGLE_MAJOR_VERSION || minorVersion != ANGLE_MINOR_VERSION)
    {
        infoLog.append("Invalid program binary version.");
        return false;
    }

    unsigned char commitString[ANGLE_COMMIT_HASH_SIZE];
    stream.readBytes(commitString, ANGLE_COMMIT_HASH_SIZE);
    if (memcmp(commitString, ANGLE_COMMIT_HASH, sizeof(unsigned char) * ANGLE_COMMIT_HASH_SIZE) != 0)
    {
        infoLog.append("Invalid program binary version.");
        return false;
    }

    int compileFlags = stream.readInt<int>();
    if (compileFlags != ANGLE_COMPILE_OPTIMIZATION_LEVEL)
    {
        infoLog.append("Mismatched compilation flags.");
        return false;
    }

    for (int i = 0; i < MAX_VERTEX_ATTRIBS; ++i)
    {
        stream.readInt(&mLinkedAttribute[i].type);
        stream.readString(&mLinkedAttribute[i].name);
        stream.readInt(&mShaderAttributes[i].type);
        stream.readString(&mShaderAttributes[i].name);
        stream.readInt(&mSemanticIndex[i]);
    }

    initAttributesByLayout();

    for (unsigned int i = 0; i < MAX_TEXTURE_IMAGE_UNITS; ++i)
    {
        stream.readBool(&mSamplersPS[i].active);
        stream.readInt(&mSamplersPS[i].logicalTextureUnit);
        stream.readInt(&mSamplersPS[i].textureType);
    }

    for (unsigned int i = 0; i < IMPLEMENTATION_MAX_VERTEX_TEXTURE_IMAGE_UNITS; ++i)
    {
        stream.readBool(&mSamplersVS[i].active);
        stream.readInt(&mSamplersVS[i].logicalTextureUnit);
        stream.readInt(&mSamplersVS[i].textureType);
    }

    stream.readInt(&mUsedVertexSamplerRange);
    stream.readInt(&mUsedPixelSamplerRange);
    stream.readBool(&mUsesPointSize);
    stream.readInt(&mShaderVersion);

    const unsigned int uniformCount = stream.readInt<unsigned int>();
    if (stream.error())
    {
        infoLog.append("Invalid program binary.");
        return false;
    }

    mUniforms.resize(uniformCount);
    for (unsigned int uniformIndex = 0; uniformIndex < uniformCount; uniformIndex++)
    {
        GLenum type = stream.readInt<GLenum>();
        GLenum precision = stream.readInt<GLenum>();
        std::string name = stream.readString();
        unsigned int arraySize = stream.readInt<unsigned int>();
        int blockIndex = stream.readInt<int>();

        int offset = stream.readInt<int>();
        int arrayStride = stream.readInt<int>();
        int matrixStride = stream.readInt<int>();
        bool isRowMajorMatrix = stream.readBool();

        const sh::BlockMemberInfo blockInfo(offset, arrayStride, matrixStride, isRowMajorMatrix);

        LinkedUniform *uniform = new LinkedUniform(type, precision, name, arraySize, blockIndex, blockInfo);

        stream.readInt(&uniform->psRegisterIndex);
        stream.readInt(&uniform->vsRegisterIndex);
        stream.readInt(&uniform->registerCount);
        stream.readInt(&uniform->registerElement);

        mUniforms[uniformIndex] = uniform;
    }

    unsigned int uniformBlockCount = stream.readInt<unsigned int>();
    if (stream.error())
    {
        infoLog.append("Invalid program binary.");
        return false;
    }

    mUniformBlocks.resize(uniformBlockCount);
    for (unsigned int uniformBlockIndex = 0; uniformBlockIndex < uniformBlockCount; ++uniformBlockIndex)
    {
        std::string name = stream.readString();
        unsigned int elementIndex = stream.readInt<unsigned int>();
        unsigned int dataSize = stream.readInt<unsigned int>();

        UniformBlock *uniformBlock = new UniformBlock(name, elementIndex, dataSize);

        stream.readInt(&uniformBlock->psRegisterIndex);
        stream.readInt(&uniformBlock->vsRegisterIndex);

        unsigned int numMembers = stream.readInt<unsigned int>();
        uniformBlock->memberUniformIndexes.resize(numMembers);
        for (unsigned int blockMemberIndex = 0; blockMemberIndex < numMembers; blockMemberIndex++)
        {
            stream.readInt(&uniformBlock->memberUniformIndexes[blockMemberIndex]);
        }

        mUniformBlocks[uniformBlockIndex] = uniformBlock;
    }

    const unsigned int uniformIndexCount = stream.readInt<unsigned int>();
    if (stream.error())
    {
        infoLog.append("Invalid program binary.");
        return false;
    }

    mUniformIndex.resize(uniformIndexCount);
    for (unsigned int uniformIndexIndex = 0; uniformIndexIndex < uniformIndexCount; uniformIndexIndex++)
    {
        stream.readString(&mUniformIndex[uniformIndexIndex].name);
        stream.readInt(&mUniformIndex[uniformIndexIndex].element);
        stream.readInt(&mUniformIndex[uniformIndexIndex].index);
    }

    stream.readInt(&mTransformFeedbackBufferMode);
    const unsigned int transformFeedbackVaryingCount = stream.readInt<unsigned int>();
    mTransformFeedbackLinkedVaryings.resize(transformFeedbackVaryingCount);
    for (unsigned int varyingIndex = 0; varyingIndex < transformFeedbackVaryingCount; varyingIndex++)
    {
        LinkedVarying &varying = mTransformFeedbackLinkedVaryings[varyingIndex];

        stream.readString(&varying.name);
        stream.readInt(&varying.type);
        stream.readInt(&varying.size);
        stream.readString(&varying.semanticName);
        stream.readInt(&varying.semanticIndex);
        stream.readInt(&varying.semanticIndexCount);
    }

    stream.readString(&mVertexHLSL);

    stream.readInt(&mVertexWorkarounds);

    const unsigned int vertexShaderCount = stream.readInt<unsigned int>();
    for (unsigned int vertexShaderIndex = 0; vertexShaderIndex < vertexShaderCount; vertexShaderIndex++)
    {
        VertexFormat inputLayout[MAX_VERTEX_ATTRIBS];

        for (size_t inputIndex = 0; inputIndex < MAX_VERTEX_ATTRIBS; inputIndex++)
        {
            VertexFormat *vertexInput = &inputLayout[inputIndex];
            stream.readInt(&vertexInput->mType);
            stream.readInt(&vertexInput->mNormalized);
            stream.readInt(&vertexInput->mComponents);
            stream.readBool(&vertexInput->mPureInteger);
        }

        unsigned int vertexShaderSize = stream.readInt<unsigned int>();
        const unsigned char *vertexShaderFunction = reinterpret_cast<const unsigned char*>(binary) + stream.offset();
        rx::ShaderExecutable *shaderExecutable = mRenderer->loadExecutable(reinterpret_cast<const DWORD*>(vertexShaderFunction),
                                                                           vertexShaderSize, rx::SHADER_VERTEX,
                                                                           mTransformFeedbackLinkedVaryings,
                                                                           (mTransformFeedbackBufferMode == GL_SEPARATE_ATTRIBS));
        if (!shaderExecutable)
        {
            infoLog.append("Could not create vertex shader.");
            return false;
        }

        
        GLenum signature[MAX_VERTEX_ATTRIBS];
        mDynamicHLSL->getInputLayoutSignature(inputLayout, signature);

        
        mVertexExecutables.push_back(new VertexExecutable(inputLayout, signature, shaderExecutable));

        stream.skip(vertexShaderSize);
    }

    stream.readString(&mPixelHLSL);
    stream.readInt(&mPixelWorkarounds);
    stream.readBool(&mUsesFragDepth);

    const size_t pixelShaderKeySize = stream.readInt<unsigned int>();
    mPixelShaderKey.resize(pixelShaderKeySize);
    for (size_t pixelShaderKeyIndex = 0; pixelShaderKeyIndex < pixelShaderKeySize; pixelShaderKeyIndex++)
    {
        stream.readInt(&mPixelShaderKey[pixelShaderKeyIndex].type);
        stream.readString(&mPixelShaderKey[pixelShaderKeyIndex].name);
        stream.readString(&mPixelShaderKey[pixelShaderKeyIndex].source);
        stream.readInt(&mPixelShaderKey[pixelShaderKeyIndex].outputIndex);
    }

    const size_t pixelShaderCount = stream.readInt<unsigned int>();
    for (size_t pixelShaderIndex = 0; pixelShaderIndex < pixelShaderCount; pixelShaderIndex++)
    {
        const size_t outputCount = stream.readInt<unsigned int>();
        std::vector<GLenum> outputs(outputCount);
        for (size_t outputIndex = 0; outputIndex < outputCount; outputIndex++)
        {
            stream.readInt(&outputs[outputIndex]);
        }

        const size_t pixelShaderSize = stream.readInt<unsigned int>();
        const unsigned char *pixelShaderFunction = reinterpret_cast<const unsigned char*>(binary) + stream.offset();
        rx::ShaderExecutable *shaderExecutable = mRenderer->loadExecutable(pixelShaderFunction, pixelShaderSize,
                                                                           rx::SHADER_PIXEL,
                                                                           mTransformFeedbackLinkedVaryings,
                                                                           (mTransformFeedbackBufferMode == GL_SEPARATE_ATTRIBS));
        if (!shaderExecutable)
        {
            infoLog.append("Could not create pixel shader.");
            return false;
        }

        
        mPixelExecutables.push_back(new PixelExecutable(outputs, shaderExecutable));

        stream.skip(pixelShaderSize);
    }

    unsigned int geometryShaderSize = stream.readInt<unsigned int>();

    if (geometryShaderSize > 0)
    {
        const char *geometryShaderFunction = (const char*) binary + stream.offset();
        mGeometryExecutable = mRenderer->loadExecutable(reinterpret_cast<const DWORD*>(geometryShaderFunction),
                                                        geometryShaderSize, rx::SHADER_GEOMETRY, mTransformFeedbackLinkedVaryings,
                                                        (mTransformFeedbackBufferMode == GL_SEPARATE_ATTRIBS));
        if (!mGeometryExecutable)
        {
            infoLog.append("Could not create geometry shader.");
            return false;
        }
        stream.skip(geometryShaderSize);
    }

    const char *ptr = (const char*) binary + stream.offset();

    const GUID *binaryIdentifier = (const GUID *) ptr;
    ptr += sizeof(GUID);

    GUID identifier = mRenderer->getAdapterIdentifier();
    if (memcmp(&identifier, binaryIdentifier, sizeof(GUID)) != 0)
    {
        infoLog.append("Invalid program binary.");
        return false;
    }

    initializeUniformStorage();

    return true;
#endif 
}

bool ProgramBinary::save(GLenum *binaryFormat, void *binary, GLsizei bufSize, GLsizei *length)
{
    if (binaryFormat)
    {
        *binaryFormat = GL_PROGRAM_BINARY_ANGLE;
    }

    BinaryOutputStream stream;

    stream.writeInt(GL_PROGRAM_BINARY_ANGLE);
    stream.writeInt(ANGLE_MAJOR_VERSION);
    stream.writeInt(ANGLE_MINOR_VERSION);
    stream.writeBytes(reinterpret_cast<const unsigned char*>(ANGLE_COMMIT_HASH), ANGLE_COMMIT_HASH_SIZE);
    stream.writeInt(ANGLE_COMPILE_OPTIMIZATION_LEVEL);

    for (unsigned int i = 0; i < MAX_VERTEX_ATTRIBS; ++i)
    {
        stream.writeInt(mLinkedAttribute[i].type);
        stream.writeString(mLinkedAttribute[i].name);
        stream.writeInt(mShaderAttributes[i].type);
        stream.writeString(mShaderAttributes[i].name);
        stream.writeInt(mSemanticIndex[i]);
    }

    for (unsigned int i = 0; i < MAX_TEXTURE_IMAGE_UNITS; ++i)
    {
        stream.writeInt(mSamplersPS[i].active);
        stream.writeInt(mSamplersPS[i].logicalTextureUnit);
        stream.writeInt(mSamplersPS[i].textureType);
    }

    for (unsigned int i = 0; i < IMPLEMENTATION_MAX_VERTEX_TEXTURE_IMAGE_UNITS; ++i)
    {
        stream.writeInt(mSamplersVS[i].active);
        stream.writeInt(mSamplersVS[i].logicalTextureUnit);
        stream.writeInt(mSamplersVS[i].textureType);
    }

    stream.writeInt(mUsedVertexSamplerRange);
    stream.writeInt(mUsedPixelSamplerRange);
    stream.writeInt(mUsesPointSize);
    stream.writeInt(mShaderVersion);

    stream.writeInt(mUniforms.size());
    for (size_t uniformIndex = 0; uniformIndex < mUniforms.size(); ++uniformIndex)
    {
        const LinkedUniform &uniform = *mUniforms[uniformIndex];

        stream.writeInt(uniform.type);
        stream.writeInt(uniform.precision);
        stream.writeString(uniform.name);
        stream.writeInt(uniform.arraySize);
        stream.writeInt(uniform.blockIndex);

        stream.writeInt(uniform.blockInfo.offset);
        stream.writeInt(uniform.blockInfo.arrayStride);
        stream.writeInt(uniform.blockInfo.matrixStride);
        stream.writeInt(uniform.blockInfo.isRowMajorMatrix);

        stream.writeInt(uniform.psRegisterIndex);
        stream.writeInt(uniform.vsRegisterIndex);
        stream.writeInt(uniform.registerCount);
        stream.writeInt(uniform.registerElement);
    }

    stream.writeInt(mUniformBlocks.size());
    for (size_t uniformBlockIndex = 0; uniformBlockIndex < mUniformBlocks.size(); ++uniformBlockIndex)
    {
        const UniformBlock& uniformBlock = *mUniformBlocks[uniformBlockIndex];

        stream.writeString(uniformBlock.name);
        stream.writeInt(uniformBlock.elementIndex);
        stream.writeInt(uniformBlock.dataSize);

        stream.writeInt(uniformBlock.memberUniformIndexes.size());
        for (unsigned int blockMemberIndex = 0; blockMemberIndex < uniformBlock.memberUniformIndexes.size(); blockMemberIndex++)
        {
            stream.writeInt(uniformBlock.memberUniformIndexes[blockMemberIndex]);
        }

        stream.writeInt(uniformBlock.psRegisterIndex);
        stream.writeInt(uniformBlock.vsRegisterIndex);
    }

    stream.writeInt(mUniformIndex.size());
    for (size_t i = 0; i < mUniformIndex.size(); ++i)
    {
        stream.writeString(mUniformIndex[i].name);
        stream.writeInt(mUniformIndex[i].element);
        stream.writeInt(mUniformIndex[i].index);
    }

    stream.writeInt(mTransformFeedbackBufferMode);
    stream.writeInt(mTransformFeedbackLinkedVaryings.size());
    for (size_t i = 0; i < mTransformFeedbackLinkedVaryings.size(); i++)
    {
        const LinkedVarying &varying = mTransformFeedbackLinkedVaryings[i];

        stream.writeString(varying.name);
        stream.writeInt(varying.type);
        stream.writeInt(varying.size);
        stream.writeString(varying.semanticName);
        stream.writeInt(varying.semanticIndex);
        stream.writeInt(varying.semanticIndexCount);
    }

    stream.writeString(mVertexHLSL);
    stream.writeInt(mVertexWorkarounds);

    stream.writeInt(mVertexExecutables.size());
    for (size_t vertexExecutableIndex = 0; vertexExecutableIndex < mVertexExecutables.size(); vertexExecutableIndex++)
    {
        VertexExecutable *vertexExecutable = mVertexExecutables[vertexExecutableIndex];

        for (size_t inputIndex = 0; inputIndex < gl::MAX_VERTEX_ATTRIBS; inputIndex++)
        {
            const VertexFormat &vertexInput = vertexExecutable->inputs()[inputIndex];
            stream.writeInt(vertexInput.mType);
            stream.writeInt(vertexInput.mNormalized);
            stream.writeInt(vertexInput.mComponents);
            stream.writeInt(vertexInput.mPureInteger);
        }

        size_t vertexShaderSize = vertexExecutable->shaderExecutable()->getLength();
        stream.writeInt(vertexShaderSize);

        const uint8_t *vertexBlob = vertexExecutable->shaderExecutable()->getFunction();
        stream.writeBytes(vertexBlob, vertexShaderSize);
    }

    stream.writeString(mPixelHLSL);
    stream.writeInt(mPixelWorkarounds);
    stream.writeInt(mUsesFragDepth);

    stream.writeInt(mPixelShaderKey.size());
    for (size_t pixelShaderKeyIndex = 0; pixelShaderKeyIndex < mPixelShaderKey.size(); pixelShaderKeyIndex++)
    {
        const rx::PixelShaderOuputVariable &variable = mPixelShaderKey[pixelShaderKeyIndex];
        stream.writeInt(variable.type);
        stream.writeString(variable.name);
        stream.writeString(variable.source);
        stream.writeInt(variable.outputIndex);
    }

    stream.writeInt(mPixelExecutables.size());
    for (size_t pixelExecutableIndex = 0; pixelExecutableIndex < mPixelExecutables.size(); pixelExecutableIndex++)
    {
        PixelExecutable *pixelExecutable = mPixelExecutables[pixelExecutableIndex];

        const std::vector<GLenum> outputs = pixelExecutable->outputSignature();
        stream.writeInt(outputs.size());
        for (size_t outputIndex = 0; outputIndex < outputs.size(); outputIndex++)
        {
            stream.writeInt(outputs[outputIndex]);
        }

        size_t pixelShaderSize = pixelExecutable->shaderExecutable()->getLength();
        stream.writeInt(pixelShaderSize);

        const uint8_t *pixelBlob = pixelExecutable->shaderExecutable()->getFunction();
        stream.writeBytes(pixelBlob, pixelShaderSize);
    }

    size_t geometryShaderSize = (mGeometryExecutable != NULL) ? mGeometryExecutable->getLength() : 0;
    stream.writeInt(geometryShaderSize);

    if (mGeometryExecutable != NULL && geometryShaderSize > 0)
    {
        const uint8_t *geometryBlob = mGeometryExecutable->getFunction();
        stream.writeBytes(geometryBlob, geometryShaderSize);
    }

    GUID identifier = mRenderer->getAdapterIdentifier();

    GLsizei streamLength = stream.length();
    const void *streamData = stream.data();

    GLsizei totalLength = streamLength + sizeof(GUID);
    if (totalLength > bufSize)
    {
        if (length)
        {
            *length = 0;
        }

        return false;
    }

    if (binary)
    {
        char *ptr = (char*) binary;

        memcpy(ptr, streamData, streamLength);
        ptr += streamLength;

        memcpy(ptr, &identifier, sizeof(GUID));
        ptr += sizeof(GUID);

        ASSERT(ptr - totalLength == binary);
    }

    if (length)
    {
        *length = totalLength;
    }

    return true;
}

GLint ProgramBinary::getLength()
{
    GLint length;
    if (save(NULL, NULL, INT_MAX, &length))
    {
        return length;
    }
    else
    {
        return 0;
    }
}

bool ProgramBinary::link(InfoLog &infoLog, const AttributeBindings &attributeBindings, Shader *fragmentShader, Shader *vertexShader,
                         const std::vector<std::string>& transformFeedbackVaryings, GLenum transformFeedbackBufferMode, const Caps &caps)
{
    if (!fragmentShader || !fragmentShader->isCompiled())
    {
        return false;
    }
    ASSERT(fragmentShader->getType() == GL_FRAGMENT_SHADER);

    if (!vertexShader || !vertexShader->isCompiled())
    {
        return false;
    }
    ASSERT(vertexShader->getType() == GL_VERTEX_SHADER);

    reset();

    mTransformFeedbackBufferMode = transformFeedbackBufferMode;

    rx::VertexShaderD3D *vertexShaderD3D = rx::VertexShaderD3D::makeVertexShaderD3D(vertexShader->getImplementation());
    rx::FragmentShaderD3D *fragmentShaderD3D = rx::FragmentShaderD3D::makeFragmentShaderD3D(fragmentShader->getImplementation());

    mShaderVersion = vertexShaderD3D->getShaderVersion();

    mPixelHLSL = fragmentShaderD3D->getTranslatedSource();
    mPixelWorkarounds = fragmentShaderD3D->getD3DWorkarounds();

    mVertexHLSL = vertexShaderD3D->getTranslatedSource();
    mVertexWorkarounds = vertexShaderD3D->getD3DWorkarounds();

    
    rx::VaryingPacking packing = { NULL };
    int registers = mDynamicHLSL->packVaryings(infoLog, packing, fragmentShaderD3D, vertexShaderD3D, transformFeedbackVaryings);

    if (registers < 0)
    {
        return false;
    }

    if (!linkVaryings(infoLog, fragmentShader, vertexShader))
    {
        return false;
    }

    mUsesPointSize = vertexShaderD3D->usesPointSize();
    std::vector<LinkedVarying> linkedVaryings;
    if (!mDynamicHLSL->generateShaderLinkHLSL(infoLog, registers, packing, mPixelHLSL, mVertexHLSL,
                                              fragmentShaderD3D, vertexShaderD3D, transformFeedbackVaryings,
                                              &linkedVaryings, &mOutputVariables, &mPixelShaderKey, &mUsesFragDepth))
    {
        return false;
    }

    bool success = true;

    if (!linkAttributes(infoLog, attributeBindings, fragmentShader, vertexShader))
    {
        success = false;
    }

    if (!linkUniforms(infoLog, *vertexShader, *fragmentShader, caps))
    {
        success = false;
    }

    
    if (vertexShaderD3D->usesDepthRange() || fragmentShaderD3D->usesDepthRange())
    {
        const sh::BlockMemberInfo &defaultInfo = sh::BlockMemberInfo::getDefaultBlockInfo();

        mUniforms.push_back(new LinkedUniform(GL_FLOAT, GL_HIGH_FLOAT, "gl_DepthRange.near", 0, -1, defaultInfo));
        mUniforms.push_back(new LinkedUniform(GL_FLOAT, GL_HIGH_FLOAT, "gl_DepthRange.far", 0, -1, defaultInfo));
        mUniforms.push_back(new LinkedUniform(GL_FLOAT, GL_HIGH_FLOAT, "gl_DepthRange.diff", 0, -1, defaultInfo));
    }

    if (!linkUniformBlocks(infoLog, *vertexShader, *fragmentShader, caps))
    {
        success = false;
    }

    if (!gatherTransformFeedbackLinkedVaryings(infoLog, linkedVaryings, transformFeedbackVaryings,
                                               transformFeedbackBufferMode, &mTransformFeedbackLinkedVaryings, caps))
    {
        success = false;
    }

    if (success)
    {
        VertexFormat defaultInputLayout[MAX_VERTEX_ATTRIBS];
        GetInputLayoutFromShader(vertexShaderD3D->getActiveAttributes(), defaultInputLayout);
        rx::ShaderExecutable *defaultVertexExecutable = getVertexExecutableForInputLayout(defaultInputLayout);

        std::vector<GLenum> defaultPixelOutput(IMPLEMENTATION_MAX_DRAW_BUFFERS);
        for (size_t i = 0; i < defaultPixelOutput.size(); i++)
        {
            defaultPixelOutput[i] = (i == 0) ? GL_FLOAT : GL_NONE;
        }
        rx::ShaderExecutable *defaultPixelExecutable = getPixelExecutableForOutputLayout(defaultPixelOutput);

        if (usesGeometryShader())
        {
            std::string geometryHLSL = mDynamicHLSL->generateGeometryShaderHLSL(registers, fragmentShaderD3D, vertexShaderD3D);
            mGeometryExecutable = mRenderer->compileToExecutable(infoLog, geometryHLSL.c_str(), rx::SHADER_GEOMETRY,
                                                                 mTransformFeedbackLinkedVaryings,
                                                                 (mTransformFeedbackBufferMode == GL_SEPARATE_ATTRIBS),
                                                                 rx::ANGLE_D3D_WORKAROUND_NONE);
        }

        if (!defaultVertexExecutable || !defaultPixelExecutable || (usesGeometryShader() && !mGeometryExecutable))
        {
            infoLog.append("Failed to create D3D shaders.");
            success = false;
            reset();
        }
    }

    return success;
}


bool ProgramBinary::linkAttributes(InfoLog &infoLog, const AttributeBindings &attributeBindings, Shader *fragmentShader, Shader *vertexShader)
{
    rx::VertexShaderD3D *vertexShaderD3D = rx::VertexShaderD3D::makeVertexShaderD3D(vertexShader->getImplementation());

    unsigned int usedLocations = 0;
    const std::vector<sh::Attribute> &activeAttributes = vertexShaderD3D->getActiveAttributes();

    
    for (unsigned int attributeIndex = 0; attributeIndex < activeAttributes.size(); attributeIndex++)
    {
        const sh::Attribute &attribute = activeAttributes[attributeIndex];
        const int location = attribute.location == -1 ? attributeBindings.getAttributeBinding(attribute.name) : attribute.location;

        mShaderAttributes[attributeIndex] = attribute;

        if (location != -1)   
        {
            const int rows = VariableRegisterCount(attribute.type);

            if (rows + location > MAX_VERTEX_ATTRIBS)
            {
                infoLog.append("Active attribute (%s) at location %d is too big to fit", attribute.name.c_str(), location);

                return false;
            }

            for (int row = 0; row < rows; row++)
            {
                const int rowLocation = location + row;
                sh::ShaderVariable &linkedAttribute = mLinkedAttribute[rowLocation];

                
                
                if (mShaderVersion >= 300)
                {
                    if (!linkedAttribute.name.empty())
                    {
                        infoLog.append("Attribute '%s' aliases attribute '%s' at location %d", attribute.name.c_str(), linkedAttribute.name.c_str(), rowLocation);
                        return false;
                    }
                }

                linkedAttribute = attribute;
                usedLocations |= 1 << rowLocation;
            }
        }
    }

    
    for (unsigned int attributeIndex = 0; attributeIndex < activeAttributes.size(); attributeIndex++)
    {
        const sh::Attribute &attribute = activeAttributes[attributeIndex];
        const int location = attribute.location == -1 ? attributeBindings.getAttributeBinding(attribute.name) : attribute.location;

        if (location == -1)   
        {
            int rows = VariableRegisterCount(attribute.type);
            int availableIndex = AllocateFirstFreeBits(&usedLocations, rows, MAX_VERTEX_ATTRIBS);

            if (availableIndex == -1 || availableIndex + rows > MAX_VERTEX_ATTRIBS)
            {
                infoLog.append("Too many active attributes (%s)", attribute.name.c_str());

                return false;   
            }

            mLinkedAttribute[availableIndex] = attribute;
        }
    }

    for (int attributeIndex = 0; attributeIndex < MAX_VERTEX_ATTRIBS; )
    {
        int index = vertexShaderD3D->getSemanticIndex(mLinkedAttribute[attributeIndex].name);
        int rows = VariableRegisterCount(mLinkedAttribute[attributeIndex].type);

        for (int r = 0; r < rows; r++)
        {
            mSemanticIndex[attributeIndex++] = index++;
        }
    }

    initAttributesByLayout();

    return true;
}

bool ProgramBinary::linkValidateVariablesBase(InfoLog &infoLog, const std::string &variableName, const sh::ShaderVariable &vertexVariable,
                                              const sh::ShaderVariable &fragmentVariable, bool validatePrecision)
{
    if (vertexVariable.type != fragmentVariable.type)
    {
        infoLog.append("Types for %s differ between vertex and fragment shaders", variableName.c_str());
        return false;
    }
    if (vertexVariable.arraySize != fragmentVariable.arraySize)
    {
        infoLog.append("Array sizes for %s differ between vertex and fragment shaders", variableName.c_str());
        return false;
    }
    if (validatePrecision && vertexVariable.precision != fragmentVariable.precision)
    {
        infoLog.append("Precisions for %s differ between vertex and fragment shaders", variableName.c_str());
        return false;
    }

    if (vertexVariable.fields.size() != fragmentVariable.fields.size())
    {
        infoLog.append("Structure lengths for %s differ between vertex and fragment shaders", variableName.c_str());
        return false;
    }
    const unsigned int numMembers = vertexVariable.fields.size();
    for (unsigned int memberIndex = 0; memberIndex < numMembers; memberIndex++)
    {
        const sh::ShaderVariable &vertexMember = vertexVariable.fields[memberIndex];
        const sh::ShaderVariable &fragmentMember = fragmentVariable.fields[memberIndex];

        if (vertexMember.name != fragmentMember.name)
        {
            infoLog.append("Name mismatch for field '%d' of %s: (in vertex: '%s', in fragment: '%s')",
                           memberIndex, variableName.c_str(),
                           vertexMember.name.c_str(), fragmentMember.name.c_str());
            return false;
        }

        const std::string memberName = variableName.substr(0, variableName.length() - 1) + "." +
                                       vertexMember.name + "'";

        if (!linkValidateVariablesBase(infoLog, vertexMember.name, vertexMember, fragmentMember, validatePrecision))
        {
            return false;
        }
    }

    return true;
}

bool ProgramBinary::linkValidateUniforms(InfoLog &infoLog, const std::string &uniformName, const sh::Uniform &vertexUniform, const sh::Uniform &fragmentUniform)
{
    if (!linkValidateVariablesBase(infoLog, uniformName, vertexUniform, fragmentUniform, true))
    {
        return false;
    }

    return true;
}

bool ProgramBinary::linkValidateVaryings(InfoLog &infoLog, const std::string &varyingName, const sh::Varying &vertexVarying, const sh::Varying &fragmentVarying)
{
    if (!linkValidateVariablesBase(infoLog, varyingName, vertexVarying, fragmentVarying, false))
    {
        return false;
    }

    if (vertexVarying.interpolation != fragmentVarying.interpolation)
    {
        infoLog.append("Interpolation types for %s differ between vertex and fragment shaders", varyingName.c_str());
        return false;
    }

    return true;
}

bool ProgramBinary::linkValidateInterfaceBlockFields(InfoLog &infoLog, const std::string &uniformName, const sh::InterfaceBlockField &vertexUniform, const sh::InterfaceBlockField &fragmentUniform)
{
    if (!linkValidateVariablesBase(infoLog, uniformName, vertexUniform, fragmentUniform, true))
    {
        return false;
    }

    if (vertexUniform.isRowMajorLayout != fragmentUniform.isRowMajorLayout)
    {
        infoLog.append("Matrix packings for %s differ between vertex and fragment shaders", uniformName.c_str());
        return false;
    }

    return true;
}

bool ProgramBinary::linkUniforms(InfoLog &infoLog, const Shader &vertexShader, const Shader &fragmentShader, const Caps &caps)
{
    const rx::VertexShaderD3D *vertexShaderD3D = rx::VertexShaderD3D::makeVertexShaderD3D(vertexShader.getImplementation());
    const rx::FragmentShaderD3D *fragmentShaderD3D = rx::FragmentShaderD3D::makeFragmentShaderD3D(fragmentShader.getImplementation());

    const std::vector<sh::Uniform> &vertexUniforms = vertexShaderD3D->getUniforms();
    const std::vector<sh::Uniform> &fragmentUniforms = fragmentShaderD3D->getUniforms();

    
    typedef std::map<std::string, const sh::Uniform*> UniformMap;
    UniformMap linkedUniforms;

    for (unsigned int vertexUniformIndex = 0; vertexUniformIndex < vertexUniforms.size(); vertexUniformIndex++)
    {
        const sh::Uniform &vertexUniform = vertexUniforms[vertexUniformIndex];
        linkedUniforms[vertexUniform.name] = &vertexUniform;
    }

    for (unsigned int fragmentUniformIndex = 0; fragmentUniformIndex < fragmentUniforms.size(); fragmentUniformIndex++)
    {
        const sh::Uniform &fragmentUniform = fragmentUniforms[fragmentUniformIndex];
        UniformMap::const_iterator entry = linkedUniforms.find(fragmentUniform.name);
        if (entry != linkedUniforms.end())
        {
            const sh::Uniform &vertexUniform = *entry->second;
            const std::string &uniformName = "uniform '" + vertexUniform.name + "'";
            if (!linkValidateUniforms(infoLog, uniformName, vertexUniform, fragmentUniform))
            {
                return false;
            }
        }
    }

    for (unsigned int uniformIndex = 0; uniformIndex < vertexUniforms.size(); uniformIndex++)
    {
        const sh::Uniform &uniform = vertexUniforms[uniformIndex];
        defineUniformBase(GL_VERTEX_SHADER, uniform, vertexShaderD3D->getUniformRegister(uniform.name));
    }

    for (unsigned int uniformIndex = 0; uniformIndex < fragmentUniforms.size(); uniformIndex++)
    {
        const sh::Uniform &uniform = fragmentUniforms[uniformIndex];
        defineUniformBase(GL_FRAGMENT_SHADER, uniform, fragmentShaderD3D->getUniformRegister(uniform.name));
    }

    if (!indexUniforms(infoLog, caps))
    {
        return false;
    }

    initializeUniformStorage();

    return true;
}

void ProgramBinary::defineUniformBase(GLenum shader, const sh::Uniform &uniform, unsigned int uniformRegister)
{
    ShShaderOutput outputType = rx::ShaderD3D::getCompilerOutputType(shader);
    sh::HLSLBlockEncoder encoder(sh::HLSLBlockEncoder::GetStrategyFor(outputType));
    encoder.skipRegisters(uniformRegister);

    defineUniform(shader, uniform, uniform.name, &encoder);
}

void ProgramBinary::defineUniform(GLenum shader, const sh::ShaderVariable &uniform,
                                  const std::string &fullName, sh::HLSLBlockEncoder *encoder)
{
    if (uniform.isStruct())
    {
        for (unsigned int elementIndex = 0; elementIndex < uniform.elementCount(); elementIndex++)
        {
            const std::string &elementString = (uniform.isArray() ? ArrayString(elementIndex) : "");

            encoder->enterAggregateType();

            for (size_t fieldIndex = 0; fieldIndex < uniform.fields.size(); fieldIndex++)
            {
                const sh::ShaderVariable &field = uniform.fields[fieldIndex];
                const std::string &fieldFullName = (fullName + elementString + "." + field.name);

                defineUniform(shader, field, fieldFullName, encoder);
            }

            encoder->exitAggregateType();
        }
    }
    else 
    {
        
        if (uniform.isArray())
        {
            encoder->enterAggregateType();
        }

        LinkedUniform *linkedUniform = getUniformByName(fullName);

        if (!linkedUniform)
        {
            linkedUniform = new LinkedUniform(uniform.type, uniform.precision, fullName, uniform.arraySize,
                                              -1, sh::BlockMemberInfo::getDefaultBlockInfo());
            ASSERT(linkedUniform);
            linkedUniform->registerElement = encoder->getCurrentElement();
            mUniforms.push_back(linkedUniform);
        }

        ASSERT(linkedUniform->registerElement == encoder->getCurrentElement());

        if (shader == GL_FRAGMENT_SHADER)
        {
            linkedUniform->psRegisterIndex = encoder->getCurrentRegister();
        }
        else if (shader == GL_VERTEX_SHADER)
        {
            linkedUniform->vsRegisterIndex = encoder->getCurrentRegister();
        }
        else UNREACHABLE();

        
        encoder->encodeType(uniform.type, uniform.arraySize, false);

        
        if (uniform.isArray())
        {
            encoder->exitAggregateType();
        }
    }
}

bool ProgramBinary::indexSamplerUniform(const LinkedUniform &uniform, InfoLog &infoLog, const Caps &caps)
{
    ASSERT(IsSampler(uniform.type));
    ASSERT(uniform.vsRegisterIndex != GL_INVALID_INDEX || uniform.psRegisterIndex != GL_INVALID_INDEX);

    if (uniform.vsRegisterIndex != GL_INVALID_INDEX)
    {
        if (!assignSamplers(uniform.vsRegisterIndex, uniform.type, uniform.arraySize, mSamplersVS,
                            &mUsedVertexSamplerRange, caps.maxVertexTextureImageUnits))
        {
            infoLog.append("Vertex shader sampler count exceeds the maximum vertex texture units (%d).",
                           caps.maxVertexTextureImageUnits);
            return false;
        }

        unsigned int maxVertexVectors = mRenderer->getReservedVertexUniformVectors() + caps.maxVertexUniformVectors;
        if (uniform.vsRegisterIndex + uniform.registerCount > maxVertexVectors)
        {
            infoLog.append("Vertex shader active uniforms exceed GL_MAX_VERTEX_UNIFORM_VECTORS (%u)",
                           caps.maxVertexUniformVectors);
            return false;
        }
    }

    if (uniform.psRegisterIndex != GL_INVALID_INDEX)
    {
        if (!assignSamplers(uniform.psRegisterIndex, uniform.type, uniform.arraySize, mSamplersPS,
                            &mUsedPixelSamplerRange, caps.maxTextureImageUnits))
        {
            infoLog.append("Pixel shader sampler count exceeds MAX_TEXTURE_IMAGE_UNITS (%d).",
                           caps.maxTextureImageUnits);
            return false;
        }

        unsigned int maxFragmentVectors = mRenderer->getReservedFragmentUniformVectors() + caps.maxFragmentUniformVectors;
        if (uniform.psRegisterIndex + uniform.registerCount > maxFragmentVectors)
        {
            infoLog.append("Fragment shader active uniforms exceed GL_MAX_FRAGMENT_UNIFORM_VECTORS (%u)",
                           caps.maxFragmentUniformVectors);
            return false;
        }
    }

    return true;
}

bool ProgramBinary::indexUniforms(InfoLog &infoLog, const Caps &caps)
{
    for (size_t uniformIndex = 0; uniformIndex < mUniforms.size(); uniformIndex++)
    {
        const LinkedUniform &uniform = *mUniforms[uniformIndex];

        if (IsSampler(uniform.type))
        {
            if (!indexSamplerUniform(uniform, infoLog, caps))
            {
                return false;
            }
        }

        for (unsigned int arrayElementIndex = 0; arrayElementIndex < uniform.elementCount(); arrayElementIndex++)
        {
            mUniformIndex.push_back(VariableLocation(uniform.name, arrayElementIndex, uniformIndex));
        }
    }

    return true;
}

bool ProgramBinary::assignSamplers(unsigned int startSamplerIndex,
                                   GLenum samplerType,
                                   unsigned int samplerCount,
                                   Sampler *outArray,
                                   GLuint *usedRange,
                                   unsigned int limit)
{
    unsigned int samplerIndex = startSamplerIndex;

    do
    {
        if (samplerIndex < limit)
        {
            outArray[samplerIndex].active = true;
            outArray[samplerIndex].textureType = GetTextureType(samplerType);
            outArray[samplerIndex].logicalTextureUnit = 0;
            *usedRange = std::max(samplerIndex + 1, *usedRange);
        }
        else
        {
            return false;
        }

        samplerIndex++;
    } while (samplerIndex < startSamplerIndex + samplerCount);

    return true;
}

bool ProgramBinary::areMatchingInterfaceBlocks(InfoLog &infoLog, const sh::InterfaceBlock &vertexInterfaceBlock, const sh::InterfaceBlock &fragmentInterfaceBlock)
{
    const char* blockName = vertexInterfaceBlock.name.c_str();

    
    if (vertexInterfaceBlock.fields.size() != fragmentInterfaceBlock.fields.size())
    {
        infoLog.append("Types for interface block '%s' differ between vertex and fragment shaders", blockName);
        return false;
    }

    if (vertexInterfaceBlock.arraySize != fragmentInterfaceBlock.arraySize)
    {
        infoLog.append("Array sizes differ for interface block '%s' between vertex and fragment shaders", blockName);
        return false;
    }

    if (vertexInterfaceBlock.layout != fragmentInterfaceBlock.layout || vertexInterfaceBlock.isRowMajorLayout != fragmentInterfaceBlock.isRowMajorLayout)
    {
        infoLog.append("Layout qualifiers differ for interface block '%s' between vertex and fragment shaders", blockName);
        return false;
    }

    const unsigned int numBlockMembers = vertexInterfaceBlock.fields.size();
    for (unsigned int blockMemberIndex = 0; blockMemberIndex < numBlockMembers; blockMemberIndex++)
    {
        const sh::InterfaceBlockField &vertexMember = vertexInterfaceBlock.fields[blockMemberIndex];
        const sh::InterfaceBlockField &fragmentMember = fragmentInterfaceBlock.fields[blockMemberIndex];

        if (vertexMember.name != fragmentMember.name)
        {
            infoLog.append("Name mismatch for field %d of interface block '%s': (in vertex: '%s', in fragment: '%s')",
                           blockMemberIndex, blockName, vertexMember.name.c_str(), fragmentMember.name.c_str());
            return false;
        }

        std::string memberName = "interface block '" + vertexInterfaceBlock.name + "' member '" + vertexMember.name + "'";
        if (!linkValidateInterfaceBlockFields(infoLog, memberName, vertexMember, fragmentMember))
        {
            return false;
        }
    }

    return true;
}

bool ProgramBinary::linkUniformBlocks(InfoLog &infoLog, const Shader &vertexShader, const Shader &fragmentShader, const Caps &caps)
{
    const rx::VertexShaderD3D *vertexShaderD3D = rx::VertexShaderD3D::makeVertexShaderD3D(vertexShader.getImplementation());
    const rx::FragmentShaderD3D *fragmentShaderD3D = rx::FragmentShaderD3D::makeFragmentShaderD3D(fragmentShader.getImplementation());

    const std::vector<sh::InterfaceBlock> &vertexInterfaceBlocks = vertexShaderD3D->getInterfaceBlocks();
    const std::vector<sh::InterfaceBlock> &fragmentInterfaceBlocks = fragmentShaderD3D->getInterfaceBlocks();

    
    typedef std::map<std::string, const sh::InterfaceBlock*> UniformBlockMap;
    UniformBlockMap linkedUniformBlocks;

    for (unsigned int blockIndex = 0; blockIndex < vertexInterfaceBlocks.size(); blockIndex++)
    {
        const sh::InterfaceBlock &vertexInterfaceBlock = vertexInterfaceBlocks[blockIndex];
        linkedUniformBlocks[vertexInterfaceBlock.name] = &vertexInterfaceBlock;
    }

    for (unsigned int blockIndex = 0; blockIndex < fragmentInterfaceBlocks.size(); blockIndex++)
    {
        const sh::InterfaceBlock &fragmentInterfaceBlock = fragmentInterfaceBlocks[blockIndex];
        UniformBlockMap::const_iterator entry = linkedUniformBlocks.find(fragmentInterfaceBlock.name);
        if (entry != linkedUniformBlocks.end())
        {
            const sh::InterfaceBlock &vertexInterfaceBlock = *entry->second;
            if (!areMatchingInterfaceBlocks(infoLog, vertexInterfaceBlock, fragmentInterfaceBlock))
            {
                return false;
            }
        }
    }

    for (unsigned int blockIndex = 0; blockIndex < vertexInterfaceBlocks.size(); blockIndex++)
    {
        if (!defineUniformBlock(infoLog, vertexShader, vertexInterfaceBlocks[blockIndex], caps))
        {
            return false;
        }
    }

    for (unsigned int blockIndex = 0; blockIndex < fragmentInterfaceBlocks.size(); blockIndex++)
    {
        if (!defineUniformBlock(infoLog, fragmentShader, fragmentInterfaceBlocks[blockIndex], caps))
        {
            return false;
        }
    }

    return true;
}

bool ProgramBinary::gatherTransformFeedbackLinkedVaryings(InfoLog &infoLog, const std::vector<LinkedVarying> &linkedVaryings,
                                                          const std::vector<std::string> &transformFeedbackVaryingNames,
                                                          GLenum transformFeedbackBufferMode,
                                                          std::vector<LinkedVarying> *outTransformFeedbackLinkedVaryings,
                                                          const Caps &caps) const
{
    size_t totalComponents = 0;

    
    outTransformFeedbackLinkedVaryings->clear();
    for (size_t i = 0; i < transformFeedbackVaryingNames.size(); i++)
    {
        bool found = false;
        for (size_t j = 0; j < linkedVaryings.size(); j++)
        {
            if (transformFeedbackVaryingNames[i] == linkedVaryings[j].name)
            {
                for (size_t k = 0; k < outTransformFeedbackLinkedVaryings->size(); k++)
                {
                    if (outTransformFeedbackLinkedVaryings->at(k).name == linkedVaryings[j].name)
                    {
                        infoLog.append("Two transform feedback varyings specify the same output variable (%s).", linkedVaryings[j].name.c_str());
                        return false;
                    }
                }

                size_t componentCount = linkedVaryings[j].semanticIndexCount * 4;
                if (transformFeedbackBufferMode == GL_SEPARATE_ATTRIBS &&
                    componentCount > caps.maxTransformFeedbackSeparateComponents)
                {
                    infoLog.append("Transform feedback varying's %s components (%u) exceed the maximum separate components (%u).",
                                   linkedVaryings[j].name.c_str(), componentCount, caps.maxTransformFeedbackSeparateComponents);
                    return false;
                }

                totalComponents += componentCount;

                outTransformFeedbackLinkedVaryings->push_back(linkedVaryings[j]);
                found = true;
                break;
            }
        }

        
        ASSERT(found);
    }

    if (transformFeedbackBufferMode == GL_INTERLEAVED_ATTRIBS && totalComponents > caps.maxTransformFeedbackInterleavedComponents)
    {
        infoLog.append("Transform feedback varying total components (%u) exceed the maximum interleaved components (%u).",
                       totalComponents, caps.maxTransformFeedbackInterleavedComponents);
        return false;
    }

    return true;
}

template <typename VarT>
void ProgramBinary::defineUniformBlockMembers(const std::vector<VarT> &fields, const std::string &prefix, int blockIndex,
                                              sh::BlockLayoutEncoder *encoder, std::vector<unsigned int> *blockUniformIndexes,
                                              bool inRowMajorLayout)
{
    for (unsigned int uniformIndex = 0; uniformIndex < fields.size(); uniformIndex++)
    {
        const VarT &field = fields[uniformIndex];
        const std::string &fieldName = (prefix.empty() ? field.name : prefix + "." + field.name);

        if (field.isStruct())
        {
            bool rowMajorLayout = (inRowMajorLayout || IsRowMajorLayout(field));

            for (unsigned int arrayElement = 0; arrayElement < field.elementCount(); arrayElement++)
            {
                encoder->enterAggregateType();

                const std::string uniformElementName = fieldName + (field.isArray() ? ArrayString(arrayElement) : "");
                defineUniformBlockMembers(field.fields, uniformElementName, blockIndex, encoder, blockUniformIndexes, rowMajorLayout);

                encoder->exitAggregateType();
            }
        }
        else
        {
            bool isRowMajorMatrix = (IsMatrixType(field.type) && inRowMajorLayout);

            sh::BlockMemberInfo memberInfo = encoder->encodeType(field.type, field.arraySize, isRowMajorMatrix);

            LinkedUniform *newUniform = new LinkedUniform(field.type, field.precision, fieldName, field.arraySize,
                                                          blockIndex, memberInfo);

            
            blockUniformIndexes->push_back(mUniforms.size());
            mUniforms.push_back(newUniform);
        }
    }
}

bool ProgramBinary::defineUniformBlock(InfoLog &infoLog, const Shader &shader, const sh::InterfaceBlock &interfaceBlock, const Caps &caps)
{
    const rx::ShaderD3D* shaderD3D = rx::ShaderD3D::makeShaderD3D(shader.getImplementation());

    
    if (getUniformBlockIndex(interfaceBlock.name) == GL_INVALID_INDEX)
    {
        std::vector<unsigned int> blockUniformIndexes;
        const unsigned int blockIndex = mUniformBlocks.size();

        
        sh::BlockLayoutEncoder *encoder = NULL;

        if (interfaceBlock.layout == sh::BLOCKLAYOUT_STANDARD)
        {
            encoder = new sh::Std140BlockEncoder;
        }
        else
        {
            encoder = new sh::HLSLBlockEncoder(sh::HLSLBlockEncoder::ENCODE_PACKED);
        }
        ASSERT(encoder);

        defineUniformBlockMembers(interfaceBlock.fields, "", blockIndex, encoder, &blockUniformIndexes, interfaceBlock.isRowMajorLayout);

        size_t dataSize = encoder->getBlockSize();

        
        if (interfaceBlock.arraySize > 0)
        {
            for (unsigned int uniformBlockElement = 0; uniformBlockElement < interfaceBlock.arraySize; uniformBlockElement++)
            {
                UniformBlock *newUniformBlock = new UniformBlock(interfaceBlock.name, uniformBlockElement, dataSize);
                newUniformBlock->memberUniformIndexes = blockUniformIndexes;
                mUniformBlocks.push_back(newUniformBlock);
            }
        }
        else
        {
            UniformBlock *newUniformBlock = new UniformBlock(interfaceBlock.name, GL_INVALID_INDEX, dataSize);
            newUniformBlock->memberUniformIndexes = blockUniformIndexes;
            mUniformBlocks.push_back(newUniformBlock);
        }
    }

    
    const GLuint blockIndex = getUniformBlockIndex(interfaceBlock.name);
    const unsigned int elementCount = std::max(1u, interfaceBlock.arraySize);
    ASSERT(blockIndex != GL_INVALID_INDEX);
    ASSERT(blockIndex + elementCount <= mUniformBlocks.size());

    unsigned int interfaceBlockRegister = shaderD3D->getInterfaceBlockRegister(interfaceBlock.name);

    for (unsigned int uniformBlockElement = 0; uniformBlockElement < elementCount; uniformBlockElement++)
    {
        UniformBlock *uniformBlock = mUniformBlocks[blockIndex + uniformBlockElement];
        ASSERT(uniformBlock->name == interfaceBlock.name);

        if (!assignUniformBlockRegister(infoLog, uniformBlock, shader.getType(),
                                        interfaceBlockRegister + uniformBlockElement, caps))
        {
            return false;
        }
    }

    return true;
}

bool ProgramBinary::assignUniformBlockRegister(InfoLog &infoLog, UniformBlock *uniformBlock, GLenum shader, unsigned int registerIndex, const Caps &caps)
{
    if (shader == GL_VERTEX_SHADER)
    {
        uniformBlock->vsRegisterIndex = registerIndex;
        if (registerIndex - mRenderer->getReservedVertexUniformBuffers() >= caps.maxVertexUniformBlocks)
        {
            infoLog.append("Vertex shader uniform block count exceed GL_MAX_VERTEX_UNIFORM_BLOCKS (%u)", caps.maxVertexUniformBlocks);
            return false;
        }
    }
    else if (shader == GL_FRAGMENT_SHADER)
    {
        uniformBlock->psRegisterIndex = registerIndex;
        if (registerIndex - mRenderer->getReservedFragmentUniformBuffers() >= caps.maxFragmentUniformBlocks)
        {
            infoLog.append("Fragment shader uniform block count exceed GL_MAX_FRAGMENT_UNIFORM_BLOCKS (%u)", caps.maxFragmentUniformBlocks);
            return false;
        }
    }
    else UNREACHABLE();

    return true;
}

bool ProgramBinary::isValidated() const
{
    return mValidated;
}

void ProgramBinary::getActiveAttribute(GLuint index, GLsizei bufsize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) const
{
    
    unsigned int activeAttribute = 0;
    unsigned int attribute;
    for (attribute = 0; attribute < MAX_VERTEX_ATTRIBS; attribute++)
    {
        if (mLinkedAttribute[attribute].name.empty())
        {
            continue;
        }

        if (activeAttribute == index)
        {
            break;
        }

        activeAttribute++;
    }

    if (bufsize > 0)
    {
        const char *string = mLinkedAttribute[attribute].name.c_str();

        strncpy(name, string, bufsize);
        name[bufsize - 1] = '\0';

        if (length)
        {
            *length = strlen(name);
        }
    }

    *size = 1;   

    *type = mLinkedAttribute[attribute].type;
}

GLint ProgramBinary::getActiveAttributeCount() const
{
    int count = 0;

    for (int attributeIndex = 0; attributeIndex < MAX_VERTEX_ATTRIBS; attributeIndex++)
    {
        if (!mLinkedAttribute[attributeIndex].name.empty())
        {
            count++;
        }
    }

    return count;
}

GLint ProgramBinary::getActiveAttributeMaxLength() const
{
    int maxLength = 0;

    for (int attributeIndex = 0; attributeIndex < MAX_VERTEX_ATTRIBS; attributeIndex++)
    {
        if (!mLinkedAttribute[attributeIndex].name.empty())
        {
            maxLength = std::max((int)(mLinkedAttribute[attributeIndex].name.length() + 1), maxLength);
        }
    }

    return maxLength;
}

void ProgramBinary::getActiveUniform(GLuint index, GLsizei bufsize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) const
{
    ASSERT(index < mUniforms.size());   

    if (bufsize > 0)
    {
        std::string string = mUniforms[index]->name;

        if (mUniforms[index]->isArray())
        {
            string += "[0]";
        }

        strncpy(name, string.c_str(), bufsize);
        name[bufsize - 1] = '\0';

        if (length)
        {
            *length = strlen(name);
        }
    }

    *size = mUniforms[index]->elementCount();

    *type = mUniforms[index]->type;
}

GLint ProgramBinary::getActiveUniformCount() const
{
    return mUniforms.size();
}

GLint ProgramBinary::getActiveUniformMaxLength() const
{
    int maxLength = 0;

    unsigned int numUniforms = mUniforms.size();
    for (unsigned int uniformIndex = 0; uniformIndex < numUniforms; uniformIndex++)
    {
        if (!mUniforms[uniformIndex]->name.empty())
        {
            int length = (int)(mUniforms[uniformIndex]->name.length() + 1);
            if (mUniforms[uniformIndex]->isArray())
            {
                length += 3;  
            }
            maxLength = std::max(length, maxLength);
        }
    }

    return maxLength;
}

GLint ProgramBinary::getActiveUniformi(GLuint index, GLenum pname) const
{
    const gl::LinkedUniform& uniform = *mUniforms[index];

    switch (pname)
    {
      case GL_UNIFORM_TYPE:         return static_cast<GLint>(uniform.type);
      case GL_UNIFORM_SIZE:         return static_cast<GLint>(uniform.elementCount());
      case GL_UNIFORM_NAME_LENGTH:  return static_cast<GLint>(uniform.name.size() + 1 + (uniform.isArray() ? 3 : 0));
      case GL_UNIFORM_BLOCK_INDEX:  return uniform.blockIndex;

      case GL_UNIFORM_OFFSET:       return uniform.blockInfo.offset;
      case GL_UNIFORM_ARRAY_STRIDE: return uniform.blockInfo.arrayStride;
      case GL_UNIFORM_MATRIX_STRIDE: return uniform.blockInfo.matrixStride;
      case GL_UNIFORM_IS_ROW_MAJOR: return static_cast<GLint>(uniform.blockInfo.isRowMajorMatrix);

      default:
        UNREACHABLE();
        break;
    }
    return 0;
}

bool ProgramBinary::isValidUniformLocation(GLint location) const
{
    ASSERT(rx::IsIntegerCastSafe<GLint>(mUniformIndex.size()));
    return (location >= 0 && location < static_cast<GLint>(mUniformIndex.size()));
}

LinkedUniform *ProgramBinary::getUniformByLocation(GLint location) const
{
    ASSERT(location >= 0 && static_cast<size_t>(location) < mUniformIndex.size());
    return mUniforms[mUniformIndex[location].index];
}

LinkedUniform *ProgramBinary::getUniformByName(const std::string &name) const
{
    for (size_t uniformIndex = 0; uniformIndex < mUniforms.size(); uniformIndex++)
    {
        if (mUniforms[uniformIndex]->name == name)
        {
            return mUniforms[uniformIndex];
        }
    }

    return NULL;
}

void ProgramBinary::getActiveUniformBlockName(GLuint uniformBlockIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformBlockName) const
{
    ASSERT(uniformBlockIndex < mUniformBlocks.size());   

    const UniformBlock &uniformBlock = *mUniformBlocks[uniformBlockIndex];

    if (bufSize > 0)
    {
        std::string string = uniformBlock.name;

        if (uniformBlock.isArrayElement())
        {
            string += ArrayString(uniformBlock.elementIndex);
        }

        strncpy(uniformBlockName, string.c_str(), bufSize);
        uniformBlockName[bufSize - 1] = '\0';

        if (length)
        {
            *length = strlen(uniformBlockName);
        }
    }
}

void ProgramBinary::getActiveUniformBlockiv(GLuint uniformBlockIndex, GLenum pname, GLint *params) const
{
    ASSERT(uniformBlockIndex < mUniformBlocks.size());   

    const UniformBlock &uniformBlock = *mUniformBlocks[uniformBlockIndex];

    switch (pname)
    {
      case GL_UNIFORM_BLOCK_DATA_SIZE:
        *params = static_cast<GLint>(uniformBlock.dataSize);
        break;
      case GL_UNIFORM_BLOCK_NAME_LENGTH:
        *params = static_cast<GLint>(uniformBlock.name.size() + 1 + (uniformBlock.isArrayElement() ? 3 : 0));
        break;
      case GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS:
        *params = static_cast<GLint>(uniformBlock.memberUniformIndexes.size());
        break;
      case GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES:
        {
            for (unsigned int blockMemberIndex = 0; blockMemberIndex < uniformBlock.memberUniformIndexes.size(); blockMemberIndex++)
            {
                params[blockMemberIndex] = static_cast<GLint>(uniformBlock.memberUniformIndexes[blockMemberIndex]);
            }
        }
        break;
      case GL_UNIFORM_BLOCK_REFERENCED_BY_VERTEX_SHADER:
        *params = static_cast<GLint>(uniformBlock.isReferencedByVertexShader());
        break;
      case GL_UNIFORM_BLOCK_REFERENCED_BY_FRAGMENT_SHADER:
        *params = static_cast<GLint>(uniformBlock.isReferencedByFragmentShader());
        break;
      default: UNREACHABLE();
    }
}

GLuint ProgramBinary::getActiveUniformBlockCount() const
{
    return mUniformBlocks.size();
}

GLuint ProgramBinary::getActiveUniformBlockMaxLength() const
{
    unsigned int maxLength = 0;

    unsigned int numUniformBlocks = mUniformBlocks.size();
    for (unsigned int uniformBlockIndex = 0; uniformBlockIndex < numUniformBlocks; uniformBlockIndex++)
    {
        const UniformBlock &uniformBlock = *mUniformBlocks[uniformBlockIndex];
        if (!uniformBlock.name.empty())
        {
            const unsigned int length = uniformBlock.name.length() + 1;

            
            const unsigned int arrayLength = (uniformBlock.isArrayElement() ? 3 : 0);

            maxLength = std::max(length + arrayLength, maxLength);
        }
    }

    return maxLength;
}

void ProgramBinary::validate(InfoLog &infoLog, const Caps &caps)
{
    applyUniforms();
    if (!validateSamplers(&infoLog, caps))
    {
        mValidated = false;
    }
    else
    {
        mValidated = true;
    }
}

bool ProgramBinary::validateSamplers(InfoLog *infoLog, const Caps &caps)
{
    
    
    
    updateSamplerMapping();

    const unsigned int maxCombinedTextureImageUnits = caps.maxCombinedTextureImageUnits;
    TextureType textureUnitType[IMPLEMENTATION_MAX_COMBINED_TEXTURE_IMAGE_UNITS];

    for (unsigned int i = 0; i < IMPLEMENTATION_MAX_COMBINED_TEXTURE_IMAGE_UNITS; ++i)
    {
        textureUnitType[i] = TEXTURE_UNKNOWN;
    }

    for (unsigned int i = 0; i < mUsedPixelSamplerRange; ++i)
    {
        if (mSamplersPS[i].active)
        {
            unsigned int unit = mSamplersPS[i].logicalTextureUnit;

            if (unit >= maxCombinedTextureImageUnits)
            {
                if (infoLog)
                {
                    infoLog->append("Sampler uniform (%d) exceeds IMPLEMENTATION_MAX_COMBINED_TEXTURE_IMAGE_UNITS (%d)", unit, maxCombinedTextureImageUnits);
                }

                return false;
            }

            if (textureUnitType[unit] != TEXTURE_UNKNOWN)
            {
                if (mSamplersPS[i].textureType != textureUnitType[unit])
                {
                    if (infoLog)
                    {
                        infoLog->append("Samplers of conflicting types refer to the same texture image unit (%d).", unit);
                    }

                    return false;
                }
            }
            else
            {
                textureUnitType[unit] = mSamplersPS[i].textureType;
            }
        }
    }

    for (unsigned int i = 0; i < mUsedVertexSamplerRange; ++i)
    {
        if (mSamplersVS[i].active)
        {
            unsigned int unit = mSamplersVS[i].logicalTextureUnit;

            if (unit >= maxCombinedTextureImageUnits)
            {
                if (infoLog)
                {
                    infoLog->append("Sampler uniform (%d) exceeds IMPLEMENTATION_MAX_COMBINED_TEXTURE_IMAGE_UNITS (%d)", unit, maxCombinedTextureImageUnits);
                }

                return false;
            }

            if (textureUnitType[unit] != TEXTURE_UNKNOWN)
            {
                if (mSamplersVS[i].textureType != textureUnitType[unit])
                {
                    if (infoLog)
                    {
                        infoLog->append("Samplers of conflicting types refer to the same texture image unit (%d).", unit);
                    }

                    return false;
                }
            }
            else
            {
                textureUnitType[unit] = mSamplersVS[i].textureType;
            }
        }
    }

    return true;
}

ProgramBinary::Sampler::Sampler() : active(false), logicalTextureUnit(0), textureType(TEXTURE_2D)
{
}

struct AttributeSorter
{
    AttributeSorter(const int (&semanticIndices)[MAX_VERTEX_ATTRIBS])
        : originalIndices(semanticIndices)
    {
    }

    bool operator()(int a, int b)
    {
        if (originalIndices[a] == -1) return false;
        if (originalIndices[b] == -1) return true;
        return (originalIndices[a] < originalIndices[b]);
    }

    const int (&originalIndices)[MAX_VERTEX_ATTRIBS];
};

void ProgramBinary::initAttributesByLayout()
{
    for (int i = 0; i < MAX_VERTEX_ATTRIBS; i++)
    {
        mAttributesByLayout[i] = i;
    }

    std::sort(&mAttributesByLayout[0], &mAttributesByLayout[MAX_VERTEX_ATTRIBS], AttributeSorter(mSemanticIndex));
}

void ProgramBinary::sortAttributesByLayout(rx::TranslatedAttribute attributes[MAX_VERTEX_ATTRIBS], int sortedSemanticIndices[MAX_VERTEX_ATTRIBS]) const
{
    rx::TranslatedAttribute oldTranslatedAttributes[MAX_VERTEX_ATTRIBS];

    for (int i = 0; i < MAX_VERTEX_ATTRIBS; i++)
    {
        oldTranslatedAttributes[i] = attributes[i];
    }

    for (int i = 0; i < MAX_VERTEX_ATTRIBS; i++)
    {
        int oldIndex = mAttributesByLayout[i];
        sortedSemanticIndices[i] = mSemanticIndex[oldIndex];
        attributes[i] = oldTranslatedAttributes[oldIndex];
    }
}

void ProgramBinary::initializeUniformStorage()
{
    
    unsigned int vertexRegisters = 0;
    unsigned int fragmentRegisters = 0;
    for (size_t uniformIndex = 0; uniformIndex < mUniforms.size(); uniformIndex++)
    {
        const LinkedUniform &uniform = *mUniforms[uniformIndex];

        if (!IsSampler(uniform.type))
        {
            if (uniform.isReferencedByVertexShader())
            {
                vertexRegisters = std::max(vertexRegisters, uniform.vsRegisterIndex + uniform.registerCount);
            }
            if (uniform.isReferencedByFragmentShader())
            {
                fragmentRegisters = std::max(fragmentRegisters, uniform.psRegisterIndex + uniform.registerCount);
            }
        }
    }

    mVertexUniformStorage = mRenderer->createUniformStorage(vertexRegisters * 16u);
    mFragmentUniformStorage = mRenderer->createUniformStorage(fragmentRegisters * 16u);
}

void ProgramBinary::reset()
{
    mVertexHLSL.clear();
    mVertexWorkarounds = rx::ANGLE_D3D_WORKAROUND_NONE;
    SafeDeleteContainer(mVertexExecutables);

    mPixelHLSL.clear();
    mPixelWorkarounds = rx::ANGLE_D3D_WORKAROUND_NONE;
    mUsesFragDepth = false;
    mPixelShaderKey.clear();
    SafeDeleteContainer(mPixelExecutables);

    SafeDelete(mGeometryExecutable);

    mTransformFeedbackBufferMode = GL_NONE;
    mTransformFeedbackLinkedVaryings.clear();

    for (size_t i = 0; i < ArraySize(mSamplersPS); i++)
    {
        mSamplersPS[i] = Sampler();
    }
    for (size_t i = 0; i < ArraySize(mSamplersVS); i++)
    {
        mSamplersVS[i] = Sampler();
    }
    mUsedVertexSamplerRange = 0;
    mUsedPixelSamplerRange = 0;
    mUsesPointSize = false;
    mShaderVersion = 0;
    mDirtySamplerMapping = true;

    SafeDeleteContainer(mUniforms);
    SafeDeleteContainer(mUniformBlocks);
    mUniformIndex.clear();
    mOutputVariables.clear();
    SafeDelete(mVertexUniformStorage);
    SafeDelete(mFragmentUniformStorage);

    mValidated = false;
}

}
