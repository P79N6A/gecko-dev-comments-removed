








#include "common/blocklayout.h"
#include "common/mathutil.h"
#include "common/utilities.h"

namespace sh
{

BlockLayoutEncoder::BlockLayoutEncoder()
    : mCurrentOffset(0)
{
}

BlockMemberInfo BlockLayoutEncoder::encodeType(GLenum type, unsigned int arraySize, bool isRowMajorMatrix)
{
    int arrayStride;
    int matrixStride;

    getBlockLayoutInfo(type, arraySize, isRowMajorMatrix, &arrayStride, &matrixStride);

    const BlockMemberInfo memberInfo(mCurrentOffset * BytesPerComponent, arrayStride * BytesPerComponent, matrixStride * BytesPerComponent, isRowMajorMatrix);

    advanceOffset(type, arraySize, isRowMajorMatrix, arrayStride, matrixStride);

    return memberInfo;
}

void BlockLayoutEncoder::nextRegister()
{
    mCurrentOffset = rx::roundUp<size_t>(mCurrentOffset, ComponentsPerRegister);
}

Std140BlockEncoder::Std140BlockEncoder()
{
}

void Std140BlockEncoder::enterAggregateType()
{
    nextRegister();
}

void Std140BlockEncoder::exitAggregateType()
{
    nextRegister();
}

void Std140BlockEncoder::getBlockLayoutInfo(GLenum type, unsigned int arraySize, bool isRowMajorMatrix, int *arrayStrideOut, int *matrixStrideOut)
{
    
    ASSERT(gl::VariableComponentSize(gl::VariableComponentType(type)) == BytesPerComponent);

    size_t baseAlignment = 0;
    int matrixStride = 0;
    int arrayStride = 0;

    if (gl::IsMatrixType(type))
    {
        baseAlignment = ComponentsPerRegister;
        matrixStride = ComponentsPerRegister;

        if (arraySize > 0)
        {
            const int numRegisters = gl::MatrixRegisterCount(type, isRowMajorMatrix);
            arrayStride = ComponentsPerRegister * numRegisters;
        }
    }
    else if (arraySize > 0)
    {
        baseAlignment = ComponentsPerRegister;
        arrayStride = ComponentsPerRegister;
    }
    else
    {
        const int numComponents = gl::VariableComponentCount(type);
        baseAlignment = (numComponents == 3 ? 4u : static_cast<size_t>(numComponents));
    }

    mCurrentOffset = rx::roundUp(mCurrentOffset, baseAlignment);

    *matrixStrideOut = matrixStride;
    *arrayStrideOut = arrayStride;
}

void Std140BlockEncoder::advanceOffset(GLenum type, unsigned int arraySize, bool isRowMajorMatrix, int arrayStride, int matrixStride)
{
    if (arraySize > 0)
    {
        mCurrentOffset += arrayStride * arraySize;
    }
    else if (gl::IsMatrixType(type))
    {
        ASSERT(matrixStride == ComponentsPerRegister);
        const int numRegisters = gl::MatrixRegisterCount(type, isRowMajorMatrix);
        mCurrentOffset += ComponentsPerRegister * numRegisters;
    }
    else
    {
        mCurrentOffset += gl::VariableComponentCount(type);
    }
}

HLSLBlockEncoder::HLSLBlockEncoder(HLSLBlockEncoderStrategy strategy)
    : mEncoderStrategy(strategy)
{
}

void HLSLBlockEncoder::enterAggregateType()
{
    nextRegister();
}

void HLSLBlockEncoder::exitAggregateType()
{
}

void HLSLBlockEncoder::getBlockLayoutInfo(GLenum type, unsigned int arraySize, bool isRowMajorMatrix, int *arrayStrideOut, int *matrixStrideOut)
{
    
    ASSERT(gl::VariableComponentSize(gl::VariableComponentType(type)) == BytesPerComponent);

    int matrixStride = 0;
    int arrayStride = 0;

    
    
    
    if (!isPacked() ||
        gl::IsMatrixType(type) ||
        arraySize > 0)
    {
        nextRegister();
    }

    if (gl::IsMatrixType(type))
    {
        matrixStride = ComponentsPerRegister;

        if (arraySize > 0)
        {
            const int numRegisters = gl::MatrixRegisterCount(type, isRowMajorMatrix);
            arrayStride = ComponentsPerRegister * numRegisters;
        }
    }
    else if (arraySize > 0)
    {
        arrayStride = ComponentsPerRegister;
    }
    else if (isPacked())
    {
        int numComponents = gl::VariableComponentCount(type);
        if ((numComponents + (mCurrentOffset % ComponentsPerRegister)) > ComponentsPerRegister)
        {
            nextRegister();
        }
    }

    *matrixStrideOut = matrixStride;
    *arrayStrideOut = arrayStride;
}

void HLSLBlockEncoder::advanceOffset(GLenum type, unsigned int arraySize, bool isRowMajorMatrix, int arrayStride, int matrixStride)
{
    if (arraySize > 0)
    {
        mCurrentOffset += arrayStride * (arraySize - 1);
    }

    if (gl::IsMatrixType(type))
    {
        ASSERT(matrixStride == ComponentsPerRegister);
        const int numRegisters = gl::MatrixRegisterCount(type, isRowMajorMatrix);
        const int numComponents = gl::MatrixComponentCount(type, isRowMajorMatrix);
        mCurrentOffset += ComponentsPerRegister * (numRegisters - 1);
        mCurrentOffset += numComponents;
    }
    else if (isPacked())
    {
        mCurrentOffset += gl::VariableComponentCount(type);
    }
    else
    {
        mCurrentOffset += ComponentsPerRegister;
    }
}

void HLSLBlockEncoder::skipRegisters(unsigned int numRegisters)
{
    mCurrentOffset += (numRegisters * ComponentsPerRegister);
}

HLSLBlockEncoder::HLSLBlockEncoderStrategy HLSLBlockEncoder::GetStrategyFor(ShShaderOutput outputType)
{
    switch (outputType)
    {
      case SH_HLSL9_OUTPUT: return ENCODE_LOOSE;
      case SH_HLSL11_OUTPUT: return ENCODE_PACKED;
      default: UNREACHABLE(); return ENCODE_PACKED;
    }
}

template <class ShaderVarType>
void HLSLVariableRegisterCount(const ShaderVarType &variable, HLSLBlockEncoder *encoder)
{
    if (variable.isStruct())
    {
        for (size_t arrayElement = 0; arrayElement < variable.elementCount(); arrayElement++)
        {
            encoder->enterAggregateType();

            for (size_t fieldIndex = 0; fieldIndex < variable.fields.size(); fieldIndex++)
            {
                HLSLVariableRegisterCount(variable.fields[fieldIndex], encoder);
            }

            encoder->exitAggregateType();
        }
    }
    else
    {
        
        encoder->encodeType(variable.type, variable.arraySize, false);
    }
}

unsigned int HLSLVariableRegisterCount(const Varying &variable)
{
    HLSLBlockEncoder encoder(HLSLBlockEncoder::ENCODE_PACKED);
    HLSLVariableRegisterCount(variable, &encoder);

    const size_t registerBytes = (encoder.BytesPerComponent * encoder.ComponentsPerRegister);
    return static_cast<unsigned int>(rx::roundUp<size_t>(encoder.getBlockSize(), registerBytes) / registerBytes);
}

unsigned int HLSLVariableRegisterCount(const Uniform &variable, ShShaderOutput outputType)
{
    HLSLBlockEncoder encoder(HLSLBlockEncoder::GetStrategyFor(outputType));
    HLSLVariableRegisterCount(variable, &encoder);

    const size_t registerBytes = (encoder.BytesPerComponent * encoder.ComponentsPerRegister);
    return static_cast<unsigned int>(rx::roundUp<size_t>(encoder.getBlockSize(), registerBytes) / registerBytes);
}

}
