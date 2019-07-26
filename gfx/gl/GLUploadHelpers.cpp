




#include "GLUploadHelpers.h"

#include "GLContext.h"
#include "mozilla/gfx/2D.h"
#include "gfxASurface.h"
#include "gfxUtils.h"
#include "gfxContext.h"
#include "nsRegion.h"

namespace mozilla {

using gfx::SurfaceFormat;

namespace gl {

static unsigned int
DataOffset(const nsIntPoint &aPoint, int32_t aStride, gfxImageFormat aFormat)
{
  unsigned int data = aPoint.y * aStride;
  data += aPoint.x * gfxASurface::BytePerPixelFromFormat(aFormat);
  return data;
}

static gfxImageFormat
ImageFormatForSurfaceFormat(gfx::SurfaceFormat aFormat)
{
    switch (aFormat) {
        case gfx::SurfaceFormat::B8G8R8A8:
            return gfxImageFormatARGB32;
        case gfx::SurfaceFormat::B8G8R8X8:
            return gfxImageFormatRGB24;
        case gfx::SurfaceFormat::R5G6B5:
            return gfxImageFormatRGB16_565;
        case gfx::SurfaceFormat::A8:
            return gfxImageFormatA8;
        default:
            return gfxImageFormatUnknown;
    }
}

static GLint GetAddressAlignment(ptrdiff_t aAddress)
{
    if (!(aAddress & 0x7)) {
       return 8;
    } else if (!(aAddress & 0x3)) {
        return 4;
    } else if (!(aAddress & 0x1)) {
        return 2;
    } else {
        return 1;
    }
}



static void
CopyAndPadTextureData(const GLvoid* srcBuffer,
                      GLvoid* dstBuffer,
                      GLsizei srcWidth, GLsizei srcHeight,
                      GLsizei dstWidth, GLsizei dstHeight,
                      GLsizei stride, GLint pixelsize)
{
    unsigned char *rowDest = static_cast<unsigned char*>(dstBuffer);
    const unsigned char *source = static_cast<const unsigned char*>(srcBuffer);

    for (GLsizei h = 0; h < srcHeight; ++h) {
        memcpy(rowDest, source, srcWidth * pixelsize);
        rowDest += dstWidth * pixelsize;
        source += stride;
    }

    GLsizei padHeight = srcHeight;

    
    if (dstHeight > srcHeight) {
        memcpy(rowDest, source - stride, srcWidth * pixelsize);
        padHeight++;
    }

    
    if (dstWidth > srcWidth) {
        rowDest = static_cast<unsigned char*>(dstBuffer) + srcWidth * pixelsize;
        for (GLsizei h = 0; h < padHeight; ++h) {
            memcpy(rowDest, rowDest - pixelsize, pixelsize);
            rowDest += dstWidth * pixelsize;
        }
    }
}







bool
CanUploadSubTextures(GLContext* gl)
{
    if (!gl->WorkAroundDriverBugs())
        return true;

    
    
    if (gl->Renderer() == GLRenderer::Adreno200 ||
        gl->Renderer() == GLRenderer::Adreno205)
    {
        return false;
    }

    
    
    if (gl->Renderer() == GLRenderer::SGX540 ||
        gl->Renderer() == GLRenderer::SGX530)
    {
        return false;
    }

    return true;
}

static void
TexSubImage2DWithUnpackSubimageGLES(GLContext* gl,
                                    GLenum target, GLint level,
                                    GLint xoffset, GLint yoffset,
                                    GLsizei width, GLsizei height,
                                    GLsizei stride, GLint pixelsize,
                                    GLenum format, GLenum type,
                                    const GLvoid* pixels)
{
    gl->fPixelStorei(LOCAL_GL_UNPACK_ALIGNMENT,
                     std::min(GetAddressAlignment((ptrdiff_t)pixels),
                              GetAddressAlignment((ptrdiff_t)stride)));
    
    
    
    
    
    int rowLength = stride/pixelsize;
    gl->fPixelStorei(LOCAL_GL_UNPACK_ROW_LENGTH, rowLength);
    gl->fTexSubImage2D(target,
                       level,
                       xoffset,
                       yoffset,
                       width,
                       height-1,
                       format,
                       type,
                       pixels);
    gl->fPixelStorei(LOCAL_GL_UNPACK_ROW_LENGTH, 0);
    gl->fTexSubImage2D(target,
                       level,
                       xoffset,
                       yoffset+height-1,
                       width,
                       1,
                       format,
                       type,
                       (const unsigned char *)pixels+(height-1)*stride);
    gl->fPixelStorei(LOCAL_GL_UNPACK_ALIGNMENT, 4);
}

static void
TexSubImage2DWithoutUnpackSubimage(GLContext* gl,
                                   GLenum target, GLint level,
                                   GLint xoffset, GLint yoffset,
                                   GLsizei width, GLsizei height,
                                   GLsizei stride, GLint pixelsize,
                                   GLenum format, GLenum type,
                                   const GLvoid* pixels)
{
    
    
    
    
    unsigned char *newPixels = new unsigned char[width*height*pixelsize];
    unsigned char *rowDest = newPixels;
    const unsigned char *rowSource = (const unsigned char *)pixels;
    for (int h = 0; h < height; h++) {
            memcpy(rowDest, rowSource, width*pixelsize);
            rowDest += width*pixelsize;
            rowSource += stride;
    }

    stride = width*pixelsize;
    gl->fPixelStorei(LOCAL_GL_UNPACK_ALIGNMENT,
                     std::min(GetAddressAlignment((ptrdiff_t)newPixels),
                              GetAddressAlignment((ptrdiff_t)stride)));
    gl->fTexSubImage2D(target,
                       level,
                       xoffset,
                       yoffset,
                       width,
                       height,
                       format,
                       type,
                       newPixels);
    delete [] newPixels;
    gl->fPixelStorei(LOCAL_GL_UNPACK_ALIGNMENT, 4);
}

static void
TexSubImage2DHelper(GLContext *gl,
                    GLenum target, GLint level,
                    GLint xoffset, GLint yoffset,
                    GLsizei width, GLsizei height, GLsizei stride,
                    GLint pixelsize, GLenum format,
                    GLenum type, const GLvoid* pixels)
{
    if (gl->IsGLES2()) {
        if (stride == width * pixelsize) {
            gl->fPixelStorei(LOCAL_GL_UNPACK_ALIGNMENT,
                             std::min(GetAddressAlignment((ptrdiff_t)pixels),
                                      GetAddressAlignment((ptrdiff_t)stride)));
            gl->fTexSubImage2D(target,
                               level,
                               xoffset,
                               yoffset,
                               width,
                               height,
                               format,
                               type,
                               pixels);
            gl->fPixelStorei(LOCAL_GL_UNPACK_ALIGNMENT, 4);
        } else if (gl->IsExtensionSupported(GLContext::EXT_unpack_subimage)) {
            TexSubImage2DWithUnpackSubimageGLES(gl, target, level, xoffset, yoffset,
                                                width, height, stride,
                                                pixelsize, format, type, pixels);

        } else {
            TexSubImage2DWithoutUnpackSubimage(gl, target, level, xoffset, yoffset,
                                              width, height, stride,
                                              pixelsize, format, type, pixels);
        }
    } else {
        
        gl->fPixelStorei(LOCAL_GL_UNPACK_ALIGNMENT,
                         std::min(GetAddressAlignment((ptrdiff_t)pixels),
                                  GetAddressAlignment((ptrdiff_t)stride)));
        int rowLength = stride/pixelsize;
        gl->fPixelStorei(LOCAL_GL_UNPACK_ROW_LENGTH, rowLength);
        gl->fTexSubImage2D(target,
                           level,
                           xoffset,
                           yoffset,
                           width,
                           height,
                           format,
                           type,
                           pixels);
        gl->fPixelStorei(LOCAL_GL_UNPACK_ROW_LENGTH, 0);
        gl->fPixelStorei(LOCAL_GL_UNPACK_ALIGNMENT, 4);
    }
}

static void
TexImage2DHelper(GLContext *gl,
                 GLenum target, GLint level, GLint internalformat,
                 GLsizei width, GLsizei height, GLsizei stride,
                 GLint pixelsize, GLint border, GLenum format,
                 GLenum type, const GLvoid *pixels)
{
    if (gl->IsGLES2()) {

        NS_ASSERTION(format == (GLenum)internalformat,
                    "format and internalformat not the same for glTexImage2D on GLES2");

        if (!CanUploadNonPowerOfTwo(gl)
            && (stride != width * pixelsize
            || !gfx::IsPowerOfTwo(width)
            || !gfx::IsPowerOfTwo(height))) {

            
            
            GLsizei paddedWidth = gfx::NextPowerOfTwo(width);
            GLsizei paddedHeight = gfx::NextPowerOfTwo(height);

            GLvoid* paddedPixels = new unsigned char[paddedWidth * paddedHeight * pixelsize];

            
            
            CopyAndPadTextureData(pixels, paddedPixels, width, height,
                                  paddedWidth, paddedHeight, stride, pixelsize);

            gl->fPixelStorei(LOCAL_GL_UNPACK_ALIGNMENT,
                             std::min(GetAddressAlignment((ptrdiff_t)paddedPixels),
                                      GetAddressAlignment((ptrdiff_t)paddedWidth * pixelsize)));
            gl->fTexImage2D(target,
                            border,
                            internalformat,
                            paddedWidth,
                            paddedHeight,
                            border,
                            format,
                            type,
                            paddedPixels);
            gl->fPixelStorei(LOCAL_GL_UNPACK_ALIGNMENT, 4);

            delete[] static_cast<unsigned char*>(paddedPixels);
            return;
        }

        if (stride == width * pixelsize) {
            gl->fPixelStorei(LOCAL_GL_UNPACK_ALIGNMENT,
                             std::min(GetAddressAlignment((ptrdiff_t)pixels),
                                      GetAddressAlignment((ptrdiff_t)stride)));
            gl->fTexImage2D(target,
                            border,
                            internalformat,
                            width,
                            height,
                            border,
                            format,
                            type,
                            pixels);
            gl->fPixelStorei(LOCAL_GL_UNPACK_ALIGNMENT, 4);
        } else {
            
            
            gl->fTexImage2D(target,
                            border,
                            internalformat,
                            width,
                            height,
                            border,
                            format,
                            type,
                            nullptr);
            TexSubImage2DHelper(gl,
                                target,
                                level,
                                0,
                                0,
                                width,
                                height,
                                stride,
                                pixelsize,
                                format,
                                type,
                                pixels);
        }
    } else {
        

        gl->fPixelStorei(LOCAL_GL_UNPACK_ALIGNMENT,
                         std::min(GetAddressAlignment((ptrdiff_t)pixels),
                                  GetAddressAlignment((ptrdiff_t)stride)));
        int rowLength = stride/pixelsize;
        gl->fPixelStorei(LOCAL_GL_UNPACK_ROW_LENGTH, rowLength);
        gl->fTexImage2D(target,
                        level,
                        internalformat,
                        width,
                        height,
                        border,
                        format,
                        type,
                        pixels);
        gl->fPixelStorei(LOCAL_GL_UNPACK_ROW_LENGTH, 0);
        gl->fPixelStorei(LOCAL_GL_UNPACK_ALIGNMENT, 4);
    }
}

SurfaceFormat
UploadImageDataToTexture(GLContext* gl,
                         unsigned char* aData,
                         int32_t aStride,
                         gfxImageFormat aFormat,
                         const nsIntRegion& aDstRegion,
                         GLuint& aTexture,
                         bool aOverwrite,
                         bool aPixelBuffer,
                         GLenum aTextureUnit,
                         GLenum aTextureTarget)
{
    bool textureInited = aOverwrite ? false : true;
    gl->MakeCurrent();
    gl->fActiveTexture(aTextureUnit);

    if (!aTexture) {
        gl->fGenTextures(1, &aTexture);
        gl->fBindTexture(aTextureTarget, aTexture);
        gl->fTexParameteri(aTextureTarget,
                           LOCAL_GL_TEXTURE_MIN_FILTER,
                           LOCAL_GL_LINEAR);
        gl->fTexParameteri(aTextureTarget,
                           LOCAL_GL_TEXTURE_MAG_FILTER,
                           LOCAL_GL_LINEAR);
        gl->fTexParameteri(aTextureTarget,
                           LOCAL_GL_TEXTURE_WRAP_S,
                           LOCAL_GL_CLAMP_TO_EDGE);
        gl->fTexParameteri(aTextureTarget,
                           LOCAL_GL_TEXTURE_WRAP_T,
                           LOCAL_GL_CLAMP_TO_EDGE);
        textureInited = false;
    } else {
        gl->fBindTexture(aTextureTarget, aTexture);
    }

    nsIntRegion paintRegion;
    if (!textureInited) {
        paintRegion = nsIntRegion(aDstRegion.GetBounds());
    } else {
        paintRegion = aDstRegion;
    }

    GLenum format;
    GLenum internalFormat;
    GLenum type;
    int32_t pixelSize = gfxASurface::BytePerPixelFromFormat(aFormat);
    SurfaceFormat surfaceFormat;

    MOZ_ASSERT(gl->GetPreferredARGB32Format() == LOCAL_GL_BGRA ||
               gl->GetPreferredARGB32Format() == LOCAL_GL_RGBA);
    switch (aFormat) {
        case gfxImageFormatARGB32:
            if (gl->GetPreferredARGB32Format() == LOCAL_GL_BGRA) {
              format = LOCAL_GL_BGRA;
              surfaceFormat = gfx::SurfaceFormat::R8G8B8A8;
              type = LOCAL_GL_UNSIGNED_INT_8_8_8_8_REV;
            } else {
              format = LOCAL_GL_RGBA;
              surfaceFormat = gfx::SurfaceFormat::B8G8R8A8;
              type = LOCAL_GL_UNSIGNED_BYTE;
            }
            internalFormat = LOCAL_GL_RGBA;
            break;
        case gfxImageFormatRGB24:
            
            
            if (gl->GetPreferredARGB32Format() == LOCAL_GL_BGRA) {
              format = LOCAL_GL_BGRA;
              surfaceFormat = gfx::SurfaceFormat::R8G8B8X8;
              type = LOCAL_GL_UNSIGNED_INT_8_8_8_8_REV;
            } else {
              format = LOCAL_GL_RGBA;
              surfaceFormat = gfx::SurfaceFormat::B8G8R8X8;
              type = LOCAL_GL_UNSIGNED_BYTE;
            }
            internalFormat = LOCAL_GL_RGBA;
            break;
        case gfxImageFormatRGB16_565:
            internalFormat = format = LOCAL_GL_RGB;
            type = LOCAL_GL_UNSIGNED_SHORT_5_6_5;
            surfaceFormat = gfx::SurfaceFormat::R5G6B5;
            break;
        case gfxImageFormatA8:
            internalFormat = format = LOCAL_GL_LUMINANCE;
            type = LOCAL_GL_UNSIGNED_BYTE;
            
            surfaceFormat = gfx::SurfaceFormat::A8;
            break;
        default:
            NS_ASSERTION(false, "Unhandled image surface format!");
            format = 0;
            type = 0;
            surfaceFormat = gfx::SurfaceFormat::UNKNOWN;
    }

    nsIntRegionRectIterator iter(paintRegion);
    const nsIntRect *iterRect;

    
    nsIntPoint topLeft = paintRegion.GetBounds().TopLeft();

    while ((iterRect = iter.Next())) {
        
        
        
        unsigned char *rectData =
            aData + DataOffset(iterRect->TopLeft() - topLeft, aStride, aFormat);

        NS_ASSERTION(textureInited || (iterRect->x == 0 && iterRect->y == 0),
                     "Must be uploading to the origin when we don't have an existing texture");

        if (textureInited && CanUploadSubTextures(gl)) {
            TexSubImage2DHelper(gl,
                                aTextureTarget,
                                0,
                                iterRect->x,
                                iterRect->y,
                                iterRect->width,
                                iterRect->height,
                                aStride,
                                pixelSize,
                                format,
                                type,
                                rectData);
        } else {
            TexImage2DHelper(gl,
                             aTextureTarget,
                             0,
                             internalFormat,
                             iterRect->width,
                             iterRect->height,
                             aStride,
                             pixelSize,
                             0,
                             format,
                             type,
                             rectData);
        }

    }

    return surfaceFormat;
}

SurfaceFormat
UploadSurfaceToTexture(GLContext* gl,
                       gfxASurface *aSurface,
                       const nsIntRegion& aDstRegion,
                       GLuint& aTexture,
                       bool aOverwrite,
                       const nsIntPoint& aSrcPoint,
                       bool aPixelBuffer,
                       GLenum aTextureUnit,
                       GLenum aTextureTarget)
{

    nsRefPtr<gfxImageSurface> imageSurface = aSurface->GetAsImageSurface();
    unsigned char* data = nullptr;

    if (!imageSurface ||
        (imageSurface->Format() != gfxImageFormatARGB32 &&
         imageSurface->Format() != gfxImageFormatRGB24 &&
         imageSurface->Format() != gfxImageFormatRGB16_565 &&
         imageSurface->Format() != gfxImageFormatA8)) {
        
        nsIntRect bounds = aDstRegion.GetBounds();
        imageSurface =
          new gfxImageSurface(gfxIntSize(bounds.width, bounds.height),
                              gfxImageFormatARGB32);

        nsRefPtr<gfxContext> context = new gfxContext(imageSurface);

        context->Translate(-gfxPoint(aSrcPoint.x, aSrcPoint.y));
        context->SetSource(aSurface);
        context->Paint();
        data = imageSurface->Data();
        NS_ASSERTION(!aPixelBuffer,
                     "Must be using an image compatible surface with pixel buffers!");
    } else {
        
        
        if (!aPixelBuffer) {
              data = imageSurface->Data();
        }
        data += DataOffset(aSrcPoint, imageSurface->Stride(),
                           imageSurface->Format());
    }

    MOZ_ASSERT(imageSurface);
    imageSurface->Flush();

    return UploadImageDataToTexture(gl,
                                    data,
                                    imageSurface->Stride(),
                                    imageSurface->Format(),
                                    aDstRegion, aTexture, aOverwrite,
                                    aPixelBuffer, aTextureUnit, aTextureTarget);
}

SurfaceFormat
UploadSurfaceToTexture(GLContext* gl,
                       gfx::DataSourceSurface *aSurface,
                       const nsIntRegion& aDstRegion,
                       GLuint& aTexture,
                       bool aOverwrite,
                       const nsIntPoint& aSrcPoint,
                       bool aPixelBuffer,
                       GLenum aTextureUnit,
                       GLenum aTextureTarget)
{
    unsigned char* data = aPixelBuffer ? nullptr : aSurface->GetData();
    int32_t stride = aSurface->Stride();
    gfxImageFormat format =
        ImageFormatForSurfaceFormat(aSurface->GetFormat());
    data += DataOffset(aSrcPoint, stride, format);
    return UploadImageDataToTexture(gl, data, stride, format,
                                    aDstRegion, aTexture, aOverwrite,
                                    aPixelBuffer, aTextureUnit,
                                    aTextureTarget);
}

bool
CanUploadNonPowerOfTwo(GLContext* gl)
{
    if (!gl->WorkAroundDriverBugs())
        return true;

    
    return gl->Renderer() != GLRenderer::Adreno200 &&
           gl->Renderer() != GLRenderer::Adreno205;
}

}
}