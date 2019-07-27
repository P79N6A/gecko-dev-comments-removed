







#ifndef LIBGLESV2_RENDERER_VERTEXBUFFER9_H_
#define LIBGLESV2_RENDERER_VERTEXBUFFER9_H_

#include "libGLESv2/renderer/d3d/VertexBuffer.h"

namespace rx
{
class Renderer9;

class VertexBuffer9 : public VertexBuffer
{
  public:
    explicit VertexBuffer9(rx::Renderer9 *const renderer);
    virtual ~VertexBuffer9();

    virtual bool initialize(unsigned int size, bool dynamicUsage);

    static VertexBuffer9 *makeVertexBuffer9(VertexBuffer *vertexBuffer);

    virtual bool storeVertexAttributes(const gl::VertexAttribute &attrib, const gl::VertexAttribCurrentValueData &currentValue,
                                       GLint start, GLsizei count, GLsizei instances, unsigned int offset);

    virtual bool getSpaceRequired(const gl::VertexAttribute &attrib, GLsizei count, GLsizei instances, unsigned int *outSpaceRequired) const;

    virtual unsigned int getBufferSize() const;
    virtual bool setBufferSize(unsigned int size);
    virtual bool discard();

    IDirect3DVertexBuffer9 *getBuffer() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(VertexBuffer9);

    rx::Renderer9 *const mRenderer;

    IDirect3DVertexBuffer9 *mVertexBuffer;
    unsigned int mBufferSize;
    bool mDynamicUsage;

    static bool spaceRequired(const gl::VertexAttribute &attrib, std::size_t count, GLsizei instances,
                              unsigned int *outSpaceRequired);
};

}

#endif 
