








#ifndef LIBGLESV2_FENCE_H_
#define LIBGLESV2_FENCE_H_

#include "libGLESv2/Error.h"

#include "common/angleutils.h"
#include "common/RefCountObject.h"

namespace rx
{
class Renderer;
class FenceNVImpl;
class FenceSyncImpl;
}

namespace gl
{

class FenceNV
{
  public:
    explicit FenceNV(rx::FenceNVImpl *impl);
    virtual ~FenceNV();

    GLboolean isFence() const;
    Error setFence(GLenum condition);
    Error testFence(GLboolean *outResult);
    Error finishFence();

    GLboolean getStatus() const { return mStatus; }
    GLuint getCondition() const { return mCondition; }

  private:
    DISALLOW_COPY_AND_ASSIGN(FenceNV);

    rx::FenceNVImpl *mFence;

    bool mIsSet;

    GLboolean mStatus;
    GLenum mCondition;
};

class FenceSync : public RefCountObject
{
  public:
    explicit FenceSync(rx::FenceSyncImpl *impl, GLuint id);
    virtual ~FenceSync();

    Error set(GLenum condition);
    Error clientWait(GLbitfield flags, GLuint64 timeout, GLenum *outResult);
    Error serverWait(GLbitfield flags, GLuint64 timeout);
    Error getStatus(GLint *outResult) const;

    GLuint getCondition() const { return mCondition; }

  private:
    DISALLOW_COPY_AND_ASSIGN(FenceSync);

    rx::FenceSyncImpl *mFence;

    GLenum mCondition;
};

}

#endif   
