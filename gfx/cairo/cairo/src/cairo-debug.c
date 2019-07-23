


































#include "cairoint.h"






















void
cairo_debug_reset_static_data (void)
{
    CAIRO_MUTEX_INITIALIZE ();

    _cairo_scaled_font_map_destroy ();

    _cairo_toy_font_face_reset_static_data ();

#if CAIRO_HAS_FT_FONT
    _cairo_ft_font_reset_static_data ();
#endif

    _cairo_intern_string_reset_static_data ();

    _cairo_scaled_font_reset_static_data ();

    _cairo_pattern_reset_static_data ();

    CAIRO_MUTEX_FINALIZE ();
}

#if HAVE_VALGRIND
void
_cairo_debug_check_image_surface_is_defined (const cairo_surface_t *surface)
{
    const cairo_image_surface_t *image = (cairo_image_surface_t *) surface;
    const uint8_t *bits;
    int row, width;

    if (surface == NULL)
	return;

    if (! RUNNING_ON_VALGRIND)
	return;

    bits = image->data;
    switch (image->format) {
    case CAIRO_FORMAT_A1:
	width = (image->width + 7)/8;
	break;
    case CAIRO_FORMAT_A8:
	width = image->width;
	break;
    case CAIRO_FORMAT_RGB24:
    case CAIRO_FORMAT_ARGB32:
	width = image->width*4;
	break;
    default:
	
	return;
    }

    for (row = 0; row < image->height; row++) {
	VALGRIND_CHECK_MEM_IS_DEFINED (bits, width);
	
	VALGRIND_MAKE_MEM_DEFINED (bits, width);
	bits += image->stride;
    }
}
#endif


#if 0
void
_cairo_image_surface_write_to_ppm (cairo_image_surface_t *isurf, const char *fn)
{
    char *fmt;
    if (isurf->format == CAIRO_FORMAT_ARGB32 || isurf->format == CAIRO_FORMAT_RGB24)
        fmt = "P6";
    else if (isurf->format == CAIRO_FORMAT_A8)
        fmt = "P5";
    else
        return;

    FILE *fp = fopen(fn, "wb");
    if (!fp)
        return;

    fprintf (fp, "%s %d %d 255\n", fmt,isurf->width, isurf->height);
    for (int j = 0; j < isurf->height; j++) {
        unsigned char *row = isurf->data + isurf->stride * j;
        for (int i = 0; i < isurf->width; i++) {
            if (isurf->format == CAIRO_FORMAT_ARGB32 || isurf->format == CAIRO_FORMAT_RGB24) {
                unsigned char r = *row++;
                unsigned char g = *row++;
                unsigned char b = *row++;
                *row++;
                putc(r, fp);
                putc(g, fp);
                putc(b, fp);
            } else {
                unsigned char a = *row++;
                putc(a, fp);
            }
        }
    }

    fclose (fp);

    fprintf (stderr, "Wrote %s\n", fn);
}
#endif
