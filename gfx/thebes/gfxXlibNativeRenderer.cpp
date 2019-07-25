






































#include "gfxXlibNativeRenderer.h"

#include "gfxXlibSurface.h"
#include "gfxImageSurface.h"
#include "gfxContext.h"
#include "gfxAlphaRecovery.h"
#include "cairo-xlib.h"
#include "cairo-xlib-xrender.h"

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
        NATIVE_DRAWING_NOTE("FALLBACK: non-rectangular clip");
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
            NATIVE_DRAWING_NOTE("FALLBACK: non-integer clip");
            goto FINISH;
        }

        if (rect.IsEqualInterior(bounds)) {
            
            *need_clip = PR_FALSE;
            goto FINISH;
        }            

        NS_ASSERTION(bounds.Contains(rect),
                     "Was expecting to be clipped to bounds");

        if (i >= max_rectangles) {
            retval = PR_FALSE;
            NATIVE_DRAWING_NOTE("FALLBACK: unsupported clip rectangle count");
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

    
    cairo_surface_t *target = cairo_get_group_target (cr);
    if (cairo_surface_get_type (target) != CAIRO_SURFACE_TYPE_XLIB) {
        NATIVE_DRAWING_NOTE("FALLBACK: non-X surface");
        return PR_FALSE;
    }
    
    cairo_matrix_t matrix;
    cairo_get_matrix (cr, &matrix);
    double device_offset_x, device_offset_y;
    cairo_surface_get_device_offset (target, &device_offset_x, &device_offset_y);

    


    NS_ASSERTION(PRInt32(device_offset_x) == device_offset_x &&
                 PRInt32(device_offset_y) == device_offset_y,
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

    PRBool needs_clip = PR_TRUE;
    nsIntRect rectangles[MAX_STATIC_CLIP_RECTANGLES];
    int rect_count = 0;

    
    


    cairo_identity_matrix (cr);
    cairo_translate (cr, -device_offset_x, -device_offset_y);
    PRBool have_rectangular_clip =
        _get_rectangular_clip (cr, bounds, &needs_clip,
                               rectangles, max_rectangles, &rect_count);
    cairo_set_matrix (cr, &matrix);
    if (!have_rectangular_clip)
        return PR_FALSE;

    
    if (needs_clip && rect_count == 0)
        return PR_TRUE;
      
    

  
    PRBool supports_alternate_visual =
        (flags & DRAW_SUPPORTS_ALTERNATE_VISUAL) != 0;
    PRBool supports_alternate_screen = supports_alternate_visual &&
        (flags & DRAW_SUPPORTS_ALTERNATE_SCREEN);
    if (!supports_alternate_screen &&
        cairo_xlib_surface_get_screen (target) != screen) {
        NATIVE_DRAWING_NOTE("FALLBACK: non-default screen");
        return PR_FALSE;
    }
        
    
    Visual *target_visual = cairo_xlib_surface_get_visual (target);
    if (!target_visual) {
        NATIVE_DRAWING_NOTE("FALLBACK: no Visual for surface");
        return PR_FALSE;
    }        
    
    if (!supports_alternate_visual && target_visual != visual) {
        
        
        XRenderPictFormat *target_format =
            cairo_xlib_surface_get_xrender_format (target);
        if (!target_format ||
            (target_format !=
             XRenderFindVisualFormat (DisplayOfScreen(screen), visual))) {
            NATIVE_DRAWING_NOTE("FALLBACK: unsupported Visual");
            return PR_FALSE;
        }
    }
  
    
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
VisualHasAlpha(Screen *screen, Visual *visual) {
    
    
    return visual->c_class == TrueColor &&
        visual->bits_per_rgb == 8 &&
        visual->red_mask == 0xff0000 &&
        visual->green_mask == 0xff00 &&
        visual->blue_mask == 0xff &&
        gfxXlibSurface::DepthOfVisual(screen, visual) == 32;
}



static PRBool
FormatConversionIsExact(Screen *screen, Visual *visual, XRenderPictFormat *format) {
    if (!format ||
        visual->c_class != TrueColor ||
        format->type != PictTypeDirect ||
        gfxXlibSurface::DepthOfVisual(screen, visual) != format->depth)
        return PR_FALSE;

    XRenderPictFormat *visualFormat =
        XRenderFindVisualFormat(DisplayOfScreen(screen), visual);

    if (visualFormat->type != PictTypeDirect )
        return PR_FALSE;

    const XRenderDirectFormat& a = visualFormat->direct;
    const XRenderDirectFormat& b = format->direct;
    return a.redMask == b.redMask &&
        a.greenMask == b.greenMask &&
        a.blueMask == b.blueMask;
}



enum DrawingMethod {
    eSimple,
    eCopyBackground,
    eAlphaExtraction
};

static already_AddRefed<gfxXlibSurface>
CreateTempXlibSurface (gfxASurface *destination, nsIntSize size,
                       PRBool canDrawOverBackground,
                       PRUint32 flags, Screen *screen, Visual *visual,
                       DrawingMethod *method)
{
    PRBool drawIsOpaque = (flags & gfxXlibNativeRenderer::DRAW_IS_OPAQUE) != 0;
    PRBool supportsAlternateVisual =
        (flags & gfxXlibNativeRenderer::DRAW_SUPPORTS_ALTERNATE_VISUAL) != 0;
    PRBool supportsAlternateScreen = supportsAlternateVisual &&
        (flags & gfxXlibNativeRenderer::DRAW_SUPPORTS_ALTERNATE_SCREEN);

    cairo_surface_t *target = destination->CairoSurface();
    cairo_surface_type_t target_type = cairo_surface_get_type (target);
    cairo_content_t target_content = cairo_surface_get_content (target);

    Screen *target_screen = target_type == CAIRO_SURFACE_TYPE_XLIB ?
        cairo_xlib_surface_get_screen (target) : screen;

    
    
    
    
    PRBool doCopyBackground = !drawIsOpaque && canDrawOverBackground &&
        target_content == CAIRO_CONTENT_COLOR;

    if (supportsAlternateScreen && screen != target_screen && drawIsOpaque) {
        
        
        visual = DefaultVisualOfScreen(target_screen);
        screen = target_screen;

    } else if (doCopyBackground || (supportsAlternateVisual && drawIsOpaque)) {
        
        
        
        Visual *target_visual = NULL;
        XRenderPictFormat *target_format = NULL;
        switch (target_type) {
        case CAIRO_SURFACE_TYPE_XLIB:
            target_visual = cairo_xlib_surface_get_visual (target);
            target_format = cairo_xlib_surface_get_xrender_format (target);
            break;
        case CAIRO_SURFACE_TYPE_IMAGE: {
            gfxASurface::gfxImageFormat imageFormat =
                static_cast<gfxImageSurface*>(destination)->Format();
            target_visual = gfxXlibSurface::FindVisual(screen, imageFormat);
            Display *dpy = DisplayOfScreen(screen);
            if (target_visual) {
                target_format = XRenderFindVisualFormat(dpy, target_visual);
            } else {
                target_format =
                    gfxXlibSurface::FindRenderFormat(dpy, imageFormat);
            }                
            break;
        }
        default:
            break;
        }

        if (supportsAlternateVisual &&
            (supportsAlternateScreen || screen == target_screen)) {
            if (target_visual) {
                visual = target_visual;
                screen = target_screen;
            }
        }
        
        
        
        

        if (doCopyBackground && visual != target_visual &&
            !FormatConversionIsExact(screen, visual, target_format)) {
            doCopyBackground = PR_FALSE;
        }
    }

    if (supportsAlternateVisual && !drawIsOpaque &&
        (screen != target_screen ||
         !(doCopyBackground || VisualHasAlpha(screen, visual)))) {
        
        Screen *visualScreen =
            supportsAlternateScreen ? target_screen : screen;
        Visual *argbVisual =
            gfxXlibSurface::FindVisual(visualScreen,
                                       gfxASurface::ImageFormatARGB32);
        if (argbVisual) {
            visual = argbVisual;
            screen = visualScreen;
        } else if (!doCopyBackground &&
                   gfxXlibSurface::DepthOfVisual(screen, visual) != 24) {
            
            
            Visual *rgb24Visual =
                gfxXlibSurface::FindVisual(screen,
                                           gfxASurface::ImageFormatRGB24);
            if (rgb24Visual) {
                visual = rgb24Visual;
            }
        }
    }

    Drawable drawable =
        (screen == target_screen && target_type == CAIRO_SURFACE_TYPE_XLIB) ?
        cairo_xlib_surface_get_drawable (target) : RootWindowOfScreen(screen);

    nsRefPtr<gfxXlibSurface> surface =
        gfxXlibSurface::Create(screen, visual,
                               gfxIntSize(size.width, size.height),
                               drawable);

    if (drawIsOpaque ||
        surface->GetContentType() == gfxASurface::CONTENT_COLOR_ALPHA) {
        NATIVE_DRAWING_NOTE(drawIsOpaque ?
                            ", SIMPLE OPAQUE\n" : ", SIMPLE WITH ALPHA");
        *method = eSimple;
    } else if (doCopyBackground) {
        NATIVE_DRAWING_NOTE(", COPY BACKGROUND\n");
        *method = eCopyBackground;
    } else {
        NATIVE_DRAWING_NOTE(", SLOW ALPHA EXTRACTION\n");
        *method = eAlphaExtraction;
    }

    return surface.forget();
}

PRBool
gfxXlibNativeRenderer::DrawOntoTempSurface(gfxXlibSurface *tempXlibSurface,
                                           nsIntPoint offset)
{
    tempXlibSurface->Flush();
    

    nsresult rv = DrawWithXlib(tempXlibSurface, offset, NULL, 0);
    tempXlibSurface->MarkDirty();
    return NS_SUCCEEDED(rv);
}

static already_AddRefed<gfxImageSurface>
CopyXlibSurfaceToImage(gfxXlibSurface *tempXlibSurface,
                       gfxASurface::gfxImageFormat format)
{
    nsRefPtr<gfxImageSurface> result =
        new gfxImageSurface(tempXlibSurface->GetSize(), format);

    gfxContext copyCtx(result);
    copyCtx.SetSource(tempXlibSurface);
    copyCtx.SetOperator(gfxContext::OPERATOR_SOURCE);
    copyCtx.Paint();

    return result.forget();
}

void
gfxXlibNativeRenderer::Draw(gfxContext* ctx, nsIntSize size,
                            PRUint32 flags, Screen *screen, Visual *visual,
                            DrawOutput* result)
{
    if (result) {
        result->mSurface = NULL;
        result->mUniformAlpha = PR_FALSE;
        result->mUniformColor = PR_FALSE;
    }

    PRBool drawIsOpaque = (flags & DRAW_IS_OPAQUE) != 0;
    gfxMatrix matrix = ctx->CurrentMatrix();

    
    
    
    
    PRBool matrixIsIntegerTranslation = !matrix.HasNonIntegerTranslation();
    PRBool canDrawOverBackground = matrixIsIntegerTranslation &&
        ctx->CurrentOperator() == gfxContext::OPERATOR_OVER;

    
    
    const gfxFloat filterRadius = 0.5;
    gfxRect affectedRect(0.0, 0.0, size.width, size.height);
    if (!matrixIsIntegerTranslation) {
        
        
        affectedRect.Outset(filterRadius);

        NATIVE_DRAWING_NOTE("FALLBACK: matrix not integer translation");
    } else if (!canDrawOverBackground) {
        NATIVE_DRAWING_NOTE("FALLBACK: unsupported operator");
    }

    
    
    gfxRect clipExtents;
    {
        gfxContextAutoSaveRestore autoSR(ctx);
        ctx->Clip(affectedRect);

        clipExtents = ctx->GetClipExtents();
        if (clipExtents.IsEmpty())
            return; 

        if (canDrawOverBackground &&
            DrawDirect(ctx, size, flags, screen, visual))
            return;
    }

    nsIntRect drawingRect(nsIntPoint(0, 0), size);
    
    
    if (!matrixIsIntegerTranslation) {
        
        
        clipExtents.Outset(filterRadius);
    }
    clipExtents.RoundOut();

    nsIntRect intExtents(PRInt32(clipExtents.X()),
                         PRInt32(clipExtents.Y()),
                         PRInt32(clipExtents.Width()),
                         PRInt32(clipExtents.Height()));
    drawingRect.IntersectRect(drawingRect, intExtents);
    gfxPoint offset(drawingRect.x, drawingRect.y);

    DrawingMethod method;
    nsRefPtr<gfxASurface> target = ctx->CurrentSurface();
    nsRefPtr<gfxXlibSurface> tempXlibSurface = 
        CreateTempXlibSurface(target, drawingRect.Size(),
                              canDrawOverBackground, flags, screen, visual,
                              &method);
    if (!tempXlibSurface)
        return;
  
    if (drawingRect.Size() != size || method == eCopyBackground) {
        
        
        result = NULL;
    }

    nsRefPtr<gfxContext> tmpCtx;
    if (!drawIsOpaque) {
        tmpCtx = new gfxContext(tempXlibSurface);
        if (method == eCopyBackground) {
            tmpCtx->SetOperator(gfxContext::OPERATOR_SOURCE);
            tmpCtx->SetSource(target, -(offset + matrix.GetTranslation()));
            
            
            
            
            NS_ASSERTION(tempXlibSurface->GetContentType()
                         == gfxASurface::CONTENT_COLOR,
                         "Don't copy background with a transparent surface");
        } else {
            tmpCtx->SetOperator(gfxContext::OPERATOR_CLEAR);
        }
        tmpCtx->Paint();
    }

    if (!DrawOntoTempSurface(tempXlibSurface, -drawingRect.TopLeft())) {
        return;
    }
  
    if (method != eAlphaExtraction) {
        ctx->SetSource(tempXlibSurface, offset);
        ctx->Paint();
        if (result) {
            result->mSurface = tempXlibSurface;
            

            result->mUniformAlpha = PR_TRUE;
            result->mColor.a = 1.0;
        }
        return;
    }
    
    nsRefPtr<gfxImageSurface> blackImage =
        CopyXlibSurfaceToImage(tempXlibSurface, gfxASurface::ImageFormatARGB32);
    
    tmpCtx->SetDeviceColor(gfxRGBA(1.0, 1.0, 1.0));
    tmpCtx->SetOperator(gfxContext::OPERATOR_SOURCE);
    tmpCtx->Paint();
    DrawOntoTempSurface(tempXlibSurface, -drawingRect.TopLeft());
    nsRefPtr<gfxImageSurface> whiteImage =
        CopyXlibSurfaceToImage(tempXlibSurface, gfxASurface::ImageFormatRGB24);
  
    if (blackImage->CairoStatus() == CAIRO_STATUS_SUCCESS &&
        blackImage->CairoStatus() == CAIRO_STATUS_SUCCESS) {
        gfxAlphaRecovery::Analysis analysis;
        if (!gfxAlphaRecovery::RecoverAlpha(blackImage, whiteImage,
                                            result ? &analysis : nsnull))
            return;

        ctx->SetSource(blackImage, offset);

        





        if (result) {
            if (analysis.uniformAlpha) {
                result->mUniformAlpha = PR_TRUE;
                result->mColor.a = analysis.alpha;
            }
            if (analysis.uniformColor) {
                result->mUniformColor = PR_TRUE;
                result->mColor.r = analysis.r;
                result->mColor.g = analysis.g;
                result->mColor.b = analysis.b;
            } else {
                result->mSurface = target->
                    CreateSimilarSurface(gfxASurface::CONTENT_COLOR_ALPHA,
                                         gfxIntSize(size.width, size.height));

                gfxContext copyCtx(result->mSurface);
                copyCtx.SetSource(blackImage);
                copyCtx.SetOperator(gfxContext::OPERATOR_SOURCE);
                copyCtx.Paint();

                ctx->SetSource(result->mSurface);
            }
        }
        
        ctx->Paint();
    }
}
