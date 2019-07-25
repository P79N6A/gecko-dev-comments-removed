







#ifndef LIBGLESV2_FENCE_H_
#define LIBGLESV2_FENCE_H_

#define GL_APICALL
#include <GLES2/gl2.h>
#include <d3d9.h>

#include "common/angleutils.h"

namespace gl
{

class Fence
{
  public:
    Fence();
    virtual ~Fence();

    GLboolean isFence();
    void setFence(GLenum condition);
    GLboolean testFence();
    void finishFence();
    void getFenceiv(GLenum pname, GLint *params);

  private:
    DISALLOW_COPY_AND_ASSIGN(Fence);

    IDirect3DQuery9* mQuery;
    GLenum mCondition;
    GLboolean mStatus;
};

}

#endif   
