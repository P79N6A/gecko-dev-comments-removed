




































#include "cairo-xlib-utils.h"

#include "cairo-xlib.h"
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
#define CAIRO_XLIB_DRAWING_NOTE(m) fprintf(stderr, m)
#else
#define CAIRO_XLIB_DRAWING_NOTE(m) do {} while (0)
#endif

static cairo_user_data_key_t pixmap_free_key;

typedef struct {
    Display *dpy;
    Pixmap   pixmap;
} pixmap_free_struct;

static void pixmap_free_func (void *data)
{
    pixmap_free_struct *pfs = (pixmap_free_struct *) data;
    XFreePixmap (pfs->dpy, pfs->pixmap);
    free (pfs);
}















static cairo_bool_t
_convert_coord_to_short (double coord, short *v)
{
    *v = (short)coord;
    
    return *v == coord;
}

static cairo_bool_t
_convert_coord_to_unsigned_short (double coord, unsigned short *v)
{
    *v = (unsigned short)coord;
    
    return *v == coord;
}

static cairo_bool_t
_intersect_interval (double a_begin, double a_end, double b_begin, double b_end,
                     double *out_begin, double *out_end)
{
    *out_begin = a_begin;
    if (*out_begin < b_begin) {
        *out_begin = b_begin;
    }
    *out_end = a_end;
    if (*out_end > b_end) {
        *out_end = b_end;
    }
    return *out_begin < *out_end;
}

static cairo_bool_t
_get_rectangular_clip (cairo_t *cr,
                       int bounds_x, int bounds_y,
                       int bounds_width, int bounds_height,
                       cairo_bool_t *need_clip,
                       XRectangle *rectangles, int max_rectangles,
                       int *num_rectangles)
{
    cairo_rectangle_list_t *cliplist;
    cairo_rectangle_t *clips;
    int i;
    double b_x = bounds_x;
    double b_y = bounds_y;
    double b_x_most = bounds_x + bounds_width;
    double b_y_most = bounds_y + bounds_height;
    int rect_count = 0;
    cairo_bool_t retval = True;

    cliplist = cairo_copy_clip_rectangle_list (cr);
    if (cliplist->status != CAIRO_STATUS_SUCCESS) {
        retval = False;
        goto FINISH;
    }

    if (cliplist->num_rectangles == 0) {
        *num_rectangles = 0;
        *need_clip = True;
        goto FINISH;
    }

    clips = cliplist->rectangles;

    for (i = 0; i < cliplist->num_rectangles; ++i) {
        double intersect_x, intersect_y, intersect_x_most, intersect_y_most;
        
        
        if (b_x >= clips[i].x && b_x_most <= clips[i].x + clips[i].width &&
            b_y >= clips[i].y && b_y_most <= clips[i].y + clips[i].height) {
            
            *need_clip = False;
            goto FINISH;
        }
        
        if (_intersect_interval (b_x, b_x_most, clips[i].x, clips[i].x + clips[i].width,
                                 &intersect_x, &intersect_x_most) &&
            _intersect_interval (b_y, b_y_most, clips[i].y, clips[i].y + clips[i].height,
                                 &intersect_y, &intersect_y_most)) {
            XRectangle *rect = &rectangles[rect_count];

            if (rect_count >= max_rectangles) {
                retval = False;
                goto FINISH;
            }

            if (!_convert_coord_to_short (intersect_x, &rect->x) ||
                !_convert_coord_to_short (intersect_y, &rect->y) ||
                !_convert_coord_to_unsigned_short (intersect_x_most - intersect_x, &rect->width) ||
                !_convert_coord_to_unsigned_short (intersect_y_most - intersect_y, &rect->height))
            {
                retval = False;
                goto FINISH;
            }

            ++rect_count;
        }
    }
  
    *need_clip = True;
    *num_rectangles = rect_count;

FINISH:
    cairo_rectangle_list_destroy (cliplist);

    return retval;
}

#define MAX_STATIC_CLIP_RECTANGLES 50






static cairo_bool_t
_draw_with_xlib_direct (cairo_t *cr,
                        Display *default_display,
                        cairo_xlib_drawing_callback callback,
                        void *closure,
                        int bounds_width, int bounds_height,
                        cairo_xlib_drawing_support_t capabilities)
{
    cairo_surface_t *target;
    Drawable d;
    cairo_matrix_t matrix;
    short offset_x, offset_y;
    cairo_bool_t needs_clip;
    XRectangle rectangles[MAX_STATIC_CLIP_RECTANGLES];
    int rect_count;
    double device_offset_x, device_offset_y;
    int max_rectangles;
    Display *dpy;
    Visual *visual;
    cairo_bool_t have_rectangular_clip;

    target = cairo_get_group_target (cr);
    cairo_surface_get_device_offset (target, &device_offset_x, &device_offset_y);
    d = cairo_xlib_surface_get_drawable (target);

    cairo_get_matrix (cr, &matrix);
    
    
    
    if (matrix.xx != 1.0 || matrix.yy != 1.0 || matrix.xy != 0.0 || matrix.yx != 0.0) {
        CAIRO_XLIB_DRAWING_NOTE("TAKING SLOW PATH: matrix not a pure translation\n");
        return False;
    }
    

    if (!_convert_coord_to_short (matrix.x0 + device_offset_x, &offset_x) ||
        !_convert_coord_to_short (matrix.y0 + device_offset_y, &offset_y)) {
        CAIRO_XLIB_DRAWING_NOTE("TAKING SLOW PATH: non-integer offset\n");
        return False;
    }
    
    max_rectangles = 0;
    if (capabilities & CAIRO_XLIB_DRAWING_SUPPORTS_CLIP_RECT) {
      max_rectangles = 1;
    }
    if (capabilities & CAIRO_XLIB_DRAWING_SUPPORTS_CLIP_LIST) {
      max_rectangles = MAX_STATIC_CLIP_RECTANGLES;
    }
    
    
    


    cairo_identity_matrix (cr);
    cairo_translate (cr, -device_offset_x, -device_offset_y);
    have_rectangular_clip =
        _get_rectangular_clip (cr,
                               offset_x, offset_y, bounds_width, bounds_height,
                               &needs_clip,
                               rectangles, max_rectangles, &rect_count);
    cairo_set_matrix (cr, &matrix);
    if (!have_rectangular_clip) {
        CAIRO_XLIB_DRAWING_NOTE("TAKING SLOW PATH: unsupported clip\n");
        return False;
    }

    
    if (needs_clip && rect_count == 0) {
        CAIRO_XLIB_DRAWING_NOTE("TAKING FAST PATH: all clipped\n");
        return True;
    }
      
    
    if (cairo_get_operator (cr) != CAIRO_OPERATOR_OVER) {
        CAIRO_XLIB_DRAWING_NOTE("TAKING SLOW PATH: non-OVER operator\n");
        return False;
    }
    
      
    if (!(capabilities & CAIRO_XLIB_DRAWING_SUPPORTS_OFFSET) &&
        (offset_x != 0 || offset_y != 0)) {
        CAIRO_XLIB_DRAWING_NOTE("TAKING SLOW PATH: unsupported offset\n");
        return False;
    }
    
    


    if (!d) {
        CAIRO_XLIB_DRAWING_NOTE("TAKING SLOW PATH: non-X surface\n");
        return False;
    }
    
      
    dpy = cairo_xlib_surface_get_display (target);
    if (!(capabilities & CAIRO_XLIB_DRAWING_SUPPORTS_ALTERNATE_DISPLAY) &&
        dpy != default_display) {
        CAIRO_XLIB_DRAWING_NOTE("TAKING SLOW PATH: non-default display\n");
        return False;
    }
        
      
    visual = cairo_xlib_surface_get_visual (target);
    if (!(capabilities & CAIRO_XLIB_DRAWING_SUPPORTS_NONDEFAULT_VISUAL) &&
        DefaultVisual (dpy, DefaultScreen (dpy)) != visual) {
        CAIRO_XLIB_DRAWING_NOTE("TAKING SLOW PATH: non-default visual\n");
        return False;
    }
  
    
    CAIRO_XLIB_DRAWING_NOTE("TAKING FAST PATH\n");
    cairo_surface_flush (target);
    callback (closure, dpy, d, visual, offset_x, offset_y, rectangles,
              needs_clip ? rect_count : 0);
    cairo_surface_mark_dirty (target);
    return True;
}

static cairo_surface_t *
_create_temp_xlib_surface (cairo_t *cr, Display *dpy, int width, int height,
                           cairo_xlib_drawing_support_t capabilities)
{
    


    cairo_surface_t *target = cairo_get_target (cr);
    Drawable target_drawable = cairo_xlib_surface_get_drawable (target);

    int screen_index = DefaultScreen (dpy);
    Drawable drawable = RootWindow (dpy, screen_index);
    Screen *screen = DefaultScreenOfDisplay (dpy);
    Visual *visual = DefaultVisual (dpy, screen_index);
    int depth = DefaultDepth (dpy, screen_index);

    Pixmap pixmap;
    cairo_surface_t *result;
    pixmap_free_struct *pfs;
    
    

    if (target_drawable) {
        Display *target_display = cairo_xlib_surface_get_display (target);
        Visual *target_visual = cairo_xlib_surface_get_visual (target);
        if ((target_display == dpy ||
             (capabilities & CAIRO_XLIB_DRAWING_SUPPORTS_ALTERNATE_DISPLAY)) &&
            (target_visual == visual ||
             (capabilities & CAIRO_XLIB_DRAWING_SUPPORTS_NONDEFAULT_VISUAL))) {
            drawable = target_drawable;
            dpy = target_display;
            visual = target_visual;
            screen = cairo_xlib_surface_get_screen (target);
            depth = cairo_xlib_surface_get_depth (target);
        }
    }
    
    pfs = malloc (sizeof(pixmap_free_struct));
    if (pfs == NULL)
        return NULL;
        
    pixmap = XCreatePixmap (dpy, drawable, width, height, depth);
    if (!pixmap) {
        free (pfs);
        return NULL;
    }
    pfs->dpy = dpy;
    pfs->pixmap = pixmap;
  
    result = cairo_xlib_surface_create (dpy, pixmap, visual, width, height);
    if (cairo_surface_status (result) != CAIRO_STATUS_SUCCESS) {
        pixmap_free_func (pfs);
        return NULL;
    }
    
    cairo_surface_set_user_data (result, &pixmap_free_key, pfs, pixmap_free_func);
    return result;
}

static cairo_bool_t
_draw_onto_temp_xlib_surface (cairo_surface_t *temp_xlib_surface,
                              cairo_xlib_drawing_callback callback,
                              void *closure,
                              double background_gray_value)
{
    Drawable d = cairo_xlib_surface_get_drawable (temp_xlib_surface);
    Display *dpy = cairo_xlib_surface_get_display (temp_xlib_surface);
    Visual *visual = cairo_xlib_surface_get_visual (temp_xlib_surface);
    cairo_bool_t result;
    cairo_t *cr = cairo_create (temp_xlib_surface);
    cairo_set_source_rgb (cr, background_gray_value, background_gray_value,
                          background_gray_value);
    cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint (cr);
    cairo_destroy (cr);
    
    cairo_surface_flush (temp_xlib_surface);
    

    result = callback (closure, dpy, d, visual, 0, 0, NULL, 0);
    cairo_surface_mark_dirty (temp_xlib_surface);
    return result;
}

static cairo_surface_t *
_copy_xlib_surface_to_image (cairo_surface_t *temp_xlib_surface,
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
    cairo_set_source_surface (cr, temp_xlib_surface, 0, 0);
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
                       cairo_xlib_drawing_result_t *analysis)
{
    int num_pixels = width*height;
    int i;
    uint32_t first;
    uint32_t deltas = 0;
    unsigned char first_alpha;
  
    if (num_pixels == 0) {
        if (analysis) {
            analysis->uniform_alpha = True;
            analysis->uniform_color = True;
            
            analysis->alpha = 1.0;
            analysis->r = analysis->g = analysis->b = 0.0;
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
        analysis->uniform_alpha = (deltas >> 24) == 0;
        if (analysis->uniform_alpha) {
            analysis->alpha = first_alpha/255.0;
            



            analysis->uniform_color = (deltas & ~(0xFF << 24)) == 0;
            if (analysis->uniform_color) {
                if (first_alpha == 0) {
                    
                    analysis->r = analysis->g = analysis->b = 0.0;
                } else {
                    double d_first_alpha = first_alpha;
                    analysis->r = (first & 0xFF)/d_first_alpha;
                    analysis->g = ((first >> 8) & 0xFF)/d_first_alpha;
                    analysis->b = ((first >> 16) & 0xFF)/d_first_alpha;
                }
            }
        }
    }
}

void
cairo_draw_with_xlib (cairo_t *cr,
                      cairo_xlib_drawing_callback callback,
                      void *closure,
                      Display *dpy,
                      unsigned int width, unsigned int height,
                      cairo_xlib_drawing_opacity_t is_opaque,
                      cairo_xlib_drawing_support_t capabilities,
                      cairo_xlib_drawing_result_t *result)
{
    cairo_surface_t *temp_xlib_surface;
    cairo_surface_t *black_image_surface;
    cairo_surface_t *white_image_surface;
    unsigned char *black_data;
    unsigned char *white_data;
  
    if (result) {
        result->surface = NULL;
        result->uniform_alpha = False;
        result->uniform_color = False;
    }
    
    


    if (width == 0 || height == 0)
        return;

    if (_draw_with_xlib_direct (cr, dpy, callback, closure, width, height,
                                capabilities))
        return;
  
    temp_xlib_surface = _create_temp_xlib_surface (cr, dpy, width, height,
                                                   capabilities);
    if (temp_xlib_surface == NULL)
        return;
    

    dpy = cairo_xlib_surface_get_display (temp_xlib_surface);
  
    if (!_draw_onto_temp_xlib_surface (temp_xlib_surface, callback, closure, 0.0)) {
        cairo_surface_destroy (temp_xlib_surface);
        return;
    }
  
    if (is_opaque == CAIRO_XLIB_DRAWING_OPAQUE) {
        cairo_set_source_surface (cr, temp_xlib_surface, 0.0, 0.0);
        cairo_paint (cr);
        if (result) {
            result->surface = temp_xlib_surface;
            

            result->uniform_alpha = True;
            result->alpha = 1.0;
        } else {
            cairo_surface_destroy (temp_xlib_surface);
        }
        return;
    }
    
    black_image_surface =
        _copy_xlib_surface_to_image (temp_xlib_surface, CAIRO_FORMAT_ARGB32,
                                     width, height, &black_data);
    
    _draw_onto_temp_xlib_surface (temp_xlib_surface, callback, closure, 1.0);
    white_image_surface =
        _copy_xlib_surface_to_image (temp_xlib_surface, CAIRO_FORMAT_RGB24,
                                     width, height, &white_data);
  
    cairo_surface_destroy (temp_xlib_surface);
    
    if (black_image_surface && white_image_surface &&
        cairo_surface_status (black_image_surface) == CAIRO_STATUS_SUCCESS &&
        cairo_surface_status (white_image_surface) == CAIRO_STATUS_SUCCESS &&
        black_data != NULL && white_data != NULL) {
        cairo_surface_flush (black_image_surface);
        cairo_surface_flush (white_image_surface);
        _compute_alpha_values ((uint32_t*)black_data, (uint32_t*)white_data, width, height, result);
        cairo_surface_mark_dirty (black_image_surface);
        
        cairo_set_source_surface (cr, black_image_surface, 0.0, 0.0);
        




        if (result && (!result->uniform_alpha || !result->uniform_color)) {
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
            
            result->surface = similar_surface;
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
