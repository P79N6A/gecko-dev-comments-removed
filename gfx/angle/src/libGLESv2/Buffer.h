









#ifndef LIBGLESV2_BUFFER_H_
#define LIBGLESV2_BUFFER_H_

#include "libGLESv2/Error.h"

#include "common/angleutils.h"
#include "common/RefCountObject.h"
#include "libGLESv2/renderer/IndexRangeCache.h"

namespace rx
{
class Renderer;
class BufferImpl;
};

namespace gl
{

class Buffer : public RefCountObject
{
  public:
    Buffer(rx::BufferImpl *impl, GLuint id);

    virtual ~Buffer();

    Error bufferData(const void *data, GLsizeiptr size, GLenum usage);
    Error bufferSubData(const void *data, GLsizeiptr size, GLintptr offset);
    Error copyBufferSubData(Buffer* source, GLintptr sourceOffset, GLintptr destOffset, GLsizeiptr size);
    Error mapRange(GLintptr offset, GLsizeiptr length, GLbitfield access);
    Error unmap();

    GLenum getUsage() const { return mUsage; }
    GLint getAccessFlags() const {  return mAccessFlags; }
    GLboolean isMapped() const { return mMapped; }
    GLvoid *getMapPointer() const { return mMapPointer; }
    GLint64 getMapOffset() const { return mMapOffset; }
    GLint64 getMapLength() const { return mMapLength; }
    GLint64 getSize() const { return mSize; }

    rx::BufferImpl *getImplementation() const { return mBuffer; }

    void markTransformFeedbackUsage();

    rx::IndexRangeCache *getIndexRangeCache() { return &mIndexRangeCache; }
    const rx::IndexRangeCache *getIndexRangeCache() const { return &mIndexRangeCache; }

  private:
    DISALLOW_COPY_AND_ASSIGN(Buffer);

    rx::BufferImpl *mBuffer;

    GLenum mUsage;
    GLint64 mSize;
    GLint mAccessFlags;
    GLboolean mMapped;
    GLvoid *mMapPointer;
    GLint64 mMapOffset;
    GLint64 mMapLength;

    rx::IndexRangeCache mIndexRangeCache;
};

}

#endif   
