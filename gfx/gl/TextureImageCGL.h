




#ifndef TextureImageCGL_h_
#define TextureImageCGL_h_

#include "GLTextureImage.h"
#include "GLContextTypes.h"
#include "nsAutoPtr.h"
#include "nsSize.h"

class gfxASurface;

namespace mozilla {
namespace gl {

class TextureImageCGL : public BasicTextureImage
{
public:

    TextureImageCGL(GLuint aTexture,
                    const nsIntSize& aSize,
                    GLenum aWrapMode,
                    ContentType aContentType,
                    GLContext* aContext,
                    TextureImage::Flags aFlags = TextureImage::NoFlags,
                    TextureImage::ImageFormat aImageFormat = gfxImageFormatUnknown);

    ~TextureImageCGL();

protected:
    already_AddRefed<gfxASurface>
    GetSurfaceForUpdate(const gfxIntSize& aSize, ImageFormat aFmt);

    bool FinishedSurfaceUpdate();

    void FinishedSurfaceUpload();

private:

    GLuint mPixelBuffer;
    int32_t mPixelBufferSize;
    bool mBoundPixelBuffer;
};

}
}

#endif
