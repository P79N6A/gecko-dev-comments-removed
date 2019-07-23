




































#include "gfxXlibNativeRenderer.h"
#include "gfxContext.h"

#include "cairo-xlib-utils.h"

typedef struct {
    gfxXlibNativeRenderer* mRenderer;
    nsresult               mRV;
} NativeRenderingClosure;

static cairo_bool_t
NativeRendering(void *closure,
                Display *dpy,
                Drawable drawable,
                Visual *visual,
                short offset_x, short offset_y,
                XRectangle* rectangles, unsigned int num_rects)
{
    NativeRenderingClosure* cl = (NativeRenderingClosure*)closure;
    nsresult rv = cl->mRenderer->
        NativeDraw(dpy, drawable, visual, offset_x, offset_y,
                   rectangles, num_rects);
    cl->mRV = rv;
    return NS_SUCCEEDED(rv);
}

nsresult
gfxXlibNativeRenderer::Draw(Display* dpy, gfxContext* ctx, int width, int height,
                            PRUint32 flags, DrawOutput* output)
{
    NativeRenderingClosure closure = { this, NS_OK };
    cairo_xlib_drawing_result_t result;
    
    
    result.surface = NULL;
  
    if (output) {
        output->mSurface = NULL;
        output->mUniformAlpha = PR_FALSE;
        output->mUniformColor = PR_FALSE;
    }

    int cairoFlags = 0;
    if (flags & DRAW_SUPPORTS_OFFSET) {
        cairoFlags |= CAIRO_XLIB_DRAWING_SUPPORTS_OFFSET;
    }
    if (flags & DRAW_SUPPORTS_CLIP_RECT) {
        cairoFlags |= CAIRO_XLIB_DRAWING_SUPPORTS_CLIP_RECT;
    }
    if (flags & DRAW_SUPPORTS_CLIP_LIST) {
        cairoFlags |= CAIRO_XLIB_DRAWING_SUPPORTS_CLIP_LIST;
    }
    if (flags & DRAW_SUPPORTS_ALTERNATE_DISPLAY) {
        cairoFlags |= CAIRO_XLIB_DRAWING_SUPPORTS_ALTERNATE_DISPLAY;
    }
    if (flags & DRAW_SUPPORTS_NONDEFAULT_VISUAL) {
        cairoFlags |= CAIRO_XLIB_DRAWING_SUPPORTS_NONDEFAULT_VISUAL;
    }
    cairo_draw_with_xlib(ctx->GetCairo(), NativeRendering, &closure, dpy,
                         width, height,
                         (flags & DRAW_IS_OPAQUE) ? CAIRO_XLIB_DRAWING_OPAQUE
                                                  : CAIRO_XLIB_DRAWING_TRANSPARENT,
                         (cairo_xlib_drawing_support_t)cairoFlags,
                         output ? &result : NULL);
    if (NS_FAILED(closure.mRV)) {
        if (result.surface) {
            NS_ASSERTION(output, "How did that happen?");
            cairo_surface_destroy (result.surface);
        }
        return closure.mRV;
    }

    if (output) {
        if (result.surface) {
            output->mSurface = gfxASurface::Wrap(result.surface);
            if (!output->mSurface) {
                cairo_surface_destroy (result.surface);
                return NS_ERROR_OUT_OF_MEMORY;
            }
        }

        output->mUniformAlpha = result.uniform_alpha;
        output->mUniformColor = result.uniform_color;
        output->mColor = gfxRGBA(result.r, result.g, result.b, result.alpha);
    }
  
    return NS_OK;
}
