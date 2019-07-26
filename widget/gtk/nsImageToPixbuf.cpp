




#include <gdk-pixbuf/gdk-pixbuf.h>

#include "gfxASurface.h"
#include "gfxImageSurface.h"
#include "gfxContext.h"

#include "imgIContainer.h"

#include "nsAutoPtr.h"

#include "nsImageToPixbuf.h"

NS_IMPL_ISUPPORTS1(nsImageToPixbuf, nsIImageToPixbuf)

inline unsigned char
unpremultiply (unsigned char color,
               unsigned char alpha)
{
    if (alpha == 0)
        return 0;
    
    return (color * 255 + alpha / 2) / alpha;
}

NS_IMETHODIMP_(GdkPixbuf*)
nsImageToPixbuf::ConvertImageToPixbuf(imgIContainer* aImage)
{
    return ImageToPixbuf(aImage);
}

GdkPixbuf*
nsImageToPixbuf::ImageToPixbuf(imgIContainer* aImage)
{
    nsRefPtr<gfxASurface> surface;
    aImage->GetFrame(imgIContainer::FRAME_CURRENT,
                     imgIContainer::FLAG_SYNC_DECODE,
                     getter_AddRefs(surface));

    
    
    
    
    if (!surface)
        aImage->GetFrame(imgIContainer::FRAME_CURRENT,
                         imgIContainer::FLAG_NONE,
                         getter_AddRefs(surface));

    NS_ENSURE_TRUE(surface, nullptr);

    nsRefPtr<gfxImageSurface> frame(surface->GetAsReadableARGB32ImageSurface());
    NS_ENSURE_TRUE(frame, nullptr);

    return ImgSurfaceToPixbuf(frame, frame->Width(), frame->Height());
}

GdkPixbuf*
nsImageToPixbuf::ImgSurfaceToPixbuf(gfxImageSurface* aImgSurface, int32_t aWidth, int32_t aHeight)
{
    GdkPixbuf* pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8,
                                       aWidth, aHeight);
    if (!pixbuf)
        return nullptr;

    uint32_t rowstride = gdk_pixbuf_get_rowstride (pixbuf);
    guchar* pixels = gdk_pixbuf_get_pixels (pixbuf);

    long cairoStride = aImgSurface->Stride();
    unsigned char* cairoData = aImgSurface->Data();

    gfxASurface::gfxImageFormat format = aImgSurface->Format();

    for (int32_t row = 0; row < aHeight; ++row) {
        for (int32_t col = 0; col < aWidth; ++col) {
            guchar* pixel = pixels + row * rowstride + 4 * col;

            uint32_t* cairoPixel = reinterpret_cast<uint32_t*>
                                                   ((cairoData + row * cairoStride + 4 * col));

            if (format == gfxASurface::ImageFormatARGB32) {
                const uint8_t a = (*cairoPixel >> 24) & 0xFF;
                const uint8_t r = unpremultiply((*cairoPixel >> 16) & 0xFF, a);
                const uint8_t g = unpremultiply((*cairoPixel >>  8) & 0xFF, a);
                const uint8_t b = unpremultiply((*cairoPixel >>  0) & 0xFF, a);

                *pixel++ = r;
                *pixel++ = g;
                *pixel++ = b;
                *pixel++ = a;
            } else {
                NS_ASSERTION(format == gfxASurface::ImageFormatRGB24,
                             "unexpected format");
                const uint8_t r = (*cairoPixel >> 16) & 0xFF;
                const uint8_t g = (*cairoPixel >>  8) & 0xFF;
                const uint8_t b = (*cairoPixel >>  0) & 0xFF;

                *pixel++ = r;
                *pixel++ = g;
                *pixel++ = b;
                *pixel++ = 0xFF; 
            }
        }
    }

    return pixbuf;
}

GdkPixbuf*
nsImageToPixbuf::SurfaceToPixbuf(gfxASurface* aSurface, int32_t aWidth, int32_t aHeight)
{
    if (aSurface->CairoStatus()) {
        NS_ERROR("invalid surface");
        return nullptr;
    }

    nsRefPtr<gfxImageSurface> imgSurface;
    if (aSurface->GetType() == gfxASurface::SurfaceTypeImage) {
        imgSurface = static_cast<gfxImageSurface*>
                                (static_cast<gfxASurface*>(aSurface));
    } else {
        imgSurface = new gfxImageSurface(gfxIntSize(aWidth, aHeight),
					 gfxImageSurface::ImageFormatARGB32);
                                       
        if (!imgSurface)
            return nullptr;

        nsRefPtr<gfxContext> context = new gfxContext(imgSurface);
        if (!context)
            return nullptr;

        context->SetOperator(gfxContext::OPERATOR_SOURCE);
        context->SetSource(aSurface);
        context->Paint();
    }

    return ImgSurfaceToPixbuf(imgSurface, aWidth, aHeight);
}
