







#ifndef LIBGLESV2_RENDERER_BUFFER9_H_
#define LIBGLESV2_RENDERER_BUFFER9_H_

#include "libGLESv2/renderer/d3d/BufferD3D.h"
#include "libGLESv2/renderer/d3d/MemoryBuffer.h"
#include "libGLESv2/angletypes.h"

namespace rx
{
class Renderer9;

class Buffer9 : public BufferD3D
{
  public:
    Buffer9(rx::Renderer9 *renderer);
    virtual ~Buffer9();

    static Buffer9 *makeBuffer9(BufferImpl *buffer);

    
    virtual size_t getSize() const { return mSize; }
    virtual bool supportsDirectBinding() const { return false; }
    virtual Renderer* getRenderer();

    
    virtual gl::Error setData(const void* data, size_t size, GLenum usage);
    gl::Error getData(const uint8_t **outData) override;
    virtual gl::Error setSubData(const void* data, size_t size, size_t offset);
    virtual gl::Error copySubData(BufferImpl* source, GLintptr sourceOffset, GLintptr destOffset, GLsizeiptr size);
    virtual gl::Error map(size_t offset, size_t length, GLbitfield access, GLvoid **mapPtr);
    virtual gl::Error unmap();
    virtual void markTransformFeedbackUsage();

  private:
    DISALLOW_COPY_AND_ASSIGN(Buffer9);

    rx::Renderer9 *mRenderer;
    MemoryBuffer mMemory;
    size_t mSize;
};

}

#endif 
