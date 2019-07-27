







#ifndef LIBGLESV2_RENDERER_FENCE11_H_
#define LIBGLESV2_RENDERER_FENCE11_H_

#include "libGLESv2/renderer/FenceImpl.h"

namespace rx
{
class Renderer11;

class FenceNV11 : public FenceNVImpl
{
  public:
    explicit FenceNV11(Renderer11 *renderer);
    virtual ~FenceNV11();

    gl::Error set();
    gl::Error test(bool flushCommandBuffer, GLboolean *outFinished);
    gl::Error finishFence(GLboolean *outFinished);

  private:
    DISALLOW_COPY_AND_ASSIGN(FenceNV11);

    template<class T> friend gl::Error FenceSetHelper(T *fence);
    template<class T> friend gl::Error FenceTestHelper(T *fence, bool flushCommandBuffer, GLboolean *outFinished);

    Renderer11 *mRenderer;
    ID3D11Query *mQuery;
};

class FenceSync11 : public FenceSyncImpl
{
  public:
    explicit FenceSync11(Renderer11 *renderer);
    virtual ~FenceSync11();

    gl::Error set();
    gl::Error clientWait(GLbitfield flags, GLuint64 timeout, GLenum *outResult);
    gl::Error serverWait(GLbitfield flags, GLuint64 timeout);
    gl::Error getStatus(GLint *outResult);

  private:
    DISALLOW_COPY_AND_ASSIGN(FenceSync11);

    template<class T> friend gl::Error FenceSetHelper(T *fence);
    template<class T> friend gl::Error FenceTestHelper(T *fence, bool flushCommandBuffer, GLboolean *outFinished);

    Renderer11 *mRenderer;
    ID3D11Query *mQuery;
    LONGLONG mCounterFrequency;
};

}

#endif 
