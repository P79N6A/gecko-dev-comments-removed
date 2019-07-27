








#ifndef COMMON_SHADERVARIABLE_H_
#define COMMON_SHADERVARIABLE_H_

#include <string>
#include <vector>
#include <algorithm>
#include "GLSLANG/ShaderLang.h"

namespace sh
{


typedef unsigned int GLenum;


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
    ShaderVariable(GLenum typeIn, GLenum precisionIn, const char *nameIn, unsigned int arraySizeIn)
        : type(typeIn),
          precision(precisionIn),
          name(nameIn),
          arraySize(arraySizeIn)
    {}

    bool isArray() const { return arraySize > 0; }
    unsigned int elementCount() const { return std::max(1u, arraySize); }

    GLenum type;
    GLenum precision;
    std::string name;
    unsigned int arraySize;
};


struct Uniform : public ShaderVariable
{
    Uniform(GLenum typeIn, GLenum precisionIn, const char *nameIn, unsigned int arraySizeIn,
            unsigned int registerIndexIn, unsigned int elementIndexIn)
        : ShaderVariable(typeIn, precisionIn, nameIn, arraySizeIn),
          registerIndex(registerIndexIn),
          elementIndex(elementIndexIn)
    {}

    bool isStruct() const { return !fields.empty(); }

    std::vector<Uniform> fields;

    
    unsigned int registerIndex;
    unsigned int elementIndex; 
};

struct Attribute : public ShaderVariable
{
    Attribute()
        : ShaderVariable((GLenum)0, (GLenum)0, "", 0),
          location(-1)
    {}

    Attribute(GLenum typeIn, GLenum precisionIn, const char *nameIn, unsigned int arraySizeIn, int locationIn)
      : ShaderVariable(typeIn, precisionIn, nameIn, arraySizeIn),
        location(locationIn)
    {}

    int location;
};

struct InterfaceBlockField : public ShaderVariable
{
    InterfaceBlockField(GLenum typeIn, GLenum precisionIn, const char *nameIn, unsigned int arraySizeIn, bool isRowMajorMatrix)
        : ShaderVariable(typeIn, precisionIn, nameIn, arraySizeIn),
          isRowMajorMatrix(isRowMajorMatrix)
    {}

    bool isStruct() const { return !fields.empty(); }

    bool isRowMajorMatrix;
    std::vector<InterfaceBlockField> fields;
};

struct Varying : public ShaderVariable
{
    Varying(GLenum typeIn, GLenum precisionIn, const char *nameIn, unsigned int arraySizeIn, InterpolationType interpolationIn)
        : ShaderVariable(typeIn, precisionIn, nameIn, arraySizeIn),
          interpolation(interpolationIn)
    {}

    bool isStruct() const { return !fields.empty(); }

    InterpolationType interpolation;
    std::vector<Varying> fields;
    std::string structName;
};

struct BlockMemberInfo
{
    BlockMemberInfo(int offset, int arrayStride, int matrixStride, bool isRowMajorMatrix)
        : offset(offset),
          arrayStride(arrayStride),
          matrixStride(matrixStride),
          isRowMajorMatrix(isRowMajorMatrix)
    {}

    static BlockMemberInfo getDefaultBlockInfo()
    {
        return BlockMemberInfo(-1, -1, -1, false);
    }

    int offset;
    int arrayStride;
    int matrixStride;
    bool isRowMajorMatrix;
};

typedef std::vector<BlockMemberInfo> BlockMemberInfoArray;

struct InterfaceBlock
{
    InterfaceBlock(const char *name, unsigned int arraySize, unsigned int registerIndex)
        : name(name),
          arraySize(arraySize),
          layout(BLOCKLAYOUT_SHARED),
          registerIndex(registerIndex),
          isRowMajorLayout(false)
    {}

    std::string name;
    unsigned int arraySize;
    size_t dataSize;
    BlockLayoutType layout;
    bool isRowMajorLayout;
    std::vector<InterfaceBlockField> fields;
    std::vector<BlockMemberInfo> blockInfo;

    
    unsigned int registerIndex;
};

}

#endif 
