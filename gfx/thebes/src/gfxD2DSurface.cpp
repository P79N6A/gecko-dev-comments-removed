


































#include "gfxD2DSurface.h"
#include "cairo.h"
#include "cairo-win32.h"

gfxD2DSurface::gfxD2DSurface(HWND aWnd)
{
    Init(cairo_d2d_surface_create_for_hwnd(aWnd));
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
