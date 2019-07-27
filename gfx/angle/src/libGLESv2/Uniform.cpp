#include "precompiled.h"






#include "libGLESv2/Uniform.h"

#include "common/utilities.h"

namespace gl
{

LinkedUniform::LinkedUniform(GLenum type, GLenum precision, const std::string &name, unsigned int arraySize,
                             const int blockIndex, const sh::BlockMemberInfo &blockInfo)
    : type(type),
      precision(precision),
      name(name),
      arraySize(arraySize),
      blockIndex(blockIndex),
      blockInfo(blockInfo),
      data(NULL),
      dirty(true),
      psRegisterIndex(GL_INVALID_INDEX),
      vsRegisterIndex(GL_INVALID_INDEX),
      registerCount(0),
      registerElement(0)
{
    
    
    if (isInDefaultBlock())
    {
        size_t bytes = dataSize();
        data = new unsigned char[bytes];
        memset(data, 0, bytes);
        registerCount = VariableRowCount(type) * elementCount();
    }
}

LinkedUniform::~LinkedUniform()
{
    delete[] data;
}

bool LinkedUniform::isArray() const
{
    return arraySize > 0;
}

unsigned int LinkedUniform::elementCount() const
{
    return arraySize > 0 ? arraySize : 1;
}

bool LinkedUniform::isReferencedByVertexShader() const
{
    return vsRegisterIndex != GL_INVALID_INDEX;
}

bool LinkedUniform::isReferencedByFragmentShader() const
{
    return psRegisterIndex != GL_INVALID_INDEX;
}

bool LinkedUniform::isInDefaultBlock() const
{
    return blockIndex == -1;
}

size_t LinkedUniform::dataSize() const
{
    ASSERT(type != GL_STRUCT_ANGLEX);
    return VariableInternalSize(type) * elementCount();
}

bool LinkedUniform::isSampler() const
{
    return IsSampler(type);
}

UniformBlock::UniformBlock(const std::string &name, unsigned int elementIndex, unsigned int dataSize)
    : name(name),
      elementIndex(elementIndex),
      dataSize(dataSize),
      psRegisterIndex(GL_INVALID_INDEX),
      vsRegisterIndex(GL_INVALID_INDEX)
{
}

bool UniformBlock::isArrayElement() const
{
    return elementIndex != GL_INVALID_INDEX;
}

bool UniformBlock::isReferencedByVertexShader() const
{
    return vsRegisterIndex != GL_INVALID_INDEX;
}

bool UniformBlock::isReferencedByFragmentShader() const
{
    return psRegisterIndex != GL_INVALID_INDEX;
}

}
