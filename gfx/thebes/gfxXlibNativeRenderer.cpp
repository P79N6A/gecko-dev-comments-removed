




#include "gfxXlibNativeRenderer.h"

#include "gfxXlibSurface.h"
#include "gfxImageSurface.h"
#include "gfxContext.h"
#include "gfxPlatform.h"
#include "gfxAlphaRecovery.h"
#include "cairo-xlib.h"
#include "cairo-xlib-xrender.h"
#include "mozilla/gfx/BorrowedContext.h"
#include "gfx2DGlue.h"

using namespace mozilla;
using namespace mozilla::gfx;

#if 0
#include <stdio.h>
#define NATIVE_DRAWING_NOTE(m) fprintf(stderr, m)
#else
#define NATIVE_DRAWING_NOTE(m) do {} while (0)
#endif



































static cairo_bool_t
_convert_coord_to_int (double coord, int32_t *v)
{
    *v = (int32_t)coord;
    
    return *v == coord;
}

static bool
_get_rectangular_clip (cairo_t *cr,
                       const IntRect& bounds,
                       bool *need_clip,
                       IntRect *rectangles, int max_rectangles,
                       int *num_rectangles)
{
    cairo_rectangle_list_t *cliplist;
    cairo_rectangle_t *clips;
    int i;
    bool retval = true;

    cliplist = cairo_copy_clip_rectangle_list (cr);
    if (cliplist->status != CAIRO_STATUS_SUCCESS) {
        retval = false;
        NATIVE_DRAWING_NOTE("FALLBACK: non-rectangular clip");
        goto FINISH;
    }

    
    clips = cliplist->rectangles;

    for (i = 0; i < cliplist->num_rectangles; ++i) {

        IntRect rect;
        if (!_convert_coord_to_int (clips[i].x, &rect.x) ||
            !_convert_coord_to_int (clips[i].y, &rect.y) ||
            !_convert_coord_to_int (clips[i].width, &rect.width) ||
            !_convert_coord_to_int (clips[i].height, &rect.height))
        {
            retval = false;
            NATIVE_DRAWING_NOTE("FALLBACK: non-integer clip");
            goto FINISH;
        }

        if (rect.IsEqualInterior(bounds)) {
            
            *need_clip = false;
            goto FINISH;
        }

        NS_ASSERTION(bounds.Contains(rect),
                     "Was expecting to be clipped to bounds");

        if (i >= max_rectangles) {
            retval = false;
            NATIVE_DRAWING_NOTE("FALLBACK: unsupported clip rectangle count");
            goto FINISH;
        }

        rectangles[i] = rect;
    }
  
    *need_clip = true;
    *num_rectangles = cliplist->num_rectangles;

FINISH:
    cairo_rectangle_list_destroy (cliplist);

    return retval;
}

#define MAX_STATIC_CLIP_RECTANGLES 50





bool
gfxXlibNativeRenderer::DrawDirect(gfxContext *ctx, nsIntSize size,
                                  uint32_t flags,
                                  Screen *screen, Visual *visual)
{
    
    
    BorrowedCairoContext borrowed(ctx->GetDrawTarget());
    if (!borrowed.mCairo) {
      return false;
    }

    bool direct = DrawCairo(borrowed.mCairo, size, flags, screen, visual);
    borrowed.Finish();

    return direct;
}

bool
gfxXlibNativeRenderer::DrawCairo(cairo_t* cr, nsIntSize size,
                                 uint32_t flags,
                                 Screen *screen, Visual *visual)
{
    
    cairo_surface_t *target = cairo_get_group_target (cr);
    if (cairo_surface_get_type (target) != CAIRO_SURFACE_TYPE_XLIB) {
        NATIVE_DRAWING_NOTE("FALLBACK: non-X surface");
        return false;
    }

    cairo_matrix_t matrix;
    cairo_get_matrix (cr, &matrix);
    double device_offset_x, device_offset_y;
    cairo_surface_get_device_offset (target, &device_offset_x, &device_offset_y);

    


    NS_ASSERTION(int32_t(device_offset_x) == device_offset_x &&
                 int32_t(device_offset_y) == device_offset_y,
                 "Expected integer device offsets");
    IntPoint offset(NS_lroundf(matrix.x0 + device_offset_x),
                      NS_lroundf(matrix.y0 + device_offset_y));

    int max_rectangles = 0;
    if (flags & DRAW_SUPPORTS_CLIP_RECT) {
      max_rectangles = 1;
    }
    if (flags & DRAW_SUPPORTS_CLIP_LIST) {
      max_rectangles = MAX_STATIC_CLIP_RECTANGLES;
    }

    

    IntRect bounds(offset, size);
    bounds.IntersectRect(bounds,
                         IntRect(0, 0,
                                   cairo_xlib_surface_get_width(target),
                                   cairo_xlib_surface_get_height(target)));

    bool needs_clip = true;
    IntRect rectangles[MAX_STATIC_CLIP_RECTANGLES];
    int rect_count = 0;

    
    


    cairo_identity_matrix (cr);
    cairo_translate (cr, -device_offset_x, -device_offset_y);
    bool have_rectangular_clip =
        _get_rectangular_clip (cr, bounds, &needs_clip,
                               rectangles, max_rectangles, &rect_count);
    cairo_set_matrix (cr, &matrix);
    if (!have_rectangular_clip)
        return false;

    
    if (needs_clip && rect_count == 0)
        return true;

    


    bool supports_alternate_visual =
        (flags & DRAW_SUPPORTS_ALTERNATE_VISUAL) != 0;
    bool supports_alternate_screen = supports_alternate_visual &&
        (flags & DRAW_SUPPORTS_ALTERNATE_SCREEN);
    if (!supports_alternate_screen &&
        cairo_xlib_surface_get_screen (target) != screen) {
        NATIVE_DRAWING_NOTE("FALLBACK: non-default screen");
        return false;
    }

    
    Visual *target_visual = cairo_xlib_surface_get_visual (target);
    if (!target_visual) {
        NATIVE_DRAWING_NOTE("FALLBACK: no Visual for surface");
        return false;
    }
    
    if (!supports_alternate_visual && target_visual != visual) {
        
        
        XRenderPictFormat *target_format =
            cairo_xlib_surface_get_xrender_format (target);
        if (!target_format ||
            (target_format !=
             XRenderFindVisualFormat (DisplayOfScreen(screen), visual))) {
            NATIVE_DRAWING_NOTE("FALLBACK: unsupported Visual");
            return false;
        }
    }

    
    NATIVE_DRAWING_NOTE("TAKING FAST PATH\n");
    cairo_surface_flush (target);
    nsresult rv = DrawWithXlib(target,
                               offset, rectangles,
                               needs_clip ? rect_count : 0);
    if (NS_SUCCEEDED(rv)) {
        cairo_surface_mark_dirty (target);
        return true;
    }
    return false;
}

static bool
VisualHasAlpha(Screen *screen, Visual *visual) {
    
    
    return visual->c_class == TrueColor &&
        visual->bits_per_rgb == 8 &&
        visual->red_mask == 0xff0000 &&
        visual->green_mask == 0xff00 &&
        visual->blue_mask == 0xff &&
        gfxXlibSurface::DepthOfVisual(screen, visual) == 32;
}



static bool
FormatConversionIsExact(Screen *screen, Visual *visual, XRenderPictFormat *format) {
    if (!format ||
        visual->c_class != TrueColor ||
        format->type != PictTypeDirect ||
        gfxXlibSurface::DepthOfVisual(screen, visual) != format->depth)
        return false;

    XRenderPictFormat *visualFormat =
        XRenderFindVisualFormat(DisplayOfScreen(screen), visual);

    if (visualFormat->type != PictTypeDirect )
        return false;

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

static cairo_surface_t*
CreateTempXlibSurface (cairo_surface_t* cairoTarget,
                       DrawTarget* drawTarget,
                       nsIntSize size,
                       bool canDrawOverBackground,
                       uint32_t flags, Screen *screen, Visual *visual,
                       DrawingMethod *method)
{
    NS_ASSERTION(cairoTarget || drawTarget, "Must have some type");

    bool drawIsOpaque = (flags & gfxXlibNativeRenderer::DRAW_IS_OPAQUE) != 0;
    bool supportsAlternateVisual =
        (flags & gfxXlibNativeRenderer::DRAW_SUPPORTS_ALTERNATE_VISUAL) != 0;
    bool supportsAlternateScreen = supportsAlternateVisual &&
        (flags & gfxXlibNativeRenderer::DRAW_SUPPORTS_ALTERNATE_SCREEN);

    cairo_surface_type_t cairoTargetType =
        cairoTarget ? cairo_surface_get_type (cairoTarget) : (cairo_surface_type_t)0xFF;

    Screen *target_screen = cairoTargetType == CAIRO_SURFACE_TYPE_XLIB ?
        cairo_xlib_surface_get_screen (cairoTarget) : screen;

    
    
    
    
    bool doCopyBackground = !drawIsOpaque && canDrawOverBackground &&
        cairoTarget && cairo_surface_get_content (cairoTarget) == CAIRO_CONTENT_COLOR;

    if (supportsAlternateScreen && screen != target_screen && drawIsOpaque) {
        
        
        visual = DefaultVisualOfScreen(target_screen);
        screen = target_screen;

    } else if (doCopyBackground || (supportsAlternateVisual && drawIsOpaque)) {
        
        
        
        Visual *target_visual = nullptr;
        XRenderPictFormat *target_format = nullptr;
        if (cairoTargetType == CAIRO_SURFACE_TYPE_XLIB) {
            target_visual = cairo_xlib_surface_get_visual (cairoTarget);
            target_format = cairo_xlib_surface_get_xrender_format (cairoTarget);
        } else if (cairoTargetType == CAIRO_SURFACE_TYPE_IMAGE || drawTarget) {
            gfxImageFormat imageFormat =
                drawTarget ? SurfaceFormatToImageFormat(drawTarget->GetFormat()) :
                    (gfxImageFormat)cairo_image_surface_get_format(cairoTarget);
            target_visual = gfxXlibSurface::FindVisual(screen, imageFormat);
            Display *dpy = DisplayOfScreen(screen);
            if (target_visual) {
                target_format = XRenderFindVisualFormat(dpy, target_visual);
            } else {
                target_format =
                    gfxXlibSurface::FindRenderFormat(dpy, imageFormat);
            }
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
            doCopyBackground = false;
        }
    }

    if (supportsAlternateVisual && !drawIsOpaque &&
        (screen != target_screen ||
         !(doCopyBackground || VisualHasAlpha(screen, visual)))) {
        
        Screen *visualScreen =
            supportsAlternateScreen ? target_screen : screen;
        Visual *argbVisual =
            gfxXlibSurface::FindVisual(visualScreen,
                                       gfxImageFormat::ARGB32);
        if (argbVisual) {
            visual = argbVisual;
            screen = visualScreen;
        } else if (!doCopyBackground &&
                   gfxXlibSurface::DepthOfVisual(screen, visual) != 24) {
            
            
            Visual *rgb24Visual =
                gfxXlibSurface::FindVisual(screen,
                                           gfxImageFormat::RGB24);
            if (rgb24Visual) {
                visual = rgb24Visual;
            }
        }
    }

    Drawable drawable =
        (screen == target_screen && cairoTargetType == CAIRO_SURFACE_TYPE_XLIB) ?
        cairo_xlib_surface_get_drawable (cairoTarget) : RootWindowOfScreen(screen);

    cairo_surface_t *surface =
        gfxXlibSurface::CreateCairoSurface(screen, visual,
                                           gfxIntSize(size.width, size.height),
                                           drawable);
    if (!surface) {
        return nullptr;
    }

    if (drawIsOpaque ||
        cairo_surface_get_content(surface) == CAIRO_CONTENT_COLOR_ALPHA) {
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

    return surface;
}

bool
gfxXlibNativeRenderer::DrawOntoTempSurface(cairo_surface_t *tempXlibSurface,
                                           IntPoint offset)
{
    cairo_surface_flush(tempXlibSurface);
    

    nsresult rv = DrawWithXlib(tempXlibSurface, offset, nullptr, 0);
    cairo_surface_mark_dirty(tempXlibSurface);
    return NS_SUCCEEDED(rv);
}

static already_AddRefed<gfxImageSurface>
CopyXlibSurfaceToImage(cairo_surface_t *tempXlibSurface,
                       gfxIntSize size,
                       gfxImageFormat format)
{
    nsRefPtr<gfxImageSurface> result = new gfxImageSurface(size, format);

    cairo_t* copyCtx = cairo_create(result->CairoSurface());
    cairo_set_source_surface(copyCtx, tempXlibSurface, 0, 0);
    cairo_set_operator(copyCtx, CAIRO_OPERATOR_SOURCE);
    cairo_paint(copyCtx);
    cairo_destroy(copyCtx);

    return result.forget();
}

void
gfxXlibNativeRenderer::Draw(gfxContext* ctx, nsIntSize size,
                            uint32_t flags, Screen *screen, Visual *visual)
{
    gfxMatrix matrix = ctx->CurrentMatrix();

    
    
    
    
    bool matrixIsIntegerTranslation = !matrix.HasNonIntegerTranslation();
    bool canDrawOverBackground = matrixIsIntegerTranslation &&
        ctx->CurrentOperator() == gfxContext::OPERATOR_OVER;

    
    
    const gfxFloat filterRadius = 0.5;
    gfxRect affectedRect(0.0, 0.0, size.width, size.height);
    if (!matrixIsIntegerTranslation) {
        
        
        affectedRect.Inflate(filterRadius);

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

    IntRect drawingRect(IntPoint(0, 0), size);
    
    
    if (!matrixIsIntegerTranslation) {
        
        
        clipExtents.Inflate(filterRadius);
    }
    clipExtents.RoundOut();

    IntRect intExtents(int32_t(clipExtents.X()),
                         int32_t(clipExtents.Y()),
                         int32_t(clipExtents.Width()),
                         int32_t(clipExtents.Height()));
    drawingRect.IntersectRect(drawingRect, intExtents);

    gfxPoint offset(drawingRect.x, drawingRect.y);

    DrawingMethod method;
    DrawTarget* drawTarget = ctx->GetDrawTarget();
    Matrix dtTransform = drawTarget->GetTransform();
    gfxPoint deviceTranslation = gfxPoint(dtTransform._31, dtTransform._32);
    cairo_surface_t* cairoTarget = static_cast<cairo_surface_t*>
            (drawTarget->GetNativeSurface(NativeSurfaceType::CAIRO_SURFACE));

    cairo_surface_t* tempXlibSurface =
        CreateTempXlibSurface(cairoTarget, drawTarget, size,
                              canDrawOverBackground, flags, screen, visual,
                              &method);
    if (!tempXlibSurface)
        return;

    bool drawIsOpaque = (flags & DRAW_IS_OPAQUE) != 0;
    if (!drawIsOpaque) {
        cairo_t* tmpCtx = cairo_create(tempXlibSurface);
        if (method == eCopyBackground) {
            NS_ASSERTION(cairoTarget, "eCopyBackground only used when there's a cairoTarget");
            cairo_set_operator(tmpCtx, CAIRO_OPERATOR_SOURCE);
            gfxPoint pt = -(offset + deviceTranslation);
            cairo_set_source_surface(tmpCtx, cairoTarget, pt.x, pt.y);
            
            
            
            
            NS_ASSERTION(cairo_surface_get_content(tempXlibSurface) == CAIRO_CONTENT_COLOR,
                         "Don't copy background with a transparent surface");
        } else {
            cairo_set_operator(tmpCtx, CAIRO_OPERATOR_CLEAR);
        }
        cairo_paint(tmpCtx);
        cairo_destroy(tmpCtx);
    }

    if (!DrawOntoTempSurface(tempXlibSurface, -drawingRect.TopLeft())) {
        cairo_surface_destroy(tempXlibSurface);
        return;
    }

    SurfaceFormat moz2DFormat =
        cairo_surface_get_content(tempXlibSurface) == CAIRO_CONTENT_COLOR ?
            SurfaceFormat::B8G8R8A8 : SurfaceFormat::B8G8R8X8;
    if (method != eAlphaExtraction) {
        if (drawTarget) {
            NativeSurface native;
            native.mFormat = moz2DFormat;
            native.mType = NativeSurfaceType::CAIRO_SURFACE;
            native.mSurface = tempXlibSurface;
            native.mSize = size;
            RefPtr<SourceSurface> sourceSurface =
                drawTarget->CreateSourceSurfaceFromNativeSurface(native);
            if (sourceSurface) {
                drawTarget->DrawSurface(sourceSurface,
                    Rect(offset.x, offset.y, size.width, size.height),
                    Rect(0, 0, size.width, size.height));
            }
        } else {
            nsRefPtr<gfxASurface> tmpSurf = gfxASurface::Wrap(tempXlibSurface);
            ctx->SetSource(tmpSurf, offset);
            ctx->Paint();
        }
        cairo_surface_destroy(tempXlibSurface);
        return;
    }
    
    nsRefPtr<gfxImageSurface> blackImage =
        CopyXlibSurfaceToImage(tempXlibSurface, size, gfxImageFormat::ARGB32);
    
    cairo_t* tmpCtx = cairo_create(tempXlibSurface);
    cairo_set_source_rgba(tmpCtx, 1.0, 1.0, 1.0, 1.0);
    cairo_set_operator(tmpCtx, CAIRO_OPERATOR_SOURCE);
    cairo_paint(tmpCtx);
    cairo_destroy(tmpCtx);
    DrawOntoTempSurface(tempXlibSurface, -drawingRect.TopLeft());
    nsRefPtr<gfxImageSurface> whiteImage =
        CopyXlibSurfaceToImage(tempXlibSurface, size, gfxImageFormat::RGB24);
  
    if (blackImage->CairoStatus() == CAIRO_STATUS_SUCCESS &&
        whiteImage->CairoStatus() == CAIRO_STATUS_SUCCESS) {
        if (!gfxAlphaRecovery::RecoverAlpha(blackImage, whiteImage)) {
            cairo_surface_destroy(tempXlibSurface);
            return;
        }

        gfxASurface* paintSurface = blackImage;
        if (drawTarget) {
            NativeSurface native;
            native.mFormat = moz2DFormat;
            native.mType = NativeSurfaceType::CAIRO_SURFACE;
            native.mSurface = paintSurface->CairoSurface();
            native.mSize = size;
            RefPtr<SourceSurface> sourceSurface =
                drawTarget->CreateSourceSurfaceFromNativeSurface(native);
            if (sourceSurface) {
                drawTarget->DrawSurface(sourceSurface,
                    Rect(offset.x, offset.y, size.width, size.height),
                    Rect(0, 0, size.width, size.height));
            }
        } else {
            ctx->SetSource(paintSurface, offset);
            ctx->Paint();
        }
    }
    cairo_surface_destroy(tempXlibSurface);
}
