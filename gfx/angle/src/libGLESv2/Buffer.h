









#ifndef LIBGLESV2_BUFFER_H_
#define LIBGLESV2_BUFFER_H_

#include <cstddef>
#include <vector>

#define GL_APICALL
#include <GLES2/gl2.h>

#include "common/angleutils.h"
#include "libGLESv2/RefCountObject.h"

namespace gl
{
class StaticVertexBuffer;
class StaticIndexBuffer;

class Buffer : public RefCountObject
{
  public:
    explicit Buffer(GLuint id);

    virtual ~Buffer();

    void bufferData(const void *data, GLsizeiptr size, GLenum usage);
    void bufferSubData(const void *data, GLsizeiptr size, GLintptr offset);

    void *data() { return mContents; }
    size_t size() const { return mSize; }
    GLenum usage() const { return mUsage; }

    StaticVertexBuffer *getStaticVertexBuffer();
    StaticIndexBuffer *getStaticIndexBuffer();
    void invalidateStaticData();
    void promoteStaticUsage(int dataSize);

  private:
    DISALLOW_COPY_AND_ASSIGN(Buffer);

    GLubyte *mContents;
    size_t mSize;
    GLenum mUsage;

    StaticVertexBuffer *mStaticVertexBuffer;
    StaticIndexBuffer *mStaticIndexBuffer;
    size_t mUnmodifiedDataUse;
};

}

#endif   
