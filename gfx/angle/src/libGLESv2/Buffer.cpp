









#include "libGLESv2/Buffer.h"
#include "libGLESv2/renderer/BufferImpl.h"
#include "libGLESv2/renderer/Renderer.h"

namespace gl
{

Buffer::Buffer(rx::BufferImpl *impl, GLuint id)
    : RefCountObject(id),
      mBuffer(impl),
      mUsage(GL_DYNAMIC_DRAW),
      mSize(0),
      mAccessFlags(0),
      mMapped(GL_FALSE),
      mMapPointer(NULL),
      mMapOffset(0),
      mMapLength(0)
{
}

Buffer::~Buffer()
{
    SafeDelete(mBuffer);
}

Error Buffer::bufferData(const void *data, GLsizeiptr size, GLenum usage)
{
    gl::Error error = mBuffer->setData(data, size, usage);
    if (error.isError())
    {
        return error;
    }

    mIndexRangeCache.clear();
    mUsage = usage;
    mSize = size;

    return error;
}

Error Buffer::bufferSubData(const void *data, GLsizeiptr size, GLintptr offset)
{
    gl::Error error = mBuffer->setSubData(data, size, offset);
    if (error.isError())
    {
        return error;
    }

    mIndexRangeCache.invalidateRange(offset, size);

    return error;
}

Error Buffer::copyBufferSubData(Buffer* source, GLintptr sourceOffset, GLintptr destOffset, GLsizeiptr size)
{
    gl::Error error = mBuffer->copySubData(source->getImplementation(), sourceOffset, destOffset, size);
    if (error.isError())
    {
        return error;
    }

    mIndexRangeCache.invalidateRange(destOffset, size);

    return error;
}

Error Buffer::mapRange(GLintptr offset, GLsizeiptr length, GLbitfield access)
{
    ASSERT(!mMapped);
    ASSERT(offset + length <= mSize);

    Error error = mBuffer->map(offset, length, access, &mMapPointer);
    if (error.isError())
    {
        mMapPointer = NULL;
        return error;
    }

    mMapped = GL_TRUE;
    mMapOffset = static_cast<GLint64>(offset);
    mMapLength = static_cast<GLint64>(length);
    mAccessFlags = static_cast<GLint>(access);

    if ((access & GL_MAP_WRITE_BIT) > 0)
    {
        mIndexRangeCache.invalidateRange(offset, length);
    }

    return error;
}

Error Buffer::unmap()
{
    ASSERT(mMapped);

    Error error = mBuffer->unmap();
    if (error.isError())
    {
        return error;
    }

    mMapped = GL_FALSE;
    mMapPointer = NULL;
    mMapOffset = 0;
    mMapLength = 0;
    mAccessFlags = 0;

    return error;
}

void Buffer::markTransformFeedbackUsage()
{
    
    mBuffer->markTransformFeedbackUsage();
    mIndexRangeCache.clear();
}

}
