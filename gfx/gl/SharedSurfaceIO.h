




#ifndef SHARED_SURFACEIO_H_
#define SHARED_SURFACEIO_H_

#include "mozilla/RefPtr.h"
#include "SharedSurface.h"

class MacIOSurface;

namespace mozilla {
namespace gl {

class SharedSurface_IOSurface : public SharedSurface
{
public:
    static SharedSurface_IOSurface* Create(MacIOSurface* surface, GLContext* gl, bool hasAlpha);

    ~SharedSurface_IOSurface();

    virtual void LockProdImpl() MOZ_OVERRIDE { }
    virtual void UnlockProdImpl() MOZ_OVERRIDE { }

    virtual void Fence() MOZ_OVERRIDE;
    virtual bool WaitSync() MOZ_OVERRIDE { return true; }

    virtual bool ReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                            GLenum format, GLenum type, GLvoid *pixels) MOZ_OVERRIDE;

    virtual GLuint ProdTexture() MOZ_OVERRIDE {
        return mProdTex;
    }

    virtual GLenum ProdTextureTarget() const MOZ_OVERRIDE {
        return LOCAL_GL_TEXTURE_RECTANGLE_ARB;
    }

    static SharedSurface_IOSurface* Cast(SharedSurface *surf) {
        MOZ_ASSERT(surf->mType == SharedSurfaceType::IOSurface);
        return static_cast<SharedSurface_IOSurface*>(surf);
    }

    GLuint ConsTexture(GLContext* consGL);

    GLenum ConsTextureTarget() const {
        return LOCAL_GL_TEXTURE_RECTANGLE_ARB;
    }

    MacIOSurface* GetIOSurface() const {
        return mSurface;
    }

private:
    SharedSurface_IOSurface(MacIOSurface* surface, GLContext* gl, const gfx::IntSize& size, bool hasAlpha);

    RefPtr<MacIOSurface> mSurface;
    GLuint mProdTex;
    const GLContext* mCurConsGL;
    GLuint mConsTex;
};

class SurfaceFactory_IOSurface : public SurfaceFactory
{
public:
    SurfaceFactory_IOSurface(GLContext* gl,
                             const SurfaceCaps& caps)
        : SurfaceFactory(gl, SharedSurfaceType::IOSurface, caps)
    {
    }

protected:
    virtual SharedSurface* CreateShared(const gfx::IntSize& size) MOZ_OVERRIDE;
};

} 
} 

#endif 
