




#ifndef SHARED_SURFACE_EGL_H_
#define SHARED_SURFACE_EGL_H_

#include "SharedSurfaceGL.h"
#include "SurfaceFactory.h"
#include "GLLibraryEGL.h"
#include "SurfaceTypes.h"
#include "mozilla/Attributes.h"
#include "mozilla/Mutex.h"

namespace mozilla {
namespace gl {

class GLContext;

class SharedSurface_EGLImage
    : public SharedSurface_GL
{
public:
    static SharedSurface_EGLImage* Create(GLContext* prodGL,
                                                  const GLFormats& formats,
                                                  const gfxIntSize& size,
                                                  bool hasAlpha,
                                                  EGLContext context);

    static SharedSurface_EGLImage* Cast(SharedSurface* surf) {
        MOZ_ASSERT(surf->Type() == SharedSurfaceType::EGLImageShare);

        return (SharedSurface_EGLImage*)surf;
    }

protected:
    mutable Mutex mMutex;
    GLLibraryEGL* const mEGL;
    const GLFormats mFormats;
    GLuint mProdTex;
    nsRefPtr<gfxImageSurface> mPixels;
    GLuint mProdTexForPipe; 
    EGLImage mImage;
    GLContext* mCurConsGL;
    GLuint mConsTex;
    nsRefPtr<TextureGarbageBin> mGarbageBin;
    EGLSync mSync;
    bool mPipeFailed;   
    bool mPipeComplete; 
    bool mPipeActive;   

    SharedSurface_EGLImage(GLContext* gl,
                           GLLibraryEGL* egl,
                           const gfxIntSize& size,
                           bool hasAlpha,
                           const GLFormats& formats,
                           GLuint prodTex)
        : SharedSurface_GL(SharedSurfaceType::EGLImageShare,
                           AttachmentType::GLTexture,
                           gl,
                           size,
                           hasAlpha)
        , mMutex("SharedSurface_EGLImage mutex")
        , mEGL(egl)
        , mFormats(formats)
        , mProdTex(prodTex)
        , mProdTexForPipe(0)
        , mImage(0)
        , mCurConsGL(nullptr)
        , mConsTex(0)
        , mSync(0)
        , mPipeFailed(false)
        , mPipeComplete(false)
        , mPipeActive(false)
    {}

    EGLDisplay Display() const;

    static bool HasExtensions(GLLibraryEGL* egl, GLContext* gl);

public:
    virtual ~SharedSurface_EGLImage();

    virtual void LockProdImpl();
    virtual void UnlockProdImpl() {}


    virtual void Fence();
    virtual bool WaitSync();


    virtual GLuint Texture() const {
        return mProdTex;
    }

    
    
    GLuint AcquireConsumerTexture(GLContext* consGL);

    
    gfxImageSurface* GetPixels() const;
};



class SurfaceFactory_EGLImage
    : public SurfaceFactory_GL
{
public:
    
    static SurfaceFactory_EGLImage* Create(GLContext* prodGL,
                                           const SurfaceCaps& caps);

protected:
    const EGLContext mContext;

    SurfaceFactory_EGLImage(GLContext* prodGL,
                            EGLContext context,
                            const SurfaceCaps& caps)
        : SurfaceFactory_GL(prodGL, SharedSurfaceType::EGLImageShare, caps)
        , mContext(context)
    {}

public:
    virtual SharedSurface* CreateShared(const gfxIntSize& size) {
        bool hasAlpha = mReadCaps.alpha;
        return SharedSurface_EGLImage::Create(mGL, mFormats, size, hasAlpha, mContext);
    }
};

} 
} 

#endif 
