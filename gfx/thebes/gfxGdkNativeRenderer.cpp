





































#include "gfxGdkNativeRenderer.h"
#include "gfxContext.h"
#include "gfxPlatformGtk.h"

#ifdef MOZ_X11
#include <gdk/gdkx.h>
#include "cairo-xlib.h"
#include "gfxXlibSurface.h"
nsresult
gfxGdkNativeRenderer::DrawWithXlib(gfxXlibSurface* surface,
                                   nsIntPoint offset,
                                   nsIntRect* clipRects, PRUint32 numClipRects)
{
    GdkDrawable *drawable = gfxPlatformGtk::GetGdkDrawable(surface);
    if (!drawable) {
        gfxIntSize size = surface->GetSize();
        int depth = cairo_xlib_surface_get_depth(surface->CairoSurface());
        GdkScreen* screen = gdk_colormap_get_screen(mColormap);
        drawable =
            gdk_pixmap_foreign_new_for_screen(screen, surface->XDrawable(),
                                              size.width, size.height, depth);
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
                       numClipRects ? &clipRect : NULL, numClipRects);
}

void
gfxGdkNativeRenderer::Draw(gfxContext* ctx, nsIntSize size,
                           PRUint32 flags, GdkColormap* colormap)
{
    mColormap = colormap;

    Visual* visual =
        gdk_x11_visual_get_xvisual(gdk_colormap_get_visual(colormap));
    Screen* screen =
        gdk_x11_screen_get_xscreen(gdk_colormap_get_screen(colormap));

    gfxXlibNativeRenderer::Draw(ctx, size, flags, screen, visual, nsnull);
}

#endif
#ifdef MOZ_DFB

#include "cairo-gdk-utils.h"

typedef struct {
    gfxGdkNativeRenderer* mRenderer;
    nsresult               mRV;
} NativeRenderingClosure;

static cairo_bool_t
NativeRendering(void *closure,
                cairo_surface_t *surface,
                short offset_x, short offset_y,
                GdkRectangle * rectangles, unsigned int num_rects)
{
    NativeRenderingClosure* cl = (NativeRenderingClosure*)closure;
    nsRefPtr<gfxASurface> gfxSurface = gfxASurface::Wrap(surface);
    GdkDrawable *drawable = gfxPlatformGtk::GetGdkDrawable(gfxSurface);
    if (!drawable)
        return 0;

    nsresult rv = cl->mRenderer->
        DrawWithGDK(drawable, offset_x, offset_y,
                    rectangles, num_rects);
    cl->mRV = rv;
    return NS_SUCCEEDED(rv);
}

void
gfxGdkNativeRenderer::Draw(gfxContext* ctx, nsIntSize size,
                           PRUint32 flags, GdkVisual* visual)
{
    NativeRenderingClosure closure = { this, NS_OK };
    cairo_gdk_drawing_result_t result;
  
    int cairoFlags = 0;
    if (flags & DRAW_SUPPORTS_CLIP_RECT) {
        cairoFlags |= CAIRO_GDK_DRAWING_SUPPORTS_CLIP_RECT;
    }
    cairo_draw_with_gdk(ctx->GetCairo(),
                        NativeRendering, 
                        &closure, size.width, size.height,
                        (flags & DRAW_IS_OPAQUE) ? CAIRO_GDK_DRAWING_OPAQUE : CAIRO_GDK_DRAWING_TRANSPARENT,
                        (cairo_gdk_drawing_support_t)cairoFlags,
                        NULL);
    if (NS_FAILED(closure.mRV)) {
        return closure.mRV;
    }
  
    return NS_OK;
}

#endif 
