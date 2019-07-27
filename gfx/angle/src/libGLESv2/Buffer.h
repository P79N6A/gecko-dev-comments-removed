









#ifndef LIBGLESV2_BUFFER_H_
#define LIBGLESV2_BUFFER_H_

#include "common/angleutils.h"
#include "common/RefCountObject.h"

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

    void bufferData(const void *data, GLsizeiptr size, GLenum usage);
    void bufferSubData(const void *data, GLsizeiptr size, GLintptr offset);
    void copyBufferSubData(Buffer* source, GLintptr sourceOffset, GLintptr destOffset, GLsizeiptr size);
    GLvoid *mapRange(GLintptr offset, GLsizeiptr length, GLbitfield access);
    void unmap();

    GLenum  getUsage() const { return mUsage; }
    GLint getAccessFlags() const {  return mAccessFlags; }
    GLboolean isMapped() const { return mMapped; }
    GLvoid *getMapPointer() const { return mMapPointer; }
    GLint64 getMapOffset() const { return mMapOffset; }
    GLint64 getMapLength() const { return mMapLength; }
    GLint64 getSize() const { return mSize; }

    rx::BufferImpl *getImplementation() const { return mBuffer; }

    void markTransformFeedbackUsage();

  private:
    DISALLOW_COPY_AND_ASSIGN(Buffer);

    rx::BufferImpl *mBuffer;

    GLenum mUsage;
    GLsizeiptr mSize;
    GLint mAccessFlags;
    GLboolean mMapped;
    GLvoid *mMapPointer;
    GLint64 mMapOffset;
    GLint64 mMapLength;
};

}

#endif   
