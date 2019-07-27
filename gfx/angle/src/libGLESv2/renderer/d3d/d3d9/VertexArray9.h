







#ifndef LIBGLESV2_RENDERER_VERTEXARRAY9_H_
#define LIBGLESV2_RENDERER_VERTEXARRAY9_H_

#include "libGLESv2/renderer/VertexArrayImpl.h"
#include "libGLESv2/renderer/d3d/d3d9/Renderer9.h"

namespace rx
{
class Renderer9;

class VertexArray9 : public VertexArrayImpl
{
  public:
    VertexArray9(rx::Renderer9 *renderer)
        : VertexArrayImpl(),
          mRenderer(renderer)
    {
    }

    virtual ~VertexArray9() { }

    virtual void setElementArrayBuffer(const gl::Buffer *buffer) { }
    virtual void setAttribute(size_t idx, const gl::VertexAttribute &attr) { }
    virtual void setAttributeDivisor(size_t idx, GLuint divisor) { }
    virtual void enableAttribute(size_t idx, bool enabledState) { }

  private:
    DISALLOW_COPY_AND_ASSIGN(VertexArray9);

    rx::Renderer9 *mRenderer;
};

}

#endif 
