








#ifndef LIBGLESV2_RENDERER_SHADEREXECUTABLE_H_
#define LIBGLESV2_RENDERER_SHADEREXECUTABLE_H_

#include "common/angleutils.h"

namespace rx
{

class ShaderExecutable
{
  public:
    ShaderExecutable(const void *function, size_t length) : mLength(length)
    {
        mFunction = new char[length];
        memcpy(mFunction, function, length);
    }
    
    virtual ~ShaderExecutable()
    {
        delete[] mFunction;
    }

    void *getFunction() const
    {
        return mFunction;
    }

    size_t getLength() const
    {
        return mLength;
    }

  private:
    DISALLOW_COPY_AND_ASSIGN(ShaderExecutable);

    void *mFunction;
    const size_t mLength;
};

}

#endif 
