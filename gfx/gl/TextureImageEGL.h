




#ifndef TEXTUREIMAGEEGL_H_
#define TEXTUREIMAGEEGL_H_

#include "GLTextureImage.h"

namespace mozilla {
namespace gl {

class TextureImageEGL final
    : public TextureImage
{
public:
    TextureImageEGL(GLuint aTexture,
                    const gfx::IntSize& aSize,
                    GLenum aWrapMode,
                    ContentType aContentType,
                    GLContext* aContext,
                    Flags aFlags = TextureImage::NoFlags,
                    TextureState aTextureState = Created,
                    TextureImage::ImageFormat aImageFormat = gfxImageFormat::Unknown);

    virtual ~TextureImageEGL();

    virtual void GetUpdateRegion(nsIntRegion& aForRegion);

    virtual gfx::DrawTarget* BeginUpdate(nsIntRegion& aRegion);

    virtual void EndUpdate();

    virtual bool DirectUpdate(gfx::DataSourceSurface* aSurf, const nsIntRegion& aRegion, const gfx::IntPoint& aFrom = gfx::IntPoint(0,0));

    virtual void BindTexture(GLenum aTextureUnit);

    virtual GLuint GetTextureID()
    {
        
        if (mTextureState == Created) {
            Resize(mSize);
        }
        return mTexture;
    };

    virtual bool InUpdate() const { return !!mUpdateDrawTarget; }

    virtual void Resize(const gfx::IntSize& aSize);

    bool BindTexImage();

    bool ReleaseTexImage();

    virtual bool CreateEGLSurface(gfxASurface* aSurface)
    {
        return false;
    }

    virtual void DestroyEGLSurface(void);

protected:
    typedef gfxImageFormat ImageFormat;

    GLContext* mGLContext;

    gfx::IntRect mUpdateRect;
    gfx::SurfaceFormat mUpdateFormat;
    RefPtr<gfx::DrawTarget> mUpdateDrawTarget;
    EGLImage mEGLImage;
    GLuint mTexture;
    EGLSurface mSurface;
    EGLConfig mConfig;
    TextureState mTextureState;

    bool mBound;
};

already_AddRefed<TextureImage>
CreateTextureImageEGL(GLContext *gl,
                      const gfx::IntSize& aSize,
                      TextureImage::ContentType aContentType,
                      GLenum aWrapMode,
                      TextureImage::Flags aFlags,
                      TextureImage::ImageFormat aImageFormat);

already_AddRefed<TextureImage>
TileGenFuncEGL(GLContext *gl,
               const gfx::IntSize& aSize,
               TextureImage::ContentType aContentType,
               TextureImage::Flags aFlags,
               TextureImage::ImageFormat aImageFormat);

} 
} 

#endif 
