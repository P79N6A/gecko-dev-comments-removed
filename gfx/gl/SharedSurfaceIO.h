




#ifndef SHARED_SURFACEIO_H_
#define SHARED_SURFACEIO_H_

#include "SharedSurfaceGL.h"
#include "mozilla/RefPtr.h"

class MacIOSurface;

namespace mozilla {
namespace gl {

class SharedSurface_IOSurface : public SharedSurface_GL
{
public:
    static SharedSurface_IOSurface* Create(MacIOSurface* surface, GLContext *gl, bool hasAlpha);

    ~SharedSurface_IOSurface();

    virtual void LockProdImpl() MOZ_OVERRIDE { }
    virtual void UnlockProdImpl() MOZ_OVERRIDE { }

    virtual void Fence() MOZ_OVERRIDE;
    virtual bool WaitSync() MOZ_OVERRIDE { return true; }

    virtual bool ReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                            GLenum format, GLenum type, GLvoid *pixels) MOZ_OVERRIDE;

    virtual GLuint ProdTexture() MOZ_OVERRIDE {
        return mTexture;
    }

    virtual GLenum ProdTextureTarget() const MOZ_OVERRIDE {
        return LOCAL_GL_TEXTURE_RECTANGLE_ARB;
    }

    static SharedSurface_IOSurface* Cast(SharedSurface *surf) {
        MOZ_ASSERT(surf->Type() == SharedSurfaceType::IOSurface);
        return static_cast<SharedSurface_IOSurface*>(surf);
    }

    MacIOSurface* GetIOSurface() { return mSurface; }

private:
    SharedSurface_IOSurface(MacIOSurface* surface, GLContext* gl, const gfx::IntSize& size, bool hasAlpha);

    RefPtr<MacIOSurface> mSurface;
    nsRefPtr<gfxImageSurface> mImageSurface;
    GLuint mTexture;
};

class SurfaceFactory_IOSurface : public SurfaceFactory_GL
{
public:
    SurfaceFactory_IOSurface(GLContext* gl,
                             const SurfaceCaps& caps)
        : SurfaceFactory_GL(gl, SharedSurfaceType::IOSurface, caps)
    {
    }

protected:
    virtual SharedSurface* CreateShared(const gfx::IntSize& size) MOZ_OVERRIDE;
};

} 
} 

#endif 
