





#include "libGLESv2/renderer/d3d/MemoryBuffer.h"

#include <algorithm>
#include <cstdlib>

namespace rx
{

MemoryBuffer::MemoryBuffer()
    : mSize(0),
      mData(NULL)
{
}

MemoryBuffer::~MemoryBuffer()
{
    free(mData);
    mData = NULL;
}

bool MemoryBuffer::resize(size_t size)
{
    if (size == 0)
    {
        clear();
    }
    else
    {
        uint8_t *newMemory = reinterpret_cast<uint8_t*>(malloc(sizeof(uint8_t) * size));
        if (newMemory == NULL)
        {
            return false;
        }

        if (mData)
        {
            
            std::copy(mData, mData + std::min(mSize, size), newMemory);
            free(mData);
        }

        mData = newMemory;
        mSize = size;
    }

    return true;
}

size_t MemoryBuffer::size() const
{
    return mSize;
}

const uint8_t *MemoryBuffer::data() const
{
    return mData;
}

uint8_t *MemoryBuffer::data()
{
    return mData;
}

void MemoryBuffer::clear()
{
    free(mData);
    mData = NULL;
    mSize = 0;
}

}
