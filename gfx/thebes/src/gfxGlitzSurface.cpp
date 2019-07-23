





































#include "gfxGlitzSurface.h"

gfxGlitzSurface::gfxGlitzSurface(glitz_drawable_t *drawable, glitz_surface_t *surface, PRBool takeOwnership)
    : mGlitzDrawable (drawable), mGlitzSurface(surface), mOwnsSurface(takeOwnership)
{
    cairo_surface_t *surf = cairo_glitz_surface_create (mGlitzSurface);
    Init(surf);
}

gfxGlitzSurface::~gfxGlitzSurface()
{
    if (mOwnsSurface) {
        if (mGlitzSurface) {
            glitz_surface_flush(mGlitzSurface);
            glitz_surface_destroy(mGlitzSurface);
        }

        if (mGlitzDrawable) {
            glitz_drawable_flush(mGlitzDrawable);
            glitz_drawable_finish(mGlitzDrawable);
            glitz_drawable_destroy(mGlitzDrawable);
        }
    }
}

void
gfxGlitzSurface::SwapBuffers()
{
    glitz_drawable_swap_buffers (GlitzDrawable());
}

unsigned long
gfxGlitzSurface::Width()
{
    return glitz_drawable_get_width (GlitzDrawable());
}

unsigned long
gfxGlitzSurface::Height()
{
    return glitz_drawable_get_height (GlitzDrawable());
}
