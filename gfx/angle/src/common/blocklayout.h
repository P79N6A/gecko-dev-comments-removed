








#ifndef COMMON_BLOCKLAYOUT_H_
#define COMMON_BLOCKLAYOUT_H_

#include <cstddef>
#include <vector>

#include "angle_gl.h"
#include <GLSLANG/ShaderLang.h>

namespace sh
{
struct ShaderVariable;
struct InterfaceBlockField;
struct Uniform;
struct Varying;
struct InterfaceBlock;

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

class BlockLayoutEncoder
{
  public:
    BlockLayoutEncoder();

    BlockMemberInfo encodeType(GLenum type, unsigned int arraySize, bool isRowMajorMatrix);

    size_t getBlockSize() const { return mCurrentOffset * BytesPerComponent; }
    size_t getCurrentRegister() const { return mCurrentOffset / ComponentsPerRegister; }
    size_t getCurrentElement() const { return mCurrentOffset % ComponentsPerRegister; }

    virtual void enterAggregateType() = 0;
    virtual void exitAggregateType() = 0;

    static const size_t BytesPerComponent = 4u;
    static const unsigned int ComponentsPerRegister = 4u;

  protected:
    size_t mCurrentOffset;

    void nextRegister();

    virtual void getBlockLayoutInfo(GLenum type, unsigned int arraySize, bool isRowMajorMatrix, int *arrayStrideOut, int *matrixStrideOut) = 0;
    virtual void advanceOffset(GLenum type, unsigned int arraySize, bool isRowMajorMatrix, int arrayStride, int matrixStride) = 0;
};




class Std140BlockEncoder : public BlockLayoutEncoder
{
  public:
    Std140BlockEncoder();

    virtual void enterAggregateType();
    virtual void exitAggregateType();

  protected:
    virtual void getBlockLayoutInfo(GLenum type, unsigned int arraySize, bool isRowMajorMatrix, int *arrayStrideOut, int *matrixStrideOut);
    virtual void advanceOffset(GLenum type, unsigned int arraySize, bool isRowMajorMatrix, int arrayStride, int matrixStride);
};






class HLSLBlockEncoder : public BlockLayoutEncoder
{
  public:
    enum HLSLBlockEncoderStrategy
    {
        ENCODE_PACKED,
        ENCODE_LOOSE
    };

    HLSLBlockEncoder(HLSLBlockEncoderStrategy strategy);

    virtual void enterAggregateType();
    virtual void exitAggregateType();
    void skipRegisters(unsigned int numRegisters);

    bool isPacked() const { return mEncoderStrategy == ENCODE_PACKED; }

    static HLSLBlockEncoderStrategy GetStrategyFor(ShShaderOutput outputType);

  protected:
    virtual void getBlockLayoutInfo(GLenum type, unsigned int arraySize, bool isRowMajorMatrix, int *arrayStrideOut, int *matrixStrideOut);
    virtual void advanceOffset(GLenum type, unsigned int arraySize, bool isRowMajorMatrix, int arrayStride, int matrixStride);

    HLSLBlockEncoderStrategy mEncoderStrategy;
};



unsigned int HLSLVariableRegisterCount(const Varying &variable);
unsigned int HLSLVariableRegisterCount(const Uniform &variable, ShShaderOutput outputType);

}

#endif 
