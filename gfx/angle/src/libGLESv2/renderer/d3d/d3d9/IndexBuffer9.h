







#ifndef LIBGLESV2_RENDERER_INDEXBUFFER9_H_
#define LIBGLESV2_RENDERER_INDEXBUFFER9_H_

#include "libGLESv2/renderer/d3d/IndexBuffer.h"

namespace rx
{
class Renderer9;

class IndexBuffer9 : public IndexBuffer
{
  public:
    explicit IndexBuffer9(Renderer9 *const renderer);
    virtual ~IndexBuffer9();

    virtual gl::Error initialize(unsigned int bufferSize, GLenum indexType, bool dynamic);

    static IndexBuffer9 *makeIndexBuffer9(IndexBuffer *indexBuffer);

    virtual gl::Error mapBuffer(unsigned int offset, unsigned int size, void** outMappedMemory);
    virtual gl::Error unmapBuffer();

    virtual GLenum getIndexType() const;
    virtual unsigned int getBufferSize() const;
    virtual gl::Error setSize(unsigned int bufferSize, GLenum indexType);

    virtual gl::Error discard();

    D3DFORMAT getIndexFormat() const;
    IDirect3DIndexBuffer9 *getBuffer() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(IndexBuffer9);

    rx::Renderer9 *const mRenderer;

    IDirect3DIndexBuffer9 *mIndexBuffer;
    unsigned int mBufferSize;
    GLenum mIndexType;
    bool mDynamic;
};

}

#endif 
