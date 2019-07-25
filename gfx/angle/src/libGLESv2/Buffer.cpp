









#include "libGLESv2/Buffer.h"

namespace gl
{

Buffer::Buffer(GLuint id) : RefCountObject(id)
{
    mContents = NULL;
    mSize = 0;
    mUsage = GL_DYNAMIC_DRAW;
}

Buffer::~Buffer()
{
    delete[] mContents;
}

void Buffer::bufferData(const void *data, GLsizeiptr size, GLenum usage)
{
    if (size == 0)
    {
        delete[] mContents;
        mContents = NULL;
    }
    else if (size != mSize)
    {
        delete[] mContents;
        mContents = new GLubyte[size];
        memset(mContents, 0, size);
    }

    if (data != NULL && size > 0)
    {
        memcpy(mContents, data, size);
    }

    mSize = size;
    mUsage = usage;
}

void Buffer::bufferSubData(const void *data, GLsizeiptr size, GLintptr offset)
{
    memcpy(mContents + offset, data, size);
}

}
