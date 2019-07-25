


































#include "gfxD2DSurface.h"
#include "cairo.h"
#include "cairo-win32.h"

gfxD2DSurface::gfxD2DSurface(HWND aWnd, gfxContentType aContent)
{
    Init(cairo_d2d_surface_create_for_hwnd(aWnd, (cairo_content_t)aContent));
}

gfxD2DSurface::gfxD2DSurface(cairo_surface_t *csurf)
{
    Init(csurf, PR_TRUE);
}

gfxD2DSurface::gfxD2DSurface(const gfxIntSize& size,
                             gfxImageFormat imageFormat)
{
    Init(cairo_d2d_surface_create((cairo_format_t)imageFormat, size.width, size.height));
}

gfxD2DSurface::~gfxD2DSurface()
{
}

void
gfxD2DSurface::Present()
{
    cairo_d2d_present_backbuffer(CairoSurface());
}

void
gfxD2DSurface::Scroll(const nsIntPoint &aDelta, const nsIntRect &aClip)
{
    cairo_rectangle_t rect;
    rect.x = aClip.x;
    rect.y = aClip.y;
    rect.width = aClip.width;
    rect.height = aClip.height;
    cairo_d2d_scroll(CairoSurface(), aDelta.x, aDelta.y, &rect);
}

HDC
gfxD2DSurface::GetDC(PRBool aRetainContents)
{
    return cairo_d2d_get_dc(CairoSurface(), aRetainContents);
}

void
gfxD2DSurface::ReleaseDC(const nsIntRect *aUpdatedRect)
{
    if (!aUpdatedRect) {
        return cairo_d2d_release_dc(CairoSurface(), NULL);
    }

    cairo_rectangle_int_t rect;
    rect.x = aUpdatedRect->x;
    rect.y = aUpdatedRect->y;
    rect.width = aUpdatedRect->width;
    rect.height = aUpdatedRect->height;
    cairo_d2d_release_dc(CairoSurface(), &rect);
}
