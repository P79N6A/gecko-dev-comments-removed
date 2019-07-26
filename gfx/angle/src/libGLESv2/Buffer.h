









#ifndef LIBGLESV2_BUFFER_H_
#define LIBGLESV2_BUFFER_H_

#include "common/angleutils.h"
#include "common/RefCountObject.h"
#include "libGLESv2/renderer/IndexRangeCache.h"

namespace rx
{
class Renderer;
class BufferStorage;
class StaticIndexBufferInterface;
class StaticVertexBufferInterface;
};

namespace gl
{

class Buffer : public RefCountObject
{
  public:
    Buffer(rx::Renderer *renderer, GLuint id);

    virtual ~Buffer();

    void bufferData(const void *data, GLsizeiptr size, GLenum usage);
    void bufferSubData(const void *data, GLsizeiptr size, GLintptr offset);

    GLenum usage() const;

    rx::BufferStorage *getStorage() const;
    unsigned int size();

    rx::StaticVertexBufferInterface *getStaticVertexBuffer();
    rx::StaticIndexBufferInterface *getStaticIndexBuffer();
    void invalidateStaticData();
    void promoteStaticUsage(int dataSize);

    rx::IndexRangeCache *getIndexRangeCache();

  private:
    DISALLOW_COPY_AND_ASSIGN(Buffer);

    rx::Renderer *mRenderer;
    GLenum mUsage;

    rx::BufferStorage *mBufferStorage;

    rx::IndexRangeCache mIndexRangeCache;

    rx::StaticVertexBufferInterface *mStaticVertexBuffer;
    rx::StaticIndexBufferInterface *mStaticIndexBuffer;
    unsigned int mUnmodifiedDataUse;
};

}

#endif   
