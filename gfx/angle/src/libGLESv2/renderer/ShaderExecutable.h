








#ifndef LIBGLESV2_RENDERER_SHADEREXECUTABLE_H_
#define LIBGLESV2_RENDERER_SHADEREXECUTABLE_H_

#include "common/angleutils.h"
#include "common/debug.h"

#include <vector>
#include <cstdint>

namespace rx
{

class ShaderExecutable
{
  public:
    ShaderExecutable(const void *function, size_t length)
        : mFunctionBuffer(length)
    {
        memcpy(mFunctionBuffer.data(), function, length);
    }

    virtual ~ShaderExecutable() {}

    const uint8_t *getFunction() const
    {
        return mFunctionBuffer.data();
    }

    size_t getLength() const
    {
        return mFunctionBuffer.size();
    }

  private:
    DISALLOW_COPY_AND_ASSIGN(ShaderExecutable);

    std::vector<uint8_t> mFunctionBuffer;
};

class UniformStorage
{
  public:
    UniformStorage(size_t initialSize)
        : mSize(initialSize)
    {
    }

    virtual ~UniformStorage() {}

    size_t size() const { return mSize; }

  private:
    size_t mSize;
};

}

#endif 
