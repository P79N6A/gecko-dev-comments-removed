







#ifndef LIBGLESV2_RENDERER_Fence11_H_
#define LIBGLESV2_RENDERER_Fence11_H_

#include "libGLESv2/renderer/FenceImpl.h"

namespace rx
{
class Renderer11;

class Fence11 : public FenceImpl
{
  public:
    explicit Fence11(rx::Renderer11 *renderer);
    virtual ~Fence11();

    bool isSet() const;
    void set();
    bool test(bool flushCommandBuffer);
    bool hasError() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(Fence11);

    rx::Renderer11 *mRenderer;
    ID3D11Query *mQuery;
};

}

#endif 
