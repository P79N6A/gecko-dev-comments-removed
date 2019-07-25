








#include "libGLESv2/geometry/backend.h"

#include "common/debug.h"

namespace gl
{

void *TranslatedBuffer::map(std::size_t requiredSpace, std::size_t *offset)
{
    ASSERT(requiredSpace <= mBufferSize);

    reserveSpace(requiredSpace);

    *offset = mCurrentPoint;
    mCurrentPoint += requiredSpace;

    return streamingMap(*offset, requiredSpace);
}

void TranslatedBuffer::reserveSpace(std::size_t requiredSpace)
{
    if (mCurrentPoint + requiredSpace > mBufferSize)
    {
        recycle();
        mCurrentPoint = 0;
    }
}

}
