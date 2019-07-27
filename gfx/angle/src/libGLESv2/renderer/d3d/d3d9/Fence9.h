







#ifndef LIBGLESV2_RENDERER_FENCE9_H_
#define LIBGLESV2_RENDERER_FENCE9_H_

#include "libGLESv2/renderer/FenceImpl.h"

namespace rx
{
class Renderer9;

class FenceNV9 : public FenceNVImpl
{
  public:
    explicit FenceNV9(Renderer9 *renderer);
    virtual ~FenceNV9();

    gl::Error set();
    gl::Error test(bool flushCommandBuffer, GLboolean *outFinished);
    gl::Error finishFence(GLboolean *outFinished);

  private:
    DISALLOW_COPY_AND_ASSIGN(FenceNV9);

    Renderer9 *mRenderer;
    IDirect3DQuery9 *mQuery;
};

}

#endif 
