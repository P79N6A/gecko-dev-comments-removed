








#ifndef COMMON_SHADERVARIABLE_H_
#define COMMON_SHADERVARIABLE_H_

#include <string>
#include <vector>
#include <algorithm>

#include <GLES3/gl3.h>
#include <GLES2/gl2.h>

namespace gl
{


enum InterpolationType
{
    INTERPOLATION_SMOOTH,
    INTERPOLATION_CENTROID,
    INTERPOLATION_FLAT
};


enum BlockLayoutType
{
    BLOCKLAYOUT_STANDARD,
    BLOCKLAYOUT_PACKED,
    BLOCKLAYOUT_SHARED
};


struct ShaderVariable
{
    GLenum type;
    GLenum precision;
    std::string name;
    unsigned int arraySize;

    ShaderVariable(GLenum typeIn, GLenum precisionIn, const char *nameIn, unsigned int arraySizeIn)
      : type(typeIn),
        precision(precisionIn),
        name(nameIn),
        arraySize(arraySizeIn)
    {}

    bool isArray() const { return arraySize > 0; }
    unsigned int elementCount() const { return std::max(1u, arraySize); }
};


struct Uniform : public ShaderVariable
{
    unsigned int registerIndex;
    unsigned int elementIndex; 
    std::vector<Uniform> fields;

    Uniform(GLenum typeIn, GLenum precisionIn, const char *nameIn, unsigned int arraySizeIn,
            unsigned int registerIndexIn, unsigned int elementIndexIn)
      : ShaderVariable(typeIn, precisionIn, nameIn, arraySizeIn),
        registerIndex(registerIndexIn),
        elementIndex(elementIndexIn)
    {}

    bool isStruct() const { return !fields.empty(); }
};

struct Attribute : public ShaderVariable
{
    int location;

    Attribute()
      : ShaderVariable(GL_NONE, GL_NONE, "", 0),
        location(-1)
    {}

    Attribute(GLenum typeIn, GLenum precisionIn, const char *nameIn, unsigned int arraySizeIn, int locationIn)
      : ShaderVariable(typeIn, precisionIn, nameIn, arraySizeIn),
        location(locationIn)
    {}
};

struct InterfaceBlockField : public ShaderVariable
{
    bool isRowMajorMatrix;
    std::vector<InterfaceBlockField> fields;

    InterfaceBlockField(GLenum typeIn, GLenum precisionIn, const char *nameIn, unsigned int arraySizeIn, bool isRowMajorMatrix)
      : ShaderVariable(typeIn, precisionIn, nameIn, arraySizeIn),
        isRowMajorMatrix(isRowMajorMatrix)
    {}

    bool isStruct() const { return !fields.empty(); }
};

struct Varying : public ShaderVariable
{
    InterpolationType interpolation;
    std::vector<Varying> fields;
    std::string structName;

    Varying(GLenum typeIn, GLenum precisionIn, const char *nameIn, unsigned int arraySizeIn, InterpolationType interpolationIn)
      : ShaderVariable(typeIn, precisionIn, nameIn, arraySizeIn),
        interpolation(interpolationIn)
    {}

    bool isStruct() const { return !fields.empty(); }
};

struct BlockMemberInfo
{
    int offset;
    int arrayStride;
    int matrixStride;
    bool isRowMajorMatrix;

    static BlockMemberInfo getDefaultBlockInfo()
    {
        return BlockMemberInfo(-1, -1, -1, false);
    }

    BlockMemberInfo(int offset, int arrayStride, int matrixStride, bool isRowMajorMatrix)
      : offset(offset),
        arrayStride(arrayStride),
        matrixStride(matrixStride),
        isRowMajorMatrix(isRowMajorMatrix)
    {}
};

typedef std::vector<BlockMemberInfo> BlockMemberInfoArray;

struct InterfaceBlock
{
    std::string name;
    unsigned int arraySize;
    size_t dataSize;
    BlockLayoutType layout;
    bool isRowMajorLayout;
    std::vector<InterfaceBlockField> fields;
    std::vector<BlockMemberInfo> blockInfo;

    unsigned int registerIndex;

    InterfaceBlock(const char *name, unsigned int arraySize, unsigned int registerIndex)
      : name(name),
        arraySize(arraySize),
        layout(BLOCKLAYOUT_SHARED),
        registerIndex(registerIndex),
        isRowMajorLayout(false)
    {}
};

}

#endif 
