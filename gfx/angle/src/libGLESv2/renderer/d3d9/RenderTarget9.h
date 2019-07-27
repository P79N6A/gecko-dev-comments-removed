








#ifndef LIBGLESV2_RENDERER_RENDERTARGET9_H_
#define LIBGLESV2_RENDERER_RENDERTARGET9_H_

#include "libGLESv2/renderer/RenderTarget.h"

namespace rx
{
class Renderer;
class Renderer9;

class RenderTarget9 : public RenderTarget
{
  public:
    RenderTarget9(Renderer *renderer, IDirect3DSurface9 *surface);
    RenderTarget9(Renderer *renderer, GLsizei width, GLsizei height, GLenum internalFormat, GLsizei samples);
    virtual ~RenderTarget9();

    static RenderTarget9 *makeRenderTarget9(RenderTarget *renderTarget);

    virtual void invalidate(GLint x, GLint y, GLsizei width, GLsizei height);

    IDirect3DSurface9 *getSurface();

  private:
    DISALLOW_COPY_AND_ASSIGN(RenderTarget9);

    IDirect3DSurface9 *mRenderTarget;

    Renderer9 *mRenderer;
};

}

#endif 
