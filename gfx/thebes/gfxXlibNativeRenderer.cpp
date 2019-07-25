






































#include "gfxXlibNativeRenderer.h"

#include "gfxXlibSurface.h"
#include "gfxContext.h"
#include "cairo-xlib.h"
#include "cairo-xlib-xrender.h"
#include <stdlib.h>

#if   HAVE_STDINT_H
#include <stdint.h>
#elif HAVE_INTTYPES_H
#include <inttypes.h>
#elif HAVE_SYS_INT_TYPES_H
#include <sys/int_types.h>
#endif

#if 0
#include <stdio.h>
#define NATIVE_DRAWING_NOTE(m) fprintf(stderr, m)
#else
#define NATIVE_DRAWING_NOTE(m) do {} while (0)
#endif















static cairo_bool_t
_convert_coord_to_int (double coord, PRInt32 *v)
{
    *v = (PRInt32)coord;
    
    return *v == coord;
}

static PRBool
_get_rectangular_clip (cairo_t *cr,
                       const nsIntRect& bounds,
                       PRBool *need_clip,
                       nsIntRect *rectangles, int max_rectangles,
                       int *num_rectangles)
{
    cairo_rectangle_list_t *cliplist;
    cairo_rectangle_t *clips;
    int i;
    PRBool retval = PR_TRUE;

    cliplist = cairo_copy_clip_rectangle_list (cr);
    if (cliplist->status != CAIRO_STATUS_SUCCESS) {
        retval = PR_FALSE;
        NATIVE_DRAWING_NOTE("TAKING SLOW PATH: non-rectangular clip\n");
        goto FINISH;
    }

    
    clips = cliplist->rectangles;

    for (i = 0; i < cliplist->num_rectangles; ++i) {
        
        nsIntRect rect;
        if (!_convert_coord_to_int (clips[i].x, &rect.x) ||
            !_convert_coord_to_int (clips[i].y, &rect.y) ||
            !_convert_coord_to_int (clips[i].width, &rect.width) ||
            !_convert_coord_to_int (clips[i].height, &rect.height))
        {
            retval = PR_FALSE;
            NATIVE_DRAWING_NOTE("TAKING SLOW PATH: non-integer clip\n");
            goto FINISH;
        }

        if (rect == bounds) {
            
            *need_clip = PR_FALSE;
            goto FINISH;
        }            

        NS_ASSERTION(bounds.Contains(rect),
                     "Was expecting to be clipped to bounds");

        if (i >= max_rectangles) {
            retval = PR_FALSE;
            NATIVE_DRAWING_NOTE("TAKING SLOW PATH: unsupported clip rectangle count\n");
            goto FINISH;
        }

        rectangles[i] = rect;
    }
  
    *need_clip = PR_TRUE;
    *num_rectangles = cliplist->num_rectangles;

FINISH:
    cairo_rectangle_list_destroy (cliplist);

    return retval;
}

#define MAX_STATIC_CLIP_RECTANGLES 50





PRBool
gfxXlibNativeRenderer::DrawDirect(gfxContext *ctx, nsIntSize size,
                                  PRUint32 flags,
                                  Screen *screen, Visual *visual)
{
    cairo_t *cr = ctx->GetCairo();

    
    if (cairo_get_operator (cr) != CAIRO_OPERATOR_OVER) {
        NATIVE_DRAWING_NOTE("TAKING SLOW PATH: non-OVER operator\n");
        return PR_FALSE;
    }
    
    
    cairo_surface_t *target = cairo_get_group_target (cr);
    if (cairo_surface_get_type (target) != CAIRO_SURFACE_TYPE_XLIB) {
        NATIVE_DRAWING_NOTE("TAKING SLOW PATH: non-X surface\n");
        return PR_FALSE;
    }
    
    

  
    PRBool supports_alternate_visual =
        (flags & DRAW_SUPPORTS_ALTERNATE_VISUAL) != 0;
    PRBool supports_alternate_screen = supports_alternate_visual
        && (flags & DRAW_SUPPORTS_ALTERNATE_SCREEN);
    if (!supports_alternate_screen &&
        cairo_xlib_surface_get_screen (target) != screen) {
        NATIVE_DRAWING_NOTE("TAKING SLOW PATH: non-default screen\n");
        return PR_FALSE;
    }
        
    
    Visual *target_visual = cairo_xlib_surface_get_visual (target);
    if (!target_visual) {
        NATIVE_DRAWING_NOTE("TAKING SLOW PATH: no Visual for surface\n");
        return PR_FALSE;
    }        
    
    if (!supports_alternate_visual && target_visual != visual) {
        
        
        XRenderPictFormat *target_format =
            cairo_xlib_surface_get_xrender_format (target);
        if (!target_format ||
            (target_format !=
             XRenderFindVisualFormat (DisplayOfScreen(screen), visual))) {
            NATIVE_DRAWING_NOTE("TAKING SLOW PATH: unsupported Visual\n");
            return PR_FALSE;
        }
    }
  
    cairo_matrix_t matrix;
    cairo_get_matrix (cr, &matrix);
    double device_offset_x, device_offset_y;
    cairo_surface_get_device_offset (target, &device_offset_x, &device_offset_y);

    


    NS_ASSERTION(PRUint32(device_offset_x) == device_offset_x &&
                 PRUint32(device_offset_y) == device_offset_y,
                 "Expected integer device offsets");
    nsIntPoint offset(NS_lroundf(matrix.x0 + device_offset_x),
                      NS_lroundf(matrix.y0 + device_offset_y));
    
    int max_rectangles = 0;
    if (flags & DRAW_SUPPORTS_CLIP_RECT) {
      max_rectangles = 1;
    }
    if (flags & DRAW_SUPPORTS_CLIP_LIST) {
      max_rectangles = MAX_STATIC_CLIP_RECTANGLES;
    }

    

    nsIntRect bounds(offset, size);
    bounds.IntersectRect(bounds,
                         nsIntRect(0, 0,
                                   cairo_xlib_surface_get_width(target),
                                   cairo_xlib_surface_get_height(target)));

    PRBool needs_clip;
    nsIntRect rectangles[MAX_STATIC_CLIP_RECTANGLES];
    int rect_count;

    
    


    cairo_identity_matrix (cr);
    cairo_translate (cr, -device_offset_x, -device_offset_y);
    PRBool have_rectangular_clip =
        _get_rectangular_clip (cr, bounds, &needs_clip,
                               rectangles, max_rectangles, &rect_count);
    cairo_set_matrix (cr, &matrix);
    if (!have_rectangular_clip)
        return PR_FALSE;

    
    NS_ASSERTION(!needs_clip || rect_count != 0,
                 "Where did the clip region go?");
      
    
    NATIVE_DRAWING_NOTE("TAKING FAST PATH\n");
    cairo_surface_flush (target);
    nsRefPtr<gfxASurface> surface = gfxASurface::Wrap(target);
    nsresult rv = DrawWithXlib(static_cast<gfxXlibSurface*>(surface.get()),
                               offset, rectangles,
                               needs_clip ? rect_count : 0);
    if (NS_SUCCEEDED(rv)) {
        cairo_surface_mark_dirty (target);
        return PR_TRUE;
    }
    return PR_FALSE;
}

static PRBool
FormatHasAlpha(const XRenderPictFormat *format) {
    if (!format)
        return false;

    if (format->type != PictTypeDirect)
        return false;

    return format->direct.alphaMask != 0;
}

static already_AddRefed<gfxXlibSurface>
_create_temp_xlib_surface (cairo_t *cr, nsIntSize size,
                           PRUint32 flags, Screen *screen, Visual *visual)
{
    Drawable drawable = None;

    
    
    
    cairo_surface_t *target = cairo_get_group_target (cr);
    if ((flags & gfxXlibNativeRenderer::DRAW_IS_OPAQUE)
        && cairo_surface_get_type (target) == CAIRO_SURFACE_TYPE_XLIB) {

        Screen *target_screen = cairo_xlib_surface_get_screen (target);
        PRBool supports_alternate_visual =
            (flags & gfxXlibNativeRenderer::DRAW_SUPPORTS_ALTERNATE_VISUAL) != 0;
        PRBool supports_alternate_screen = supports_alternate_visual
            && (flags & gfxXlibNativeRenderer::DRAW_SUPPORTS_ALTERNATE_SCREEN);
        if (target_screen == screen || supports_alternate_screen) {

            if (supports_alternate_visual) {
                Visual *target_visual = cairo_xlib_surface_get_visual (target);
                if (target_visual &&
                    (!FormatHasAlpha(cairo_xlib_surface_get_xrender_format (target)))) {
                    visual = target_visual;
                } else if (target_screen != screen) {
                    visual = DefaultVisualOfScreen (target_screen);
                }
            }

            drawable = cairo_xlib_surface_get_drawable (target);
            screen = target_screen;
        }
    }

    if (!drawable) {
        drawable = RootWindowOfScreen (screen);
    }
    return gfxXlibSurface::Create(screen, visual,
                                  gfxIntSize(size.width, size.height),
                                  drawable);
}

PRBool
gfxXlibNativeRenderer::DrawOntoTempSurface(gfxXlibSurface *tempXlibSurface,
                                           nsIntPoint offset)
{
    cairo_surface_t *temp_xlib_surface = tempXlibSurface->CairoSurface();
    cairo_surface_flush (temp_xlib_surface);
    

    nsresult rv = DrawWithXlib(tempXlibSurface, offset, NULL, 0);
    cairo_surface_mark_dirty (temp_xlib_surface);
    return NS_SUCCEEDED(rv);
}

static cairo_surface_t *
_copy_xlib_surface_to_image (gfxXlibSurface *tempXlibSurface,
                             cairo_format_t format,
                             int width, int height,
                             unsigned char **data_out)
{
    unsigned char *data;
    cairo_surface_t *result;
    cairo_t *cr;
    
    *data_out = data = (unsigned char*)malloc (width*height*4);
    if (!data)
        return NULL;
  
    result = cairo_image_surface_create_for_data (data, format, width, height, width*4);
    cr = cairo_create (result);
    cairo_set_source_surface (cr, tempXlibSurface->CairoSurface(), 0, 0);
    cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint (cr);
    cairo_destroy (cr);
    return result;
}

#define SET_ALPHA(v, a) (((v) & ~(0xFF << 24)) | ((a) << 24))
#define GREEN_OF(v) (((v) >> 8) & 0xFF)


















static void
_compute_alpha_values (uint32_t *black_data,
                       uint32_t *white_data,
                       int width, int height,
                       gfxXlibNativeRenderer::DrawOutput *analysis)
{
    int num_pixels = width*height;
    int i;
    uint32_t first;
    uint32_t deltas = 0;
    unsigned char first_alpha;
  
    if (num_pixels == 0) {
        if (analysis) {
            analysis->mUniformAlpha = True;
            analysis->mUniformColor = True;
            
            analysis->mColor = gfxRGBA(0.0, 0.0, 0.0, 1.0);
        }
        return;
    }
  
    first_alpha = 255 - (GREEN_OF(*white_data) - GREEN_OF(*black_data));
    
    first = SET_ALPHA(*black_data, first_alpha);
  
    for (i = 0; i < num_pixels; ++i) {
        uint32_t black = *black_data;
        uint32_t white = *white_data;
        unsigned char pixel_alpha = 255 - (GREEN_OF(white) - GREEN_OF(black));
        
        black = SET_ALPHA(black, pixel_alpha);
        *black_data = black;
        deltas |= (first ^ black);
        
        black_data++;
        white_data++;
    }
    
    if (analysis) {
        analysis->mUniformAlpha = (deltas >> 24) == 0;
        if (analysis->mUniformAlpha) {
            analysis->mColor.a = first_alpha/255.0;
            



            analysis->mUniformColor = (deltas & ~(0xFF << 24)) == 0;
            if (analysis->mUniformColor) {
                if (first_alpha == 0) {
                    
                    analysis->mColor = gfxRGBA(0.0, 0.0, 0.0, 0.0);
                } else {
                    double d_first_alpha = first_alpha;
                    analysis->mColor.r = (first & 0xFF)/d_first_alpha;
                    analysis->mColor.g = ((first >> 8) & 0xFF)/d_first_alpha;
                    analysis->mColor.b = ((first >> 16) & 0xFF)/d_first_alpha;
                }
            }
        }
    }
}

void
gfxXlibNativeRenderer::Draw(gfxContext* ctx, nsIntSize size,
                            PRUint32 flags, Screen *screen, Visual *visual,
                            DrawOutput* result)
{
    cairo_surface_t *black_image_surface;
    cairo_surface_t *white_image_surface;
    unsigned char *black_data;
    unsigned char *white_data;
  
    if (result) {
        result->mSurface = NULL;
        result->mUniformAlpha = PR_FALSE;
        result->mUniformColor = PR_FALSE;
    }
    
    PRBool matrixIsIntegerTranslation =
        !ctx->CurrentMatrix().HasNonIntegerTranslation();

    
    
    const gfxFloat filterRadius = 0.5;
    gfxRect affectedRect(0.0, 0.0, size.width, size.height);
    if (!matrixIsIntegerTranslation) {
        
        
        affectedRect.Outset(filterRadius);

        NATIVE_DRAWING_NOTE("TAKING SLOW PATH: matrix not integer translation\n");
    }
    
    
    gfxRect clipExtents;
    {
        gfxContextAutoSaveRestore autoSR(ctx);
        ctx->Clip(affectedRect);

        clipExtents = ctx->GetClipExtents();
        if (clipExtents.IsEmpty())
            return; 

        if (matrixIsIntegerTranslation &&
            DrawDirect(ctx, size, flags, screen, visual))
            return;
    }

    nsIntRect drawingRect(nsIntPoint(0, 0), size);
    PRBool drawIsOpaque = (flags & DRAW_IS_OPAQUE) != 0;
    if (drawIsOpaque || !result) {
        
        
        if (!matrixIsIntegerTranslation) {
            
            
            clipExtents.Outset(filterRadius);
        }
        clipExtents.RoundOut();

        nsIntRect intExtents(PRInt32(clipExtents.X()),
                             PRInt32(clipExtents.Y()),
                             PRInt32(clipExtents.Width()),
                             PRInt32(clipExtents.Height()));
        drawingRect.IntersectRect(drawingRect, intExtents);
    }
    gfxPoint offset(drawingRect.x, drawingRect.y);

    cairo_t *cr = ctx->GetCairo();
    nsRefPtr<gfxXlibSurface> tempXlibSurface = 
        _create_temp_xlib_surface (cr, drawingRect.Size(),
                                   flags, screen, visual);
    if (tempXlibSurface == NULL)
        return;
  
    nsRefPtr<gfxContext> tmpCtx;
    if (!drawIsOpaque) {
        tmpCtx = new gfxContext(tempXlibSurface);
        tmpCtx->SetOperator(gfxContext::OPERATOR_CLEAR);
        tmpCtx->Paint();
    }

    if (!DrawOntoTempSurface(tempXlibSurface, -drawingRect.TopLeft())) {
        return;
    }
  
    if (drawIsOpaque) {
        ctx->SetSource(tempXlibSurface, offset);
        ctx->Paint();
        if (result) {
            result->mSurface = tempXlibSurface;
            

            result->mUniformAlpha = PR_TRUE;
            result->mColor.a = 1.0;
        }
        return;
    }
    
    int width = drawingRect.width;
    int height = drawingRect.height;
    black_image_surface =
        _copy_xlib_surface_to_image (tempXlibSurface, CAIRO_FORMAT_ARGB32,
                                     width, height, &black_data);
    
    tmpCtx->SetDeviceColor(gfxRGBA(1.0, 1.0, 1.0));
    tmpCtx->SetOperator(gfxContext::OPERATOR_SOURCE);
    tmpCtx->Paint();
    DrawOntoTempSurface(tempXlibSurface, -drawingRect.TopLeft());
    white_image_surface =
        _copy_xlib_surface_to_image (tempXlibSurface, CAIRO_FORMAT_RGB24,
                                     width, height, &white_data);
  
    if (black_image_surface && white_image_surface &&
        cairo_surface_status (black_image_surface) == CAIRO_STATUS_SUCCESS &&
        cairo_surface_status (white_image_surface) == CAIRO_STATUS_SUCCESS &&
        black_data != NULL && white_data != NULL) {
        cairo_surface_flush (black_image_surface);
        cairo_surface_flush (white_image_surface);
        _compute_alpha_values ((uint32_t*)black_data, (uint32_t*)white_data, width, height, result);
        cairo_surface_mark_dirty (black_image_surface);
        
        cairo_set_source_surface (cr, black_image_surface, offset.x, offset.y);
        




        if (result && (!result->mUniformAlpha || !result->mUniformColor)) {
            cairo_surface_t *target = cairo_get_group_target (cr);
            cairo_surface_t *similar_surface =
                cairo_surface_create_similar (target, CAIRO_CONTENT_COLOR_ALPHA,
                                              width, height);
            cairo_t *copy_cr = cairo_create (similar_surface);
            cairo_set_source_surface (copy_cr, black_image_surface, 0.0, 0.0);
            cairo_set_operator (copy_cr, CAIRO_OPERATOR_SOURCE);
            cairo_paint (copy_cr);
            cairo_destroy (copy_cr);
      
            cairo_set_source_surface (cr, similar_surface, 0.0, 0.0);
            
            result->mSurface = gfxASurface::Wrap(similar_surface);
        }
        
        cairo_paint (cr);
    }
    
    if (black_image_surface) {
        cairo_surface_destroy (black_image_surface);
    }
    if (white_image_surface) {
        cairo_surface_destroy (white_image_surface);
    }
    free (black_data);
    free (white_data);
}
