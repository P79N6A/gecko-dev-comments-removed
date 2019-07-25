








#include "libGLESv2/HandleAllocator.h"

#include "libGLESv2/main.h"

namespace gl
{

HandleAllocator::HandleAllocator() : mBaseValue(1), mNextValue(1)
{
}

HandleAllocator::~HandleAllocator()
{
}

void HandleAllocator::setBaseHandle(GLuint value)
{
    ASSERT(mBaseValue == mNextValue);
    mBaseValue = value;
    mNextValue = value;
}

GLuint HandleAllocator::allocate()
{
    if (mFreeValues.size())
    {
        GLuint handle = mFreeValues.back();
        mFreeValues.pop_back();
        return handle;
    }
    return mNextValue++;
}

void HandleAllocator::release(GLuint handle)
{
    if (handle == mNextValue - 1)
    {
        
        if(mNextValue > mBaseValue)
        {
            mNextValue--;
        }
    }
    else
    {
        
        if (handle >= mBaseValue)
        {
            mFreeValues.push_back(handle);
        }
    }
}

}
