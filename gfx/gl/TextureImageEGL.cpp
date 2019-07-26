




#include "TextureImageEGL.h"
#include "GLLibraryEGL.h"
#include "GLContext.h"
#include "gfxPlatform.h"
#include "mozilla/gfx/Types.h"

namespace mozilla {
namespace gl {

static GLenum
GLFormatForImage(gfxImageFormat aFormat)
{
    switch (aFormat) {
    case gfxImageFormatARGB32:
    case gfxImageFormatRGB24:
        
        return LOCAL_GL_RGBA;
    case gfxImageFormatRGB16_565:
        return LOCAL_GL_RGB;
    case gfxImageFormatA8:
        return LOCAL_GL_LUMINANCE;
    default:
        NS_WARNING("Unknown GL format for Image format");
    }
    return 0;
}

static GLenum
GLTypeForImage(gfxImageFormat aFormat)
{
    switch (aFormat) {
    case gfxImageFormatARGB32:
    case gfxImageFormatRGB24:
    case gfxImageFormatA8:
        return LOCAL_GL_UNSIGNED_BYTE;
    case gfxImageFormatRGB16_565:
        return LOCAL_GL_UNSIGNED_SHORT_5_6_5;
    default:
        NS_WARNING("Unknown GL format for Image format");
    }
    return 0;
}

TextureImageEGL::TextureImageEGL(GLuint aTexture,
                                 const nsIntSize& aSize,
                                 GLenum aWrapMode,
                                 ContentType aContentType,
                                 GLContext* aContext,
                                 Flags aFlags,
                                 TextureState aTextureState,
                                 TextureImage::ImageFormat aImageFormat)
    : TextureImage(aSize, aWrapMode, aContentType, aFlags)
    , mGLContext(aContext)
    , mUpdateFormat(aImageFormat)
    , mEGLImage(nullptr)
    , mTexture(aTexture)
    , mSurface(nullptr)
    , mConfig(nullptr)
    , mTextureState(aTextureState)
    , mBound(false)
{
    if (mUpdateFormat == gfxImageFormatUnknown) {
        mUpdateFormat = gfxPlatform::GetPlatform()->OptimalFormatForContent(GetContentType());
    }

    if (mUpdateFormat == gfxImageFormatRGB16_565) {
        mTextureFormat = gfx::FORMAT_R8G8B8X8;
    } else if (mUpdateFormat == gfxImageFormatRGB24) {
        
        
        
        mTextureFormat = gfx::FORMAT_B8G8R8X8;
    } else {
        mTextureFormat = gfx::FORMAT_B8G8R8A8;
    }
}

TextureImageEGL::~TextureImageEGL()
{
    if (mGLContext->IsDestroyed() || !mGLContext->IsOwningThreadCurrent()) {
        return;
    }

    
    
    
    
    mGLContext->MakeCurrent();
    mGLContext->fDeleteTextures(1, &mTexture);
    ReleaseTexImage();
    DestroyEGLSurface();
}

void
TextureImageEGL::GetUpdateRegion(nsIntRegion& aForRegion)
{
    if (mTextureState != Valid) {
        
        
        aForRegion = nsIntRect(nsIntPoint(0, 0), mSize);
    }

    
    
    
    
    aForRegion = nsIntRegion(aForRegion.GetBounds());
}

gfxASurface*
TextureImageEGL::BeginUpdate(nsIntRegion& aRegion)
{
    NS_ASSERTION(!mUpdateSurface, "BeginUpdate() without EndUpdate()?");

    
    GetUpdateRegion(aRegion);
    mUpdateRect = aRegion.GetBounds();

    
    if (!nsIntRect(nsIntPoint(0, 0), mSize).Contains(mUpdateRect)) {
        NS_ERROR("update outside of image");
        return nullptr;
    }

    

    mUpdateSurface =
        new gfxImageSurface(gfxIntSize(mUpdateRect.width, mUpdateRect.height),
                            mUpdateFormat);

    mUpdateSurface->SetDeviceOffset(gfxPoint(-mUpdateRect.x, -mUpdateRect.y));

    return mUpdateSurface;
}

void
TextureImageEGL::EndUpdate()
{
    NS_ASSERTION(!!mUpdateSurface, "EndUpdate() without BeginUpdate()?");

    

    
    
    

    
    
    
    mUpdateSurface->SetDeviceOffset(gfxPoint(0, 0));

    nsRefPtr<gfxImageSurface> uploadImage = nullptr;
    gfxIntSize updateSize(mUpdateRect.width, mUpdateRect.height);

    NS_ASSERTION(mUpdateSurface->GetType() == gfxSurfaceTypeImage &&
                  mUpdateSurface->GetSize() == updateSize,
                  "Upload image isn't an image surface when one is expected, or is wrong size!");

    uploadImage = static_cast<gfxImageSurface*>(mUpdateSurface.get());

    if (!uploadImage) {
        return;
    }

    mGLContext->MakeCurrent();
    mGLContext->fBindTexture(LOCAL_GL_TEXTURE_2D, mTexture);

    if (mTextureState != Valid) {
        NS_ASSERTION(mUpdateRect.x == 0 && mUpdateRect.y == 0 &&
                      mUpdateRect.Size() == mSize,
                      "Bad initial update on non-created texture!");

        mGLContext->fTexImage2D(LOCAL_GL_TEXTURE_2D,
                                0,
                                GLFormatForImage(mUpdateFormat),
                                mUpdateRect.width,
                                mUpdateRect.height,
                                0,
                                GLFormatForImage(uploadImage->Format()),
                                GLTypeForImage(uploadImage->Format()),
                                uploadImage->Data());
    } else {
        mGLContext->fTexSubImage2D(LOCAL_GL_TEXTURE_2D,
                                    0,
                                    mUpdateRect.x,
                                    mUpdateRect.y,
                                    mUpdateRect.width,
                                    mUpdateRect.height,
                                    GLFormatForImage(uploadImage->Format()),
                                    GLTypeForImage(uploadImage->Format()),
                                    uploadImage->Data());
    }

    mUpdateSurface = nullptr;
    mTextureState = Valid;
    return;         
}

bool
TextureImageEGL::DirectUpdate(gfxASurface* aSurf, const nsIntRegion& aRegion, const nsIntPoint& aFrom )
{
    nsIntRect bounds = aRegion.GetBounds();

    nsIntRegion region;
    if (mTextureState != Valid) {
        bounds = nsIntRect(0, 0, mSize.width, mSize.height);
        region = nsIntRegion(bounds);
    } else {
        region = aRegion;
    }

    mTextureFormat =
      mGLContext->UploadSurfaceToTexture(aSurf,
                                          region,
                                          mTexture,
                                          mTextureState == Created,
                                          bounds.TopLeft() + aFrom,
                                          false);

    mTextureState = Valid;
    return true;
}

void
TextureImageEGL::BindTexture(GLenum aTextureUnit)
{
    
    if (mTextureState == Created) {
        Resize(mSize);
    }

    mGLContext->fActiveTexture(aTextureUnit);
    mGLContext->fBindTexture(LOCAL_GL_TEXTURE_2D, mTexture);
    mGLContext->fActiveTexture(LOCAL_GL_TEXTURE0);
}

void
TextureImageEGL::Resize(const nsIntSize& aSize)
{
    NS_ASSERTION(!mUpdateSurface, "Resize() while in update?");

    if (mSize == aSize && mTextureState != Created)
        return;

    mGLContext->fBindTexture(LOCAL_GL_TEXTURE_2D, mTexture);

    mGLContext->fTexImage2D(LOCAL_GL_TEXTURE_2D,
                            0,
                            GLFormatForImage(mUpdateFormat),
                            aSize.width,
                            aSize.height,
                            0,
                            GLFormatForImage(mUpdateFormat),
                            GLTypeForImage(mUpdateFormat),
                            nullptr);

    mTextureState = Allocated;
    mSize = aSize;
}

bool
TextureImageEGL::BindTexImage()
{
    if (mBound && !ReleaseTexImage())
        return false;

    EGLBoolean success =
        sEGLLibrary.fBindTexImage(EGL_DISPLAY(),
                                  (EGLSurface)mSurface,
                                  LOCAL_EGL_BACK_BUFFER);

    if (success == LOCAL_EGL_FALSE)
        return false;

    mBound = true;
    return true;
}

bool
TextureImageEGL::ReleaseTexImage()
{
    if (!mBound)
        return true;

    EGLBoolean success =
        sEGLLibrary.fReleaseTexImage(EGL_DISPLAY(),
                                      (EGLSurface)mSurface,
                                      LOCAL_EGL_BACK_BUFFER);

    if (success == LOCAL_EGL_FALSE)
        return false;

    mBound = false;
    return true;
}

void
TextureImageEGL::DestroyEGLSurface(void)
{
    if (!mSurface)
        return;

    sEGLLibrary.fDestroySurface(EGL_DISPLAY(), mSurface);
    mSurface = nullptr;
}

void
TextureImageEGL::ApplyFilter()
{
    mGLContext->ApplyFilterToBoundTexture(mFilter);
}

}
}