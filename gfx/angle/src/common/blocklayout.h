








#ifndef COMMON_BLOCKLAYOUT_H_
#define COMMON_BLOCKLAYOUT_H_

#include <vector>
#include "angle_gl.h"
#include <GLSLANG/ShaderLang.h>
#include <cstddef>

namespace sh
{
struct ShaderVariable;
struct InterfaceBlockField;
struct BlockMemberInfo;
struct Uniform;
struct Varying;

class BlockLayoutEncoder
{
  public:
    BlockLayoutEncoder(std::vector<BlockMemberInfo> *blockInfoOut);

    void encodeInterfaceBlockFields(const std::vector<InterfaceBlockField> &fields);
    void encodeInterfaceBlockField(const InterfaceBlockField &field);
    void encodeType(GLenum type, unsigned int arraySize, bool isRowMajorMatrix);
    size_t getBlockSize() const { return mCurrentOffset * BytesPerComponent; }

    static const size_t BytesPerComponent = 4u;
    static const unsigned int ComponentsPerRegister = 4u;

  protected:
    size_t mCurrentOffset;

    void nextRegister();

    virtual void enterAggregateType() = 0;
    virtual void exitAggregateType() = 0;
    virtual void getBlockLayoutInfo(GLenum type, unsigned int arraySize, bool isRowMajorMatrix, int *arrayStrideOut, int *matrixStrideOut) = 0;
    virtual void advanceOffset(GLenum type, unsigned int arraySize, bool isRowMajorMatrix, int arrayStride, int matrixStride) = 0;

  private:
    std::vector<BlockMemberInfo> *mBlockInfoOut;
};




class Std140BlockEncoder : public BlockLayoutEncoder
{
  public:
    Std140BlockEncoder(std::vector<BlockMemberInfo> *blockInfoOut);

  protected:
    virtual void enterAggregateType();
    virtual void exitAggregateType();
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

    HLSLBlockEncoder(std::vector<BlockMemberInfo> *blockInfoOut,
                     HLSLBlockEncoderStrategy strategy);

    virtual void enterAggregateType();
    virtual void exitAggregateType();
    void skipRegisters(unsigned int numRegisters);

    bool isPacked() const { return mEncoderStrategy == ENCODE_PACKED; }

  protected:
    virtual void getBlockLayoutInfo(GLenum type, unsigned int arraySize, bool isRowMajorMatrix, int *arrayStrideOut, int *matrixStrideOut);
    virtual void advanceOffset(GLenum type, unsigned int arraySize, bool isRowMajorMatrix, int arrayStride, int matrixStride);

    HLSLBlockEncoderStrategy mEncoderStrategy;
};



void HLSLVariableGetRegisterInfo(unsigned int baseRegisterIndex, Uniform *variable, ShShaderOutput outputType);



unsigned int HLSLVariableRegisterCount(const Varying &variable);
unsigned int HLSLVariableRegisterCount(const Uniform &variable, ShShaderOutput outputType);

}

#endif
