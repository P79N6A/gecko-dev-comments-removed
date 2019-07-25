









#include "libGLESv2/Buffer.h"

#include "libGLESv2/main.h"
#include "libGLESv2/geometry/VertexDataManager.h"
#include "libGLESv2/geometry/IndexDataManager.h"

namespace gl
{

Buffer::Buffer(GLuint id) : RefCountObject(id)
{
    mContents = NULL;
    mSize = 0;
    mUsage = GL_DYNAMIC_DRAW;

    mVertexBuffer = NULL;
    mIndexBuffer = NULL;
}

Buffer::~Buffer()
{
    delete[] mContents;
    delete mVertexBuffer;
    delete mIndexBuffer;
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

    invalidateStaticData();

    if (usage == GL_STATIC_DRAW)
    {
        mVertexBuffer = new StaticVertexBuffer(getDevice());
        mIndexBuffer = new StaticIndexBuffer(getDevice());
    }
}

void Buffer::bufferSubData(const void *data, GLsizeiptr size, GLintptr offset)
{
    memcpy(mContents + offset, data, size);

    if ((mVertexBuffer && mVertexBuffer->size() != 0) || (mIndexBuffer && mIndexBuffer->size() != 0))
    {
        invalidateStaticData();

        if (mUsage == GL_STATIC_DRAW)
        {
            
            
            
        
        
        }
    }
}

StaticVertexBuffer *Buffer::getVertexBuffer()
{
    return mVertexBuffer;
}

StaticIndexBuffer *Buffer::getIndexBuffer()
{
    return mIndexBuffer;
}

void Buffer::invalidateStaticData()
{
    delete mVertexBuffer;
    mVertexBuffer = NULL;

    delete mIndexBuffer;
    mIndexBuffer = NULL;
}

}
