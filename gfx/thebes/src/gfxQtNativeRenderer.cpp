




































#include "gfxQtNativeRenderer.h"
#include "gfxContext.h"

#include "gfxQtPlatform.h"

#include "cairo.h"
#include <QWidget>

typedef struct {
    gfxQtNativeRenderer* mRenderer;
    nsresult               mRV;
} NativeRenderingClosure;


static cairo_bool_t
NativeRendering(void *closure,
                QWidget * drawable,
                short offset_x, short offset_y,
                QRect * rectangles, unsigned int num_rects)
{
    NativeRenderingClosure* cl = (NativeRenderingClosure*)closure;
    nsresult rv = cl->mRenderer->
        NativeDraw(drawable, offset_x, offset_y,
                   rectangles, num_rects);
    cl->mRV = rv;
    return NS_SUCCEEDED(rv);
}


nsresult
gfxQtNativeRenderer::Draw(gfxContext* ctx, int width, int height,
                          PRUint32 flags, DrawOutput* output)
{
    NativeRenderingClosure closure = { this, NS_OK };

    if (output) {
        output->mSurface = NULL;
        output->mUniformAlpha = PR_FALSE;
        output->mUniformColor = PR_FALSE;
    }

#if 0 

    cairo_gdk_drawing_result_t result;
    
    
    result.surface = NULL;

    int cairoFlags = 0;
    if (flags & DRAW_SUPPORTS_OFFSET) {
        cairoFlags |= CAIRO_GDK_DRAWING_SUPPORTS_OFFSET;
    }
    if (flags & DRAW_SUPPORTS_CLIP_RECT) {
        cairoFlags |= CAIRO_GDK_DRAWING_SUPPORTS_CLIP_RECT;
    }
    if (flags & DRAW_SUPPORTS_CLIP_LIST) {
        cairoFlags |= CAIRO_GDK_DRAWING_SUPPORTS_CLIP_LIST;
    }
    if (flags & DRAW_SUPPORTS_ALTERNATE_SCREEN) {
        cairoFlags |= CAIRO_GDK_DRAWING_SUPPORTS_ALTERNATE_SCREEN;
    }
    if (flags & DRAW_SUPPORTS_NONDEFAULT_VISUAL) {
        cairoFlags |= CAIRO_GDK_DRAWING_SUPPORTS_NONDEFAULT_VISUAL;
    }

    cairo_draw_with_gdk(ctx->GetCairo(),
                        gfxPlatformGtk::GetPlatform()->GetGdkDrawable(ctx->OriginalSurface()),
                        NativeRendering, 
                        &closure, width, height,
                        (flags & DRAW_IS_OPAQUE) ? CAIRO_GDK_DRAWING_OPAQUE : CAIRO_GDK_DRAWING_TRANSPARENT,
                        (cairo_gdk_drawing_support_t)cairoFlags,
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
#endif

    return NS_OK;
}
