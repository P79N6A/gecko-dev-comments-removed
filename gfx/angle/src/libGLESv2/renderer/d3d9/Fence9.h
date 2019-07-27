







#ifndef LIBGLESV2_RENDERER_FENCE9_H_
#define LIBGLESV2_RENDERER_FENCE9_H_

#include "libGLESv2/renderer/FenceImpl.h"

namespace rx
{
class Renderer9;

class Fence9 : public FenceImpl
{
  public:
    explicit Fence9(rx::Renderer9 *renderer);
    virtual ~Fence9();

    bool isSet() const;
    void set();
    bool test(bool flushCommandBuffer);
    bool hasError() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(Fence9);

    rx::Renderer9 *mRenderer;
    IDirect3DQuery9 *mQuery;
};

}

#endif 
