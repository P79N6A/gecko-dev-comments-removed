




#include "gfxGdkNativeRenderer.h"
#include "gfxContext.h"
#include "gfxPlatformGtk.h"

#ifdef MOZ_X11
#include <gdk/gdkx.h>
#include "cairo-xlib.h"
#include "gfxXlibSurface.h"

#if (MOZ_WIDGET_GTK == 2)
nsresult
gfxGdkNativeRenderer::DrawWithXlib(cairo_surface_t* surface,
                                   nsIntPoint offset,
                                   nsIntRect* clipRects, uint32_t numClipRects)
{
    GdkDrawable *drawable = gfxPlatformGtk::GetGdkDrawable(surface);
    if (!drawable) {
        int depth = cairo_xlib_surface_get_depth(surface);
        GdkScreen* screen = gdk_colormap_get_screen(mColormap);
        drawable =
            gdk_pixmap_foreign_new_for_screen(screen, cairo_xlib_surface_get_drawable(surface),
                                              cairo_xlib_surface_get_width(surface),
                                              cairo_xlib_surface_get_height(surface),
                                              depth);
        if (!drawable)
            return NS_ERROR_FAILURE;

        gdk_drawable_set_colormap(drawable, mColormap);
        gfxPlatformGtk::SetGdkDrawable(surface, drawable);
        g_object_unref(drawable); 
    }
    
    GdkRectangle clipRect;
    if (numClipRects) {
        NS_ASSERTION(numClipRects == 1, "Too many clip rects");
        clipRect.x = clipRects[0].x;
        clipRect.y = clipRects[0].y;
        clipRect.width = clipRects[0].width;
        clipRect.height = clipRects[0].height;
    }

    return DrawWithGDK(drawable, offset.x, offset.y,
                       numClipRects ? &clipRect : nullptr, numClipRects);
}

void
gfxGdkNativeRenderer::Draw(gfxContext* ctx, nsIntSize size,
                           uint32_t flags, GdkColormap* colormap)
{
    mColormap = colormap;

    Visual* visual =
        gdk_x11_visual_get_xvisual(gdk_colormap_get_visual(colormap));
    Screen* screen =
        gdk_x11_screen_get_xscreen(gdk_colormap_get_screen(colormap));

    gfxXlibNativeRenderer::Draw(ctx, size, flags, screen, visual, nullptr);
}

#else

#endif

#endif
